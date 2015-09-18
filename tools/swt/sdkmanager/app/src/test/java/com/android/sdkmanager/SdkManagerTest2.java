/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.sdkmanager;


import com.android.SdkConstants;
import com.android.prefs.AndroidLocation.AndroidLocationException;
import com.android.sdklib.SdkManagerTestCase;
import com.android.sdklib.internal.avd.AvdManager;
import com.android.utils.ILogger;

import java.io.File;
import java.io.IOException;

import junit.framework.Assert;
import junit.framework.AssertionFailedError;

/**
 * Tests the SDK Manager command-line tool using a mock "black box"
 * approach by invoking the main with a given command line and then
 * checking the results.
 *
 * @see SdkManagerTest SdkManagerTest for testing internals methods directly.
 */
public class SdkManagerTest2 extends SdkManagerTestCase {

    private File mFolderSiX86;
    private File mFolderSiArm;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        // add 2 tag/abi folders with a new skin each
        mFolderSiX86 = makeSystemImageFolder(TARGET_DIR_NAME_0, "tag-1", "x86");
        makeFakeSkin(mFolderSiX86, "Tag1X86Skin");
        mFolderSiArm = makeSystemImageFolder(TARGET_DIR_NAME_0, "tag-1", "armeabi");
        makeFakeSkin(mFolderSiArm, "Tag1ArmSkin");
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    // ------ tests

    public void testListTargets() throws Exception {
        runCmdLine("list", "targets", "--compact");
        assertEquals(
                "P android-0\n",
                getLog().toString());

        runCmdLine("list", "target");
        assertEquals(
                "P Available Android targets:\n" +
                "P ----------\n" +
                "P id: 1 or \"android-0\"\n" +
                "P      Name: Android 0.0\n" +
                "P      Type: Platform\n" +
                "P      API level: 0\n" +
                "P      Revision: 1\n" +
                "P      Skins: HVGA (default), Tag1ArmSkin, Tag1X86Skin\n" +
                "P  Tag/ABIs : default/armeabi, tag-1/armeabi, tag-1/x86\n",
                getLog().toString());
    }

    public void testListAvds() throws Exception {
        runCmdLine("list", "avds", "--compact");
        assertEquals("", getLog().toString());

        runCmdLine("list", "avd");
        assertEquals("P Available Android Virtual Devices:\n", getLog().toString());
    }

