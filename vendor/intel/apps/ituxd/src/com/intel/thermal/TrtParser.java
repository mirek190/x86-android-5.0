/*
 * Copyright 2014 Intel Corporation All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.thermal;

import android.os.SystemProperties;

import android.util.Slog;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.UnsupportedEncodingException;

import java.nio.charset.Charset;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
/**
 * <p> TrtParser parses the TRT(Thermal RelationShip Table) data.
 * It creates device throttle configuration file and sensor
 * configuration file for Thermal service to apply its policies.
 * </p>
 */
public class TrtParser {
    private static final int TARGET_DEV = 0;
    private static final int SOURCE_DEV = 1;
    private static final int MAX_THRESHOLD_VALUES = 15;
    private static final int MAX_COOL_DEVS_IN_ZONE = 10;
    private static final int TUPLE_LEN = 64;
    private static final int POLL_DELAY = 30000; // 30 seconds
    private static final int DEFAULT_TJMAX = 90000; // Core critical temperature

    private static final String DEV_PATH ="/dev/acpi_thermal_rel";
    private static final String CORE_TEMP_PATH = "/sys/devices/platform/coretemp.0/";
    private static final String THERMAL_ZONE_PATH = "/sys/class/thermal/thermal_zone";
    private static final String CONFIG_PATH = "/data/acpi_thermal/";
    private static final String SENSOR_FILE_PATH =
            "/data/acpi_thermal/thermal_sensor_config.xml";
    private static final String THROTTLE_FILE_PATH =
            "/data/acpi_thermal/thermal_throttle_config.xml";
    private static final String PRODUCT_NAME_PATH = "/sys/class/dmi/id/product_name";
    private static final String TAG = "Thermal:TRT parser";
    private static final String SENSOR_HEADER =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            + "\n<!-- Thermal Management Configuration File -->"
            + "\n<!-- Intel Corporation 2014 -->"
            + "\n<thermalconfig>\n";
    private static final String THROTTLE_HEADER =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            + "\n<!-- Thermal Management Configuration File -->"
            + "\n<!-- Intel Corporation 2014 -->"
            + "\n<thermalthrottleconfig>\n";
    private static final String PLAT_INFO_HEADER = "<PlatformInfo>\n\t<PlatformName>";
    private static final String PLAT_INFO_FOOT = "</PlatformName>\n</PlatformInfo>";

    private static int sCount, sLength, sNumSensors, sNumCoolingDevs;

    private static byte[] mTrtBuf = null;

    private ArrayList<TrtTuple> mTrtTable;
    private ArrayList<Zone> mZoneList;
    private ArrayList<CoolingDevice> mCoolingDevsList;
    private ArrayList<ZoneRelationTable> mZoneRelList;

    /**
     * Class to represent TRT table objects as meaningful data structures.
     */
    class TrtTuple {
        String mSrc; // Source device
        String mTgt; // Target device
        long mSmpl; // Sampling period
    }

    /**
     * Class to represent thermal zone and its attributes.
     */
    class Zone {
        int mId;
        String mName;
        String mLogic;
        boolean mSupportsUevent;
        int mDebounceInterval;
        int mThresholds[] = new int[MAX_THRESHOLD_VALUES];
        long mPoll[] = new long[MAX_THRESHOLD_VALUES];
    }

    /**
     * Class to represent cooling device and its attributes.
     */
    class CoolingDevice {
        String mName;
        int mId;
        String mClassPath;
        String mThrottlePath;
    }

    /**
     * Class to represent zone to cooling devices mapping.
     */
    class ZoneRelationTable {
        int mNumCoolingDevs;
        String mZone;
        int mId;
        boolean mCanShutDown;
        String mCoolingDevs[] = new String[MAX_COOL_DEVS_IN_ZONE];
    }

    /**
     * Method that substitutes thermal zone/cooling device names for TRT Strings.
     * @param type device type: source device or target device
     * @param name Name that needs substitution
     * @return substituted string
     */
    private String substituteString(int type, String name) {
        int i = 0;
        if (type == TARGET_DEV) {
            if (name.contains("CPU")) return "Core 0";
        } else if (type == SOURCE_DEV) {
            if (name.contains("CPU")) return "Processor";
            else if (name.contains("PLY") || name.contains("DSP")) return "LCD";
        }
        return name.substring(0, 4);
    }

