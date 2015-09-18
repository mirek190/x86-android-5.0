/* Android AMTL
 *
 * Copyright (C) Intel 2013
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
 *
 * Author: Erwan Bracq <erwan.bracq@intel.com>
 * Author: Morgane Butscher <morganeX.butscher@intel.com>
 */

package com.intel.amtl.helper;

import com.intel.amtl.AMTLApplication;

import android.os.FileObserver;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;

public class LogManager extends FileObserver {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "LogManager";
    private final String APTRIGGER = "/logs/aplogs/aplog_trigger";
    private final String NBAPFILE = "10";
    private String tag = null;
    private String aplogPath = null;
    private String bplogPath = null;
    private String backupPath = null;
    private String snapPath = null;

    public LogManager(String bckpath, String appath, String bppath) {
        super(appath, FileObserver.ALL_EVENTS);
        aplogPath = appath;
        bplogPath = bppath;
        backupPath = bckpath;
        // check backup folder
        if (!this.checkBackupFolder())
            return;
        // right now, only aplog path needs to be monitored to get the created aplog from crash
        // logger.
    }

    public void setTag(String tag) {
        this.tag = tag;
    }

    private boolean checkBackupFolder() {
        File file = new File(this.backupPath);
        return file.exists();
    }

    private boolean requestAplog() {
        boolean ret = false;
        BufferedWriter out = null;
        try {
            out = new BufferedWriter(new FileWriter(APTRIGGER));
            out.write("APLOG=" + NBAPFILE);
            out.newLine();
            ret = true;
        } catch (IOException e) {
            Log.e(TAG, MODULE + ": Error while writing in file: " + e);
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException ex) {
                    Log.e(TAG, MODULE + ": Error during close " + ex);
                }
            }
            return ret;
        }
    }

    private void copyFile(String src, String dst)  {
        InputStream in = null;
        OutputStream out = null;
        try {

            Log.d(TAG, MODULE + ": copyFile src: " + src + " to dst: " + dst);

            in = new FileInputStream(new File(src));
            out = new FileOutputStream(new File(dst));

            byte[] buf = new byte[1024];
            int len;

            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
        } catch (IOException e) {
            Log.e(TAG, MODULE + ": COPY issue on object: " + src);
            e.printStackTrace();
        } finally {
            try {
                if (out != null) {
                    out.close();
                }
            } catch (IOException ex) {
                Log.e(TAG, MODULE + ": Error during close " + ex);
            } finally {
                try {
                    if (in != null) {
                        in.close();
                    }
                } catch (IOException ex) {
                    Log.e(TAG, MODULE + ": Error during close " + ex);
                }
            }
        }
    }

    private void doLogCopy(final String regex, String searchPath) {
        FilenameFilter filter = new FilenameFilter() {
            public boolean accept(File directory, String fileName) {
                return fileName.startsWith(regex);
            }
        };
        File src = new File(searchPath);
        File[] files = src.listFiles(filter);
        if (files != null) {
            for (File cpyfile : files) {
                this.copyFile(searchPath + cpyfile.getName(), this.snapPath + cpyfile.getName());
            }
        }
    }

    public synchronized boolean makeBackup() {
        // Create tagged snapshot folder
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd_HHmmss");
        String fdf = df.format(new Date());
        this.snapPath = backupPath + this.tag + "_" + fdf + "/";
        File file = new File(snapPath);
        if ((!file.exists()) && (!file.mkdirs())) {
            return false;
        }

        // Do bplog copy
        this.doLogCopy("bplog", this.bplogPath);

        // Activate the watcher, this allow us to get the created aplog folder by crashlogger.
        this.startWatching();

        // Request aplog snapshot to crashlogger.
        if (!this.requestAplog()) {
            return false;
        }

        return true;
    }

    @Override
    public synchronized void onEvent(int event, String path) {
        if (path == null) {
            return;
        }
        //a new file or subdirectory was created under the monitored directory
        if ((FileObserver.CREATE & event) != 0) {
            String lastEventPath = aplogPath + path + "/";
            Log.d(TAG, MODULE + ": CREATED event on object: " + lastEventPath);
            this.stopWatching();

            try {
                Thread.currentThread();
                Thread.sleep(15000);
            } catch (InterruptedException ie) {
                // Exception
                Log.d(TAG, MODULE + ": Sleep interrupted, Aplog backup may be trucated.");
            }
            this.doLogCopy("aplog", lastEventPath);
        }
    }
}