    public void testListDevices() throws Exception {
        runCmdLine("list", "device", "--compact");
        assertEquals(
                "P Galaxy Nexus\n" +
                "P MockDevice-tag-1\n" +
                "P MockDevice-tag-1\n" +
                "P Nexus 10\n" +
                "P Nexus 4\n" +
                "P Nexus 5\n" +
                "P Nexus 7 2013\n" +
                "P Nexus 7\n" +
                "P Nexus One\n" +
                "P Nexus S\n" +
                "P 2.7in QVGA\n" +
                "P 2.7in QVGA slider\n" +
                "P 3.2in HVGA slider (ADP1)\n" +
                "P 3.2in QVGA (ADP2)\n" +
                "P 3.3in WQVGA\n" +
                "P 3.4in WQVGA\n" +
                "P 3.7 FWVGA slider\n" +
                "P 3.7in WVGA (Nexus One)\n" +
                "P 4in WVGA (Nexus S)\n" +
                "P 4.65in 720p (Galaxy Nexus)\n" +
                "P 4.7in WXGA\n" +
                "P 5.1in WVGA\n" +
                "P 5.4in FWVGA\n" +
                "P 7in WSVGA (Tablet)\n" +
                "P 10.1in WXGA (Tablet)\n",
                getLog().toString());

        runCmdLine("list", "devices");
        assertEquals(
                "P Available devices definitions:\n" +
                "P id: 0 or \"Galaxy Nexus\"\n" +
                "P     Name: Galaxy Nexus\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 1 or \"MockDevice-tag-1\"\n" +
                "P     Name: Mock Tag 1 Device Name\n" +
                "P     OEM : Mock Tag 1 OEM\n" +
                "P ---------\n" +
                "P id: 2 or \"MockDevice-tag-1\"\n" +
                "P     Name: Mock Tag 1 Device Name\n" +
                "P     OEM : Mock Tag 1 OEM\n" +
                "P ---------\n" +
                "P id: 3 or \"Nexus 10\"\n" +
                "P     Name: Nexus 10\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 4 or \"Nexus 4\"\n" +
                "P     Name: Nexus 4\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 5 or \"Nexus 5\"\n" +
                "P     Name: Nexus 5\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 6 or \"Nexus 7 2013\"\n" +
                "P     Name: Nexus 7\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 7 or \"Nexus 7\"\n" +
                "P     Name: Nexus 7 (2012)\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 8 or \"Nexus One\"\n" +
                "P     Name: Nexus One\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 9 or \"Nexus S\"\n" +
                "P     Name: Nexus S\n" +
                "P     OEM : Google\n" +
                "P ---------\n" +
                "P id: 10 or \"2.7in QVGA\"\n" +
                "P     Name: 2.7\" QVGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 11 or \"2.7in QVGA slider\"\n" +
                "P     Name: 2.7\" QVGA slider\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 12 or \"3.2in HVGA slider (ADP1)\"\n" +
                "P     Name: 3.2\" HVGA slider (ADP1)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 13 or \"3.2in QVGA (ADP2)\"\n" +
                "P     Name: 3.2\" QVGA (ADP2)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 14 or \"3.3in WQVGA\"\n" +
                "P     Name: 3.3\" WQVGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 15 or \"3.4in WQVGA\"\n" +
                "P     Name: 3.4\" WQVGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 16 or \"3.7 FWVGA slider\"\n" +
                "P     Name: 3.7\" FWVGA slider\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 17 or \"3.7in WVGA (Nexus One)\"\n" +
                "P     Name: 3.7\" WVGA (Nexus One)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 18 or \"4in WVGA (Nexus S)\"\n" +
                "P     Name: 4\" WVGA (Nexus S)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 19 or \"4.65in 720p (Galaxy Nexus)\"\n" +
                "P     Name: 4.65\" 720p (Galaxy Nexus)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 20 or \"4.7in WXGA\"\n" +
                "P     Name: 4.7\" WXGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 21 or \"5.1in WVGA\"\n" +
                "P     Name: 5.1\" WVGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 22 or \"5.4in FWVGA\"\n" +
                "P     Name: 5.4\" FWVGA\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 23 or \"7in WSVGA (Tablet)\"\n" +
                "P     Name: 7\" WSVGA (Tablet)\n" +
                "P     OEM : Generic\n" +
                "P ---------\n" +
                "P id: 24 or \"10.1in WXGA (Tablet)\"\n" +
                "P     Name: 10.1\" WXGA (Tablet)\n" +
                "P     OEM : Generic\n",
                getLog().toString());
    }