    /**
     * Method that reads raw bytes of TRT data.
     * @return true on success and false on failure
     */
    private boolean getTrtTableBytes() {
        byte[] temp = new byte[8];
        mTrtBuf = ThermalUtils.getTRTData(DEV_PATH);
        if (mTrtBuf == null) return false;
        sCount = (mTrtBuf.length / TUPLE_LEN) - 1; // last tuple is reserved
        int bufptr = 0;
        for (int i = 0; i < sCount; i++) {
            TrtTuple tuple = new TrtTuple();
            System.arraycopy(mTrtBuf, bufptr, temp, 0, 8);
            tuple.mSrc = substituteString(SOURCE_DEV, new String(temp));
            System.arraycopy(mTrtBuf, bufptr + 8, temp, 0, 8);
            tuple.mTgt = substituteString(TARGET_DEV, new String(temp));
            tuple.mSmpl = POLL_DELAY;
            bufptr += TUPLE_LEN;
            mTrtTable.add(tuple);
        }
        return true;
    }

    /**
     * Method that converts the TRT table to zone to cooling device mapping.
     */
    private void convertTrtTableToZoneRelations() {
        int i, j, zoneNum, temp;
        int id = 0, threshold = 0;
        boolean takeDefault = false;
        // We never know how many zones will be there before going through
        // the entire list of tuples in TRT table.
        // So, create as many ZoneRelationTable objects as the TRT tuples are.
        for (i = 0; i < sCount; i++) {
            ZoneRelationTable zrt = new ZoneRelationTable();
            mZoneRelList.add(zrt);
        }
        for (i = 0; i < sCount; i++) {
            for (j = mTrtTable.size() - 1; j > i; j--) {
                // When a Target device object is encountered again,
                // increment number of cooling devices under that
                // and add the cooling device to the current zone.
                if (mTrtTable.get(i).mTgt.equals(mTrtTable.get(j).mTgt)) {
                    temp = mZoneRelList.get(id).mNumCoolingDevs;
                    mZoneRelList.get(id).mCoolingDevs[temp] = mTrtTable.get(i).mSrc;
                    mZoneRelList.get(id).mNumCoolingDevs++;
                    break;
                }
            }
            if ( i == j) {
                temp = mZoneRelList.get(id).mNumCoolingDevs;
                mZoneRelList.get(id).mCoolingDevs[temp] = mTrtTable.get(i).mSrc;
                mZoneRelList.get(id).mNumCoolingDevs++;
                mZoneRelList.get(id).mId = id;
                mZoneRelList.get(id).mZone = mTrtTable.get(i).mTgt;
                Zone zone = new Zone();
                zone.mId = id;
                zone.mName = mTrtTable.get(i).mTgt;
                zone.mLogic = "Raw";
                zone.mSupportsUevent = false;
                zone.mDebounceInterval = 2000;
                zoneNum = ThermalUtils.getThermalZoneIndexContains(mTrtTable.get(i).mTgt);
                // All thresholds in millidegrees.
                // If zone is core temperature, read from CORE_TEMP path.
                // Other zones can have path auto-generated to read thresholds.
                if (mTrtTable.get(i).mTgt.contains("Core")) {
                    try {
                        threshold = Integer.parseInt
                                (ThermalUtils.readSysfs(CORE_TEMP_PATH + "temp2_crit"));
                    } catch (NumberFormatException e) {
                        threshold = DEFAULT_TJMAX;
                    }
                    zone.mSupportsUevent = true;
                    zone.mThresholds[2] = threshold;
                    zone.mThresholds[1] = (90 * threshold) / 100;
                    zone.mThresholds[0] = (80 * threshold) / 100;
                    takeDefault = false;
                } else if (zoneNum < 0) {
                    takeDefault = true;
                } else {
                    try {
                        zone.mThresholds[0] = Integer.parseInt
                                (ThermalUtils.readSysfs
                                (THERMAL_ZONE_PATH + zoneNum + "/trip_point_2_temp"));
                        zone.mThresholds[2] = Integer.parseInt
                                (ThermalUtils.readSysfs
                                (THERMAL_ZONE_PATH + zoneNum + "/trip_point_3_temp"));
                        zone.mThresholds[1] =
                                (zone.mThresholds[0] + zone.mThresholds[2]) / 2;
                    } catch (NumberFormatException e) {
                        takeDefault = true;
                    }
                }
                // Default values if thresholds could not be read from sysfs
                if (takeDefault) {
                    zone.mThresholds[0] = 40000;
                    zone.mThresholds[1] = 50000;
                    zone.mThresholds[2] = 60000;
                }
                zone.mPoll[0] = mTrtTable.get(i).mSmpl;
                zone.mPoll[1] = mTrtTable.get(i).mSmpl;
                zone.mPoll[2] = mTrtTable.get(i).mSmpl;
                id++;
                mZoneList.add(zone);
            }
        }
        sNumSensors = id;
    }