    public void testCreateAvd() throws Exception {
        runCmdLine("list", "avd");
        assertEquals("P Available Android Virtual Devices:\n", getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "my-avd",
                "--abi",    "armeabi");
        assertEquals(
                "P Android 0.0 is a basic Android platform.\n" +
                "P Do you wish to create a custom hardware profile [no]" +
                "Created AVD 'my-avd' based on Android 0.0, ARM (armeabi) processor\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "my-avd2",
                "--abi",    "default/armeabi");
        assertEquals(
                "P Android 0.0 is a basic Android platform.\n" +
                "P Do you wish to create a custom hardware profile [no]" +
                "Created AVD 'my-avd2' based on Android 0.0, ARM (armeabi) processor\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "avd-for-tag1",
                "--abi",    "tag-1/armeabi");
        assertEquals(
                "P Android 0.0 is a basic Android platform.\n" +
                "P Do you wish to create a custom hardware profile [no]" +
                "Created AVD 'avd-for-tag1' based on Android 0.0, Tag 1 ARM (armeabi) processor\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "avd-for-tag2",
                "--tag",    "tag-1",
                "--abi",    "armeabi");
        assertEquals(
                "P Android 0.0 is a basic Android platform.\n" +
                "P Do you wish to create a custom hardware profile [no]" +
                "Created AVD 'avd-for-tag2' based on Android 0.0, Tag 1 ARM (armeabi) processor\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "device-tag1",
                "--tag",    "tag-1",
                "--abi",    "armeabi",
                "--device", "MockDevice-tag-1");
        assertEquals(
                "P Created AVD 'device-tag1' based on Android 0.0, Tag 1 ARM (armeabi) processor,\n" +
                "with the following hardware config:\n" +
                "hw.accelerometer=no\n" +
                "hw.audioInput=yes\n" +
                "hw.battery=yes\n" +
                "hw.dPad=no\n" +
                "hw.device.hash2=MD5:7e046a6244489b7deeca681ab5a76cb3\n" +
                "hw.device.manufacturer=Mock Tag 1 OEM\n" +
                "hw.device.name=MockDevice-tag-1\n" +
                "hw.gps=no\n" +
                "hw.keyboard=yes\n" +
                "hw.lcd.density=240\n" +
                "hw.mainKeys=no\n" +
                "hw.sdCard=no\n" +
                "hw.sensors.orientation=no\n" +
                "hw.sensors.proximity=no\n" +
                "hw.trackBall=no\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "gn-avd",
                "--device", "0",
                "--abi",    "armeabi");
        assertEquals(
                "P Created AVD 'gn-avd' based on Android 0.0, ARM (armeabi) processor,\n" +
                "with the following hardware config:\n" +
                "hw.accelerometer=yes\n" +
                "hw.audioInput=yes\n" +
                "hw.battery=yes\n" +
                "hw.dPad=no\n" +
                "hw.device.hash2=MD5:6930e145748b87e87d3f40cabd140a41\n" +
                "hw.device.manufacturer=Google\n" +
                "hw.device.name=Galaxy Nexus\n" +
                "hw.gps=yes\n" +
                "hw.keyboard=no\n" +
                "hw.lcd.density=320\n" +
                "hw.mainKeys=no\n" +
                "hw.sdCard=no\n" +
                "hw.sensors.orientation=yes\n" +
                "hw.sensors.proximity=yes\n" +
                "hw.trackBall=no\n",
                getLog().toString());

        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "gn-sdcard",
                "--device", "0",
                "--abi",    "armeabi",
                "--sdcard", "1023G");
        assertEquals(
                "P [EXEC] @SDK/tools/mksdcard 1023G @AVD/gn-sdcard.avd/sdcard.img\n" +
                "P Created AVD 'gn-sdcard' based on Android 0.0, ARM (armeabi) processor,\n" +
                "with the following hardware config:\n" +
                "hw.accelerometer=yes\n" +
                "hw.audioInput=yes\n" +
                "hw.battery=yes\n" +
                "hw.dPad=no\n" +
                "hw.device.hash2=MD5:6930e145748b87e87d3f40cabd140a41\n" +
                "hw.device.manufacturer=Google\n" +
                "hw.device.name=Galaxy Nexus\n" +
                "hw.gps=yes\n" +
                "hw.keyboard=no\n" +
                "hw.lcd.density=320\n" +
                "hw.mainKeys=no\n" +
                "hw.sdCard=yes\n" +
                "hw.sensors.orientation=yes\n" +
                "hw.sensors.proximity=yes\n" +
                "hw.trackBall=no\n",
                sanitizePaths(getLog().toString()));