    /**
     * Method that fetches info about cooling devices from TRT table.
     */
    private void getCoolingDevsInfoFromTrt() {
        int i, j;
        int id = 0;
        for (i = 0; i < sCount; i++) {
            for (j = mTrtTable.size() - 1; j > i; j--) {
                if (mTrtTable.get(i).mSrc.equals(mTrtTable.get(j).mSrc)) break;
            }
            if (i == j) {
                CoolingDevice cdev  = new CoolingDevice();
                cdev.mName = mTrtTable.get(i).mSrc;
                cdev.mId = id;
                cdev.mClassPath = "auto";
                cdev.mThrottlePath = "auto";
                id++;
                mCoolingDevsList.add(cdev);
            }
        }
        sNumCoolingDevs = id;
    }

    /**
     * Method that will write sensor configuration file.
     * @return true on success and false on failure.
     */
    private boolean writeSensorConfig() {
        int i;
        FileWriter fw = null;
        BufferedWriter bw = null;
        try {
            fw = new FileWriter(SENSOR_FILE_PATH);
            bw = new BufferedWriter(fw);
            bw.write(SENSOR_HEADER);
            bw.write("<ConfigFileVersion>");
            bw.write(SystemProperties.get("ro.thermal.ituxversion", "2.5"));
            bw.write("</ConfigFileVersion>\n");
            bw.write(PLAT_INFO_HEADER);
            bw.write(ThermalUtils.readSysfs(PRODUCT_NAME_PATH));
            bw.write(PLAT_INFO_FOOT);
            for (i = 0; i < mZoneList.size(); i++) {
                bw.write("\n\t<Sensor>");
                bw.write("\n\t\t<SensorName>" + mZoneList.get(i).mName + "</SensorName>");
                if (mZoneList.get(i).mName.equals("Core 0")) {
                    bw.write("\n\t\t<SensorPath>/sys/devices/platform/coretemp.0/</SensorPath>");
                    bw.write("\n\t\t<InputTemp>temp2_input</InputTemp>");
                    bw.write("\n\t\t<HighTemp>temp2_threshold2</HighTemp>");
                    bw.write("\n\t\t<LowTemp>temp2_threshold1</LowTemp>");
                    bw.write("\n\t\t<UEventDevPath>DEVPATH=/devices/platform/coretemp.0");
                    bw.write("</UEventDevPath>");
                }
                bw.write("\n\t</Sensor>");
            }
            for (i = 0; i < mZoneList.size(); i++) {
                bw.write("\n<Zone>");
                bw.write("\n\t<ZoneID>" + mZoneList.get(i).mId + "</ZoneID>");
                bw.write("\n\t<ZoneName>" + mZoneList.get(i).mName + "</ZoneName>");
                bw.write("\n\t<ZoneLogic>" + mZoneList.get(i).mLogic + "</ZoneLogic>");
                bw.write("\n\t<DebounceInterval>" + mZoneList.get(i).mDebounceInterval);
                bw.write("</DebounceInterval>");
                bw.write("\n\t<PollDelay>");
                bw.write("\n\t\t<DelayNormal>" + mZoneList.get(i).mPoll[0] + "</DelayNormal>");
                bw.write("\n\t\t<DelayWarning>" + mZoneList.get(i).mPoll[1] + "</DelayWarning>");
                bw.write("\n\t\t<DelayAlert>" + mZoneList.get(i).mPoll[2] + "</DelayAlert>");
                bw.write("\n\t</PollDelay>");
                bw.write("\n\t<ZoneThreshold>");
                bw.write("\n\t\t<ZoneThresholdNormal>" +
                        mZoneList.get(i).mThresholds[0] + "</ZoneThresholdNormal>");
                bw.write("\n\t\t<ZoneThresholdWarning>" +
                        mZoneList.get(i).mThresholds[1] + "</ZoneThresholdWarning>");
                bw.write("\n\t\t<ZoneThresholdAlert>" +
                        mZoneList.get(i).mThresholds[2] + "</ZoneThresholdAlert>");
                bw.write("\n\t</ZoneThreshold>");
                bw.write("\n\t<SensorAttrib>\n\t\t<SensorName>" +
                         mZoneList.get(i).mName + "</SensorName>" +
                         "\n\t</SensorAttrib>\n</Zone>");
            }
            bw.write("</thermalconfig>");
            bw.close();
            return true;
        } catch (IOException e) {
            Slog.e(TAG, "Exception caught:" + e);
            return false;
        } finally {
            try {
                if (bw != null) {
                    bw.close();
                }
            } catch (IOException e) {
                Slog.e(TAG, "IOException caught while closing the writer");
                return true;
            }
        }
    }