        runCmdLine("list", "avd");
        assertEquals(
                "P Available Android Virtual Devices:\n" +
                "P     Name: avd-for-tag1\n" +
                "P     Path: @AVD/avd-for-tag1.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: tag-1/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P ---------\n" +
                "P     Name: avd-for-tag2\n" +
                "P     Path: @AVD/avd-for-tag2.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: tag-1/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P ---------\n" +
                "P     Name: device-tag1\n" +
                "P   Device: MockDevice-tag-1 (Mock Tag 1 OEM)\n" +
                "P     Path: @AVD/device-tag1.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: tag-1/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P ---------\n" +
                "P     Name: gn-avd\n" +
                "P   Device: Galaxy Nexus (Google)\n" +
                "P     Path: @AVD/gn-avd.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P ---------\n" +
                "P     Name: gn-sdcard\n" +
                "P   Device: Galaxy Nexus (Google)\n" +
                "P     Path: @AVD/gn-sdcard.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P   Sdcard: 1023G\n" +
                "P ---------\n" +
                "P     Name: my-avd\n" +
                "P     Path: @AVD/my-avd.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P ---------\n" +
                "P     Name: my-avd2\n" +
                "P     Path: @AVD/my-avd2.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n",
                sanitizePaths(getLog().toString()));
    }

    public void testCreateAvd_Errors() throws Exception {
        expectExitOnError("E The parameter --target must be defined for action 'create avd'",
                "create", "avd",
                "--name",   "my-avd",
                "--abi",    "armeabi");

        expectExitOnError("E The parameter --name must be defined for action 'create avd'",
                "create", "avd",
                "--target", "android-0",
                "--abi",    "armeabi");

        expectExitOnError("E This platform has more than one ABI. Please specify one using --abi",
                "create", "avd",
                "--target", "android-0",
                "--name",   "my-avd");

        expectExitOnError("E Invalid --abi abi/too/long: expected format 'abi' or 'tag/abi'",
                "create", "avd",
                "--target", "android-0",
                "--name",   "my-avd",
                "--abi",    "abi/too/long");

        expectExitOnError("E --tag tag-1 conflicts with --abi other-tag/armeabi",
                "create", "avd",
                "--target", "android-0",
                "--name",   "my-avd",
                "--tag",    "tag-1",
                "--abi",    "other-tag/armeabi");

        expectExitOnError("E Invalid --tag not-a-tag for the selected target",
                "create", "avd",
                "--target", "android-0",
                "--name",   "my-avd",
                "--tag",    "not-a-tag",
                "--abi",    "armeabi");

        expectExitOnError("E Invalid --abi not-an-abi for the selected target",
                "create", "avd",
                "--target", "android-0",
                "--name",   "my-avd",
                "--tag",    "tag-1",
                "--abi",    "not-an-abi");

        expectExitOnError("E SD Card size must be in the range 9 MiB..1023 GiB",
                "create", "avd",
                "--target", "android-0",
                "--name",   "gn-sdcard",
                "--device", "0",
                "--abi",    "armeabi",
                "--sdcard", "8M");

        expectExitOnError("E SD Card size must be in the range 9 MiB..1023 GiB",
                "create", "avd",
                "--target", "android-0",
                "--name",   "gn-sdcard",
                "--device", "0",
                "--abi",    "armeabi",
                "--sdcard", "1024G");

        expectExitOnError("E 'abcd' is not recognized as a valid sdcard value",
                "create", "avd",
                "--target", "android-0",
                "--name",   "gn-sdcard",
                "--device", "0",
                "--abi",    "armeabi",
                "--sdcard", "abcd");
    }

    public void testMissingTagSysImg() throws Exception {
        // setup installed a "tag-1" system image above. Create an AVD using it.
        runCmdLine("create", "avd",
                "--target", "android-0",
                "--name",   "device-tag1",
                "--tag",    "tag-1",
                "--abi",    "armeabi",
                "--device", "MockDevice-tag-1");
        assertEquals(
                "P Created AVD 'device-tag1' based on Android 0.0, Tag 1 ARM (armeabi) processor,\n" +
                "with the following hardware config:\n" +
                "hw.accelerometer=no\n" +
                "hw.audioInput=yes\n" +
                "hw.battery=yes\n" +
                "hw.dPad=no\n" +
                "hw.device.hash2=MD5:7e046a6244489b7deeca681ab5a76cb3\n" +
                "hw.device.manufacturer=Mock Tag 1 OEM\n" +
                "hw.device.name=MockDevice-tag-1\n" +
                "hw.gps=no\n" +
                "hw.keyboard=yes\n" +
                "hw.lcd.density=240\n" +
                "hw.mainKeys=no\n" +
                "hw.sdCard=no\n" +
                "hw.sensors.orientation=no\n" +
                "hw.sensors.proximity=no\n" +
                "hw.trackBall=no\n",
                getLog().toString());

        // listing AVDs shows the new one.
        runCmdLine("list", "avd");
        assertEquals(
                "P Available Android Virtual Devices:\n" +
                "P     Name: device-tag1\n" +
                "P   Device: MockDevice-tag-1 (Mock Tag 1 OEM)\n" +
                "P     Path: @AVD/device-tag1.avd\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: tag-1/armeabi\n" +
                "P     Skin: HVGA\n",
                sanitizePaths(getLog().toString()));

        // delete the tag-1 system images (2 ABIs)
        deleteDir(mFolderSiArm);
        deleteDir(mFolderSiX86);

        // listing AVDs shows the AVD as invalid.
        runCmdLine("list", "avd");
        assertEquals(
                "P Available Android Virtual Devices:\n" +
                "P \n" +
                "The following Android Virtual Devices could not be loaded:\n" +
                "P     Name: device-tag1\n" +
                "P     Path: @AVD/device-tag1.avd\n" +
                "P    Error: Missing system image for Tag 1 armeabi Android 0.0. Run 'android update avd -n device-tag1'\n",
                sanitizePaths(getLog().toString()));

    }

    // ------ helpers

    /**
     * A test-specific implementation of {@link SdkCommandLine} that calls
     * {@link Assert#fail()} instead of {@link System#exit(int)}.
     */
    private static class ExitSdkCommandLine extends SdkCommandLine {
        public ExitSdkCommandLine(ILogger logger) {
            super(logger);
        }

        @Override
        protected void exit() {
            fail("SdkCommandLine.exit reached. Log:\n" + getLog().toString());
        }
    }

    /**
     * A test-specific implementation of {@link Main} that calls
     * {@link Assert#fail()} instead of {@link System#exit(int)}.
     */
    private class ExitMain extends Main {
        @Override
        protected void exit(int code) {
            fail("Main.exit(" + code + ") reached. Log:\n" + getLogger().toString());
        }

        @Override
        protected String readLine(byte[] buffer) throws IOException {
            String log = getLogger().toString();

            if (log.endsWith("Do you wish to create a custom hardware profile [no]")) {
                return "no";
            }

            fail("Unexpected Main.readLine call. Log:\n" + log);
            return null; // not reached
        }

        @Override
        protected AvdManager getAvdManager() throws AndroidLocationException {
            return SdkManagerTest2.this.getAvdManager();
        }
    }

    /**
     * Creates a {@link Main}, set it up with an {@code SdkManager} and a logger,
     * parses the given command-line arguments and executes the action.
     */
    private Main runCmdLine(String...args) throws Exception {
        super.createSdkAvdManagers();
        Main main = new ExitMain();
        main.setupForTest(getSdkManager(), getLog(), new ExitSdkCommandLine(getLog()), args);
        getLog().clear();
        main.doAction();
        return main;
    }

    /**
     * Used to invoke a test that should end by a fatal error (one severe enough for the
     * {@code Main} processor to just give up and call {@code exit}.)
     * <p/>
     * Invokes {@link #runCmdLine(String...)} with the given command-line arguments
     * and checks the log results to make sure the log output contains the expected string.
     */
    private void expectExitOnError(String expected, String...args) throws Exception {
        boolean failedAsExpected = false;
        boolean failedToFailed = false;
        try {
            runCmdLine(args);
            failedToFailed = true;
        } catch (AssertionFailedError e) {
            String msg = e.getMessage();
            failedAsExpected = msg.contains(expected);
        }
        if (!failedAsExpected || failedToFailed) {
            fail("Expected exit-on-error, " +
                    (failedToFailed ? "did not fail at all." : "did not fail with expected string.") +
                    "\nExpected  : " + expected +
                    "\nActual log: " + getLog().toString());
        }
    }

    /**
     * Sanitizes paths to the SDK and the AVD root folders in the log output.
     */
    private String sanitizePaths(String str) {
        if (str != null) {
            String osPath = getSdkManager().getLocation();
            str = str.replace(osPath, "@SDK");

            try {
                osPath = getAvdManager().getBaseAvdFolder();
                str = str.replace(osPath, "@AVD");
            } catch (AndroidLocationException ignore) {}

            str = str.replace(File.separatorChar, '/');
            str = str.replace(SdkConstants.mkSdCardCmdName(), "mksdcard");
        }

        return str;
    }

}