    /**
     * Method that will write device throttle configuration file.
     * @return true on success and false on failure.
     */
    private boolean writeThrottleConfig() {
        int i, j, k, tmp = 0;
        FileWriter fw = null;
        BufferedWriter bw = null;
        try {
            fw = new FileWriter(THROTTLE_FILE_PATH);
            bw = new BufferedWriter(fw);
            bw.write(THROTTLE_HEADER);
            bw.write("<ConfigFileVersion>" +
                    SystemProperties.get("ro.thermal.ituxversion", "2.5"));
            bw.write("</ConfigFileVersion>\n");
            bw.write(PLAT_INFO_HEADER);
            bw.write(ThermalUtils.readSysfs(PRODUCT_NAME_PATH));
            bw.write(PLAT_INFO_FOOT);
            for (i = 0; i < mCoolingDevsList.size(); i++) {
                bw.write("\n\t<ContributingDeviceInfo>");
                bw.write("\n\t\t<CDeviceName>" + mCoolingDevsList.get(i).mName + "</CDeviceName>");
                bw.write("\n\t\t<CDeviceID>" + mCoolingDevsList.get(i).mId + "</CDeviceID>");
                bw.write("\n\t\t<CDeviceClassPath>" + mCoolingDevsList.get(i).mClassPath +
                        "</CDeviceClassPath>");
                bw.write("\n\t\t<CDeviceThrottlePath>" + mCoolingDevsList.get(i).mThrottlePath +
                        "</CDeviceThrottlePath>");
                bw.write("\n\t</ContributingDeviceInfo>");
            }
            for (i = 0; i < mZoneList.size(); i++) {
                bw.write("\n\t<ZoneThrottleInfo>");
                bw.write("\n\t\t<ZoneID>" + mZoneRelList.get(i).mId + "</ZoneID>");
                bw.write("\n\t\t<CriticalShutDown>0</CriticalShutDown>");
                for (j = 0; j < mZoneRelList.get(i).mNumCoolingDevs; j++) {
                    bw.write("\n\t\t<CoolingDeviceInfo>");
                    for (k = 0; k < mCoolingDevsList.size(); k++) {
                        if (mCoolingDevsList.get(k).mName.equals
                                (mZoneRelList.get(i).mCoolingDevs[j])) {
                            tmp = mCoolingDevsList.get(k).mId;
                            break;
                        }
                    }
                    bw.write("\n\t\t\t<CoolingDevId>" + tmp + "</CoolingDevId>");
                    bw.write("\n\t\t</CoolingDeviceInfo>");
                }
                bw.write("\n\t</ZoneThrottleInfo>");
            }
            bw.write("\n</thermalthrottleconfig>");
            bw.close();
            return true;
        } catch (IOException e) {
            Slog.e(TAG, "Caught exception:" + e);
            return false;
        } finally {
            try {
                if (bw != null) {
                    bw.close();
                    return true;
                }
            } catch (IOException e) {
                Slog.e(TAG, "Exception while closing the writer");
                return true;
            }
        }
    }

    /**
     * Method that will clear arraylists and byte array
     */
    private void cleanup() {
        if (mTrtTable != null) mTrtTable.clear();
        if (mZoneList != null) mZoneList.clear();
        if (mCoolingDevsList != null) mCoolingDevsList.clear();
        if (mZoneRelList != null) mZoneRelList.clear();
        mTrtBuf = null;
    }

    /**
     * Method that will call methods to fetch TRT data, convert
     * into zone relationship table and write config files.
     * Returns true on success, else false
     */
    public boolean writeConfigFiles() {
        File confFile = new File(CONFIG_PATH);
        if (!confFile.exists()) {
            if (!confFile.mkdir() || !confFile.isDirectory()) {
                Slog.i(TAG, "Could not create config directory");
                return false;
            }
        }

        if (!getTrtTableBytes()) {
            Slog.i(TAG, "Could not get TRT data");
            return false;
        }
        convertTrtTableToZoneRelations();
        getCoolingDevsInfoFromTrt();
        if (!writeSensorConfig()) {
            Slog.e(TAG, "Error in writing to sensor config");
            cleanup();
            return false;
        }
        if (!writeThrottleConfig()) {
            Slog.e(TAG, "Error in writing to throttle config");
            cleanup();
            return false;
        }
        cleanup();
        return true;
    }

    /**
     * Class constructor.
     */
    public TrtParser() {
        mTrtTable = new ArrayList<TrtTuple>();
        mZoneList = new ArrayList<Zone>();
        mCoolingDevsList = new ArrayList<CoolingDevice>();
        mZoneRelList = new ArrayList<ZoneRelationTable>();
    }
}
