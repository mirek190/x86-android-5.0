/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.filemanager.util;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ExpandableListActivity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Environment;
import android.os.StatFs;
import android.os.storage.StorageManager;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import android.os.Binder;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.Process;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.os.UserManager;


import com.intel.filemanager.FileMainActivity;
import com.intel.filemanager.FileManagerSettings;
import com.intel.filemanager.R;
import com.intel.filemanager.SubCategoryActivity;

/**
 * This class contains the functions that operations on file and directories.
 */
public class FileUtil {

    public static interface Defs {

        public static final String PATH_SDCARD_1 = Environment
                .getExternalStorageDirectory().getAbsolutePath();
        public static final String PATH_T2_SDCARD_2 = PATH_SDCARD_1
                + "/sdcard_ext";
        public static final String PATH_LOCAL = "/local";

        public static final String COMPRESSTYPETAG = "compress";
        public static final String UNCOMPRESSTYPETAG = "uncompress";
        public static final String OPERATIONTYPETAG = "operationtype";
        public static final String SRCFOLDERTAG = "srcparfolder";
        public static final String DESPATH = "despath";
        public static final String ISCUT = "iscut";
        public static final String AUDIOFILE = "audio";
        public static final String VIDEOFILE = "video";
        public static final String IMAGEFILE = "image";

        public static final int DELAY = 50;
        public static int OPERATIONMODE = 0;
        public static final int PASTETYPE = 0;
        public static final int DELETETYPE = 1;
        public static final int COMPRESSTYPE = 2;
        public static final int UNCOMPRESSTYPE = 3;
        public static final int OPERATIONOVER = 10;
        public static final int OPERATIONFAIL = 11;

        public static final long SDCARD_OBLIGATED_SIZE = 2 * 1024 * 1024;

        public static final int ERR_EMPTY = 1;
        public static final int ERR_INVALID_CHAR = 2;
        public static final int ERR_INVALID_INIT_CHAR = 3;
        public static final int ERR_TOO_LONG = 4;
        public static final int ERR_FOLDER_ALREADY_EXIST = 5;
        public static final int ERR_FILE_ALREADY_EXIST = 6;
        public static final int ERR_UNACCEPTABLE_CHAR = 7;
    }

    /**
     * label for logging
     */
    private static LogHelper Log = LogHelper.getLogger();
    private static final String TAG = "FileUtil";

    /**
     * The number of bytes in a kilobyte.
     */
    public static final long ONE_KB = 1024;

    /**
     * The number of bytes in a megabyte.
     */
    public static final long ONE_MB = ONE_KB * ONE_KB;

    /**
     * The number of bytes in a gigabyte.
     */
    public static final long ONE_GB = ONE_KB * ONE_MB;
    /**
     * The invalid character in file name or folder name.
     */
    public static final String invalidCharacter = "/\\%$#@,:";
    /**
     * The base path of compressed files.
     */
    public static String basePath = "/local/";

    /*
     * The invalid head of file/folder name
     */
    private static final char[] INVALIDHEAD = new char[] { '*', '#', '(', ')',
            ':', ';', '?', '!', '"', '@', '&', '<', '+', '$', '^', '>', '{',
            '=', '%', '}', '~', ',' };
    private static final int HEADLEN = 22;

    /*
     * The screen size of oms1.0
     */
    public static final int SCREEN_360X480 = 1;
    public static final int SCREEN_360X640 = 2;
    public static final int SCREEN_480X800 = 3;
    public static final int SCREEN_480X854 = 4;
    public static final int SCREEN_854X480 = 5;
    public static final int SCREEN_240X400 = 6;
    public static final int SCREEN_400X240 = 7;
    public static final int SCREEN_800X480 = 8;
    public static final int SCREEN_320X480 = 9;
    public static final int SCREEN_480X320 = 10;
    public static final int SCREEN_DEFAULT = SCREEN_360X480;
    public static int SCREEN_HEIGHT = 480;
    public static int SCREEN_WIDTH = 360;

    public static final int CATEGORY_UNKNOWN = -1;
    public static final int CATEGORY_IMAGE = 1;
    public static final int CATEGORY_VIDEO = 2;
    public static final int CATEGORY_AUDIO = 3;
    public static final int CATEGORY_DOC = 4;
    public static final int CATEGORY_APP = 5;
    public static final int CATEGORY_ARC = 6;
    public static final int CATEGORY_FAV = 7;
    public static final int CATEGORY_BOOK = 8;
    public static final int CATEGORY_APK = 9;

    public static final int OPERATION_TYPE_SHARE = 1;
    public static final int OPERATION_TYPE_COPY = 2;
    public static final int OPERATION_TYPE_CUT = 3;
    public static final int OPERATION_TYPE_PASTE = 4;
    public static final int OPERATION_TYPE_ZIP = 5;
    public static final int OPERATION_TYPE_UNZIP = 6;
    public static final int OPERATION_TYPE_DELETE = 7;
    public static final int OPERATION_TYPE_RENAME = 8;
    public static final int OPERATION_TYPE_SET_RINGTONE = 9;
    public static final int OPERATION_TYPE_SET_WALLPAPER = 10;
    public static final int OPERATION_TYPE_SHOW_PROPERTY = 11;
    public static final int OPERATION_TYPE_FAV_ADD = 12;
    public static final int OPERATION_TYPE_FAV_REMOVE = 13;
    public static final int OPERATION_TYPE_OPEN_PATH = 14;
    public static final int OPERATION_TYPE_MORE = 20;

    public static final int SORT_BY_NAME = 1;
    public static final int SORT_BY_SIZE = 2;
    public static final int SORT_BY_TIME = 3;
    public static final int SORT_BY_TYPE = 4;

    public static String PATH_SDCARD_2 = "";
    public static String PATH_SDCARD_3 = "";
    
    // -----------------------------------------------------------------------
    /**
     * Opens a {@link FileInputStream} for the specified file, providing better
     * error messages than simply calling <code>new FileInputStream(file)</code>
     * .
     * <p>
     * At the end of the method either the stream will be successfully opened,
     * or an exception will have been thrown.
     * <p>
     * An exception is thrown if the file does not exist. An exception is thrown
     * if the file object exists but is a directory. An exception is thrown if
     * the file exists but cannot be read.
     * 
     * @param file
     *            the file to open for input, must not be <code>null</code>
     * @return a new {@link FileInputStream} for the specified file
     * @throws FileNotFoundException
     *             if the file does not exist
     * @throws IOException
     *             if the file object is a directory
     * @throws IOException
     *             if the file cannot be read
     */
    public static FileInputStream openInputStream(File file) throws IOException {
        if (file.exists()) {
            if (file.isDirectory()) {
                throw new IOException("File '" + file
                        + "' exists but is a directory");
            }
            if (file.canRead() == false) {
                throw new IOException("File '" + file + "' cannot be read");
            }
        } else {
            throw new FileNotFoundException("File '" + file
                    + "' does not exist");
        }
        return new FileInputStream(file);
    }

    // -----------------------------------------------------------------------
    /**
     * get uncompressed file/directory size.
     * 
     * @param file
     *            the compressed file
     * @return size of uncompressed file/directory size. -1 indicate that we now
     *         can not count the size.
     * @throws
     */
    public static long getSizeOfUnZip(File file) throws IOException {
        long size = 0;
        if (file == null)
            throw new NullPointerException("from is null");
        if (!file.getName().toLowerCase().endsWith(".zip"))
            throw new IllegalArgumentException(
                    "zip file name must end with .zip");
        if (!file.exists())
            throw new IllegalArgumentException("from does not exist");
        ZipEntry entry;
        ZipFile zf = new ZipFile(file);
        try {
            for (Enumeration<? extends ZipEntry> e = zf.entries(); e
                    .hasMoreElements();) {
                entry = e.nextElement();
                if (null == entry)
                    throw new IOException(file.getName() + "/" + zf.getName()
                            + " is mission");
                if (!entry.isDirectory())
                    if (-1 == entry.getSize()) {
                        // Log.d(TAG, " no size : " + entry.getName());
                        size = -1;
                        break;
                    }
                size = size + entry.getSize();
                // Log.d(TAG, "entry size " + entry.getSize());

            }
        } finally {
            if (null != zf)
                zf.close();
        }
        // Log.d(TAG, "size " + size);
        return size;
    }

    public static long getNumberOfZip(File file) throws IOException {
        long size = 0;
        if (file == null)
            throw new NullPointerException("from is null");
        if (!file.getName().toLowerCase().endsWith(".zip"))
            throw new IllegalArgumentException(
                    "zip file name must end with .zip");
        if (!file.exists())
            throw new IllegalArgumentException("from does not exist");
        ZipEntry entry;
        ZipFile zf = new ZipFile(file);
        for (Enumeration<? extends ZipEntry> e = zf.entries(); e
                .hasMoreElements();) {
            entry = e.nextElement();
            if (null == entry)
                throw new IOException(file.getName() + "/" + zf.getName()
                        + " is mission");
            if (!entry.isDirectory())
                if (-1 == entry.getSize()) {
                    // Log.d(TAG, " no size : " + entry.getName());
                    size = -1;
                    break;
                }
            size = size + entry.getSize();
            // Log.d(TAG, "entry size " + entry.getSize());

        }
        // Log.d(TAG, "size " + size);
        return size;
    }

    /**
     * uncompress zip file into a directory.
     * 
     * @param from
     *            the zip file.
     * @param to
     *            the target directory. this value can be <code>null</code>, if
     *            this directory does not exist, then create it.
     * @throws IOException
     */
    public static void gUnZipFile(File from, File to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null");
        if (!from.getName().toLowerCase().endsWith(".zip"))
            throw new IllegalArgumentException(
                    "zip file name must end with .zip");
        if (!from.exists())
            throw new IllegalArgumentException("from does not exist");
        if (to == null) {
            String path = from.getAbsolutePath();
            path = path.substring(0, path.length() - 5);
            to = new File(path);
        }
        if (!to.exists()) {
            // Log.d(TAG, "to path " + to.getAbsolutePath());
            if (!to.mkdir())
                System.exit(0);
        }
        ZipInputStream zin = new ZipInputStream(new FileInputStream(from));
        ZipEntry entry;

        while ((entry = zin.getNextEntry()) != null) {
            if (entry.isDirectory()) {
                File directory = new File(to.getAbsolutePath(), entry.getName());
                if (!directory.exists()) {
                    if (!directory.mkdirs())
                        throw new IOException("can not create directory");
                }
                zin.closeEntry();
            } else {
                // File myFile = new File(entry.getName());
                FileOutputStream fout = new FileOutputStream(
                        to.getAbsolutePath() + "/" + entry.getName());
                DataOutputStream dout = new DataOutputStream(fout);
                byte[] b = new byte[1024];
                int len = 0;
                while ((len = zin.read(b)) != -1) {
                    dout.write(b, 0, len);
                }
                dout.close();
                fout.close();
                zin.closeEntry();
            }
        }
    }

    /**
     * compress multiple files one time.
     * 
     * @param from
     *            the source file array list.
     * @param to
     *            the destination zip file.
     * @throws IOException
     */
    public static void gZipMulti(Object[] from, File to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");

        // Log.d(TAG, "to file " + to.getAbsolutePath());
        FileOutputStream out = new FileOutputStream(to);
        ZipOutputStream outzip = new ZipOutputStream(out);
        basePath = to.getParentFile().getAbsolutePath();
        ZipEntry entry;
        try {
            for (Object fo : from) {
                File f = (File) fo;
                // Log.d(TAG, "zip file " + f.getAbsolutePath());
                if (f.isDirectory()) {
                    entry = new ZipEntry(f.getAbsolutePath().substring(
                            basePath.length() + 1)
                            + File.separator);
                    outzip.putNextEntry(entry);
                    gZipDirectory(f, outzip);
                } else {
                    entry = new ZipEntry(f.getAbsolutePath().substring(
                            basePath.length() + 1));
                    // Log.d(TAG, f.getAbsolutePath() + " size " + f.length());
                    entry.setSize(f.length());
                    outzip.putNextEntry(entry);
                    gZipFile(f, outzip);
                }
                // outzip.closeEntry();
            }
        } finally {
            IOUtils.closeQuietly(outzip);
        }
    }

    /**
     * compress a file or directory to gz format or zip format file.
     * <p>
     * if <code>from<code> is file, the compressed file's format is gzip,
     * If <code>from<code> is directory, the compressed files's format is zip.
     * 
     * @param from
     *            the source file or directory. can be <code>null<code>.
     * @param to
     *            the target file. If <code>from<code> is file, then
     *            <code>to<code> will be gzip format, if <code>from<code> is
     *            directory, then <code>to<code> is zip format.If this target
     *            file does not exist, it will be created. can be
     *            <code>null<code>.
     * @throws {@link IOException}
     * @throws {@link NullPointerException}
     * @throws {@link FileNotFoundException}
     */
    public static void gZip(File from, File to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        if (!from.exists())
            throw new FileNotFoundException("from does not exist");
        if (from.isDirectory())
            gZipDirectory(from, to);
        else
            gZipFile(from, to);
    }

    /**
     * compress a file to gzip format, and save the compressed file in to.
     * <p>
     * If the parameters from or to is null, IllegalArgumentException wil be
     * thrown.
     * 
     * @param from
     *            the sorce file
     *            <code>String<code> path which will be compressed. can be <code>null<code>.
     * @param to
     *            the output compressed file <code>String<code> path. can be
     *            <code>null<code>.
     * @throws IOException
     * @throws {@link NullPointerException}
     * @see {@link #gZipFile(File, File)}
     */
    public static void gZipFile(String from, String to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        gZipFile(new File(from), new File(to));
    }

    /**
     * compress a
     * <code>File<code> from to gzip format and save the compressed file in <code>File<code> to.
     * 
     * @param from
     *            the source <code>File<code> which will be compressed.can be
     *            <code>null<code>.
     * @param to
     *            the target compressed <code>File<code>. can be
     *            <code>null<code>.
     * @throws IOException
     * @throws {@link NullPointerException}
     */
    public static void gZipFile(File from, File to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");

        FileOutputStream outstream;
        ZipOutputStream out;
        FileInputStream in = openInputStream(from);
        try {
            outstream = openOutputStream(to);
            out = new ZipOutputStream(outstream);
        } catch (IOException e) {
            IOUtils.closeQuietly(in);
            throw e;
        }
        ZipEntry entry = new ZipEntry(from.getAbsolutePath().substring(
                basePath.length() + 1));
        // Log.d(TAG, from.getAbsolutePath() + " size " + from.length());
        entry.setSize(from.length());
        out.putNextEntry(entry);
        byte[] buffer = new byte[4096];
        int bytesRead;
        try {
            while ((bytesRead = in.read(buffer)) != -1)
                out.write(buffer, 0, bytesRead);
        } finally {
            IOUtils.closeQuietly(in);
            // out.closeEntry();
            IOUtils.closeQuietly(out);
        }
    }

    /**
     * 
     * @param from
     * @param to
     * @throws IOException
     */
    public static void gZipFile(File from, ZipOutputStream to)
            throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        FileInputStream in = new FileInputStream(from);

        try {
            gZipFile(in, to);
        } finally {
            IOUtils.closeQuietly(in);
        }
    }

    /**
     * 
     * @param from
     * @param to
     * @throws IOException
     * @throws {@link NullPointerException}
     */
    public static void gZipFile(FileInputStream from, ZipOutputStream to)
            throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        byte[] buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = from.read(buffer)) != -1)
            to.write(buffer, 0, bytesRead);
    }

    /**
     * Compressed the directory
     * <code>from<code> and it's contents to gzip file <code>to<code>.
     * 
     * @param from
     *            The source directory which and i'ts contents will be
     *            compressed. can be <code>null<code>.
     * @param to
     *            The target gzip compressed file. can be <code>null<code>
     * @throws IOException
     * @see {@link #gZipFile(String, String)}
     */
    public static void gZipDirectory(String from, String to) throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        gZipDirectory(new File(from), new File(to));
    }

    /**
     * Compress one directory and all it's contents files into a gzip file.
     * 
     * @param from
     *            The source directory that including it's contents will be
     *            compressed to gzip format file
     *            <code>to<code>. can be <code>null<code>.
     * @param to
     *            the target compressed gzip format file. can be
     *            <code>null<code>.
     * @throws IOException
     * @see {@link #gZipFile(File, File)}
     */
    public static void gZipDirectory(File from, File to) throws IOException {
        if (to == null)
            throw new NullPointerException("to is null.");
        ZipOutputStream out = new ZipOutputStream(openOutputStream(to));
        try {
            gZipDirectory(from, out);
        } finally {
            IOUtils.closeQuietly(out);
        }
    }

    /**
     * compress directory and all it's contents files into a gzip file.
     * 
     * @param from
     *            The source directory that including it's contents will be
     *            compressed to gzip format file
     *            <code>to<code>. can be <code>null<code>.
     * @param to
     *            the target compressed gzip format file. can be
     *            <code>null<code>.
     * @throws IOException
     * @throws {@link NullPointerException}
     * @throws {@link FileNotFoundException}
     * @author b073
     */
    public static void gZipDirectory(File from, ZipOutputStream to)
            throws IOException {
        if (from == null)
            throw new NullPointerException("from is null.");
        if (to == null)
            throw new NullPointerException("to is null");
        if (!from.exists())
            throw new FileNotFoundException("from does not exist");
        if (!from.isDirectory()) {
            throw new IllegalArgumentException("from is not directory.");
        } else {
            File[] entries = from.listFiles();
            byte[] buffer = new byte[4096];
            int bytes_read;

            for (File f : entries) {
                if (f.isDirectory()) {
                    ZipEntry entry = new ZipEntry(f.getAbsolutePath()
                            .substring(basePath.length() + 1) + File.separator);
                    to.putNextEntry(entry);
                    to.closeEntry();
                    gZipDirectory(f, to);
                } else {
                    ZipEntry fileentry = new ZipEntry(f.getAbsolutePath()
                            .substring(basePath.length() + 1));
                    // Log.d(TAG, f.getAbsolutePath() + " size " + f.length());
                    fileentry.setSize(f.length());
                    to.putNextEntry(fileentry);

                    FileInputStream in = openInputStream(f);
                    try {
                        while ((bytes_read = in.read(buffer)) != -1)
                            to.write(buffer, 0, bytes_read);
                    } finally {
                        IOUtils.closeQuietly(in);
                    }
                    // to.closeEntry();
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    /**
     * Opens a {@link FileOutputStream} for the specified file, checking and
     * creating the parent directory if it does not exist.
     * <p>
     * At the end of the method either the stream will be successfully opened,
     * or an exception will have been thrown.
     * <p>
     * The parent directory will be created if it does not exist. The file will
     * be created if it does not exist. An exception is thrown if the file
     * object exists but is a directory. An exception is thrown if the file
     * exists but cannot be written to. An exception is thrown if the parent
     * directory cannot be created.
     * 
     * @param file
     *            the file to open for output, must not be <code>null</code>
     * @return a new {@link FileOutputStream} for the specified file
     * @throws IOException
     *             if the file object is a directory
     * @throws IOException
     *             if the file cannot be written to
     * @throws IOException
     *             if a parent directory needs creating but that fails
     */
    public static FileOutputStream openOutputStream(File file)
            throws IOException {
        if (file.exists()) {
            if (file.isDirectory()) {
                throw new IOException("File '" + file
                        + "' exists but is a directory");
            }
            if (file.canWrite() == false) {
                throw new IOException("File '" + file
                        + "' cannot be written to");
            }
        } else {
            File parent = file.getParentFile();
            if (parent != null && parent.exists() == false) {
                if (parent.mkdirs() == false) {
                    throw new IOException("File '" + file
                            + "' could not be created");
                }
            }
        }
        return new FileOutputStream(file);
    }

    // ------------------------------------------------------------------------------
    /**
     * Copy/Move file/directory to another position. If the source is directory,
     * then copy or move all it's contents to the new position.If the dest does
     * not exist, create it.
     * 
     * @param dest
     *            The destination directory. Can be <code>null</code>.
     * @param source
     *            The source file or directory that is copied/moved. Can be
     *            <code>null</code>.
     * @param mCut
     *            This boolean indicates whether delete the source
     *            file/directory after copied.
     * @throws IOException
     */
    public static void copy(File dest, File source, boolean mCut)
            throws IOException {
        if (mCut)
            moveToDirectory(source, dest, false);
        else
            copyToDirectory(source, dest, false);
    }

    // -----------------------------------------------------------------------
    /**
     * Copies a file to a directory preserving the file date.
     * <p>
     * This method copies the contents of the specified source file to a file of
     * the same name in the specified destination directory. The destination
     * directory is created if it does not exist. If the destination file
     * exists, then this method will overwrite it.
     * 
     * @param srcFile
     *            an existing file to copy, must not be <code>null</code>
     * @param destDir
     *            the directory to place the copy in, must not be
     *            <code>null</code>
     * 
     * @throws NullPointerException
     *             if source or destination is null
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     * @see #copyFile(File, File, boolean)
     */
    public static void copyFileToDirectory(File srcFile, File destDir)
            throws IOException {
        copyFileToDirectory(srcFile, destDir, true);
    }

    /**
     * Copies a file to a directory optionally preserving the file date.
     * <p>
     * This method copies the contents of the specified source file to a file of
     * the same name in the specified destination directory. The destination
     * directory is created if it does not exist. If the destination file
     * exists, then this method will overwrite it.
     * 
     * @param srcFile
     *            an existing file to copy, must not be <code>null</code>
     * @param destDir
     *            the directory to place the copy in, must not be
     *            <code>null</code>
     * @param preserveFileDate
     *            true if the file date of the copy should be the same as the
     *            original
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     * @see #copyFile(File, File, boolean)
     */
    public static void copyFileToDirectory(File srcFile, File destDir,
            boolean preserveFileDate) throws IOException {
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (destDir.exists() && destDir.isDirectory() == false) {
            throw new IllegalArgumentException("Destination '" + destDir
                    + "' is not a directory");
        }
        copyFile(srcFile, new File(destDir, srcFile.getName()),
                preserveFileDate);
    }

    /**
     * Copies a file to a new location preserving the file date.
     * <p>
     * This method copies the contents of the specified source file to the
     * specified destination file. The directory holding the destination file is
     * created if it does not exist. If the destination file exists, then this
     * method will overwrite it.
     * 
     * @param srcFile
     *            an existing file to copy, must not be <code>null</code>
     * @param destFile
     *            the new file, must not be <code>null</code>
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     * @see #copyFileToDirectory(File, File)
     */
    public static void copyFile(File srcFile, File destFile) throws IOException {
        copyFile(srcFile, destFile, true);
    }

    /**
     * Copies a file to a new location.
     * <p>
     * This method copies the contents of the specified source file to the
     * specified destination file. The directory holding the destination file is
     * created if it does not exist. If the destination file exists, then this
     * method will overwrite it.
     * 
     * @param srcFile
     *            an existing file to copy, must not be <code>null</code>
     * @param destFile
     *            the new file, must not be <code>null</code>
     * @param preserveFileDate
     *            true if the file date of the copy should be the same as the
     *            original
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     * @see #copyFileToDirectory(File, File, boolean)
     */
    public static void copyFile(File srcFile, File destFile,
            boolean preserveFileDate) throws IOException {
        if (srcFile == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destFile == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (srcFile.exists() == false) {
            throw new FileNotFoundException("Source '" + srcFile
                    + "' does not exist");
        }
        if (srcFile.isDirectory()) {
            throw new IOException("Source '" + srcFile
                    + "' exists but is a directory");
        }
        if (srcFile.getCanonicalPath().equals(destFile.getCanonicalPath())) {
            throw new IOException("Source '" + srcFile + "' and destination '"
                    + destFile + "' are the same");
        }
        if (destFile.getParentFile() != null
                && destFile.getParentFile().exists() == false) {
            if (destFile.getParentFile().mkdirs() == false) {
                throw new IOException("Destination '" + destFile
                        + "' directory cannot be created");
            }
        }
        if (destFile.exists() && destFile.canWrite() == false) {
            throw new IOException("Destination '" + destFile
                    + "' exists but is read-only");
        }
        doCopyFile(srcFile, destFile, preserveFileDate);
    }

    /**
     * Internal copy file method.
     * 
     * @param srcFile
     *            the validated source file, must not be <code>null</code>
     * @param destFile
     *            the validated destination file, must not be <code>null</code>
     * @param preserveFileDate
     *            whether to preserve the file date
     * @throws IOException
     *             if an error occurs
     */
    private static void doCopyFile(File srcFile, File destFile,
            boolean preserveFileDate) throws IOException {
        if (destFile.exists() && destFile.isDirectory()) {
            throw new IOException("Destination '" + destFile
                    + "' exists but is a directory");
        }

        FileInputStream input = openInputStream(srcFile);
        try {
            FileOutputStream output = openOutputStream(destFile);
            try {
                IOUtils.copyStream(input, output);
            } finally {
                IOUtils.closeQuietly(output);
            }
        } finally {
            IOUtils.closeQuietly(input);
        }

        if (srcFile.length() != destFile.length()) {
            throw new IOException("Failed to copy full contents from '"
                    + srcFile + "' to '" + destFile + "'");
        }
        if (preserveFileDate) {
            destFile.setLastModified(srcFile.lastModified());
        }
    }

    // -----------------------------------------------------------------------
    /**
     * copy a file or directory to the destination directory.
     * 
     * @param src
     *            the file or directory to be moved
     * @param destDir
     *            the destination directory
     * @param createDestDir
     *            If <code>true</code> create the destination directory,
     *            otherwise if <code>false</code> throw an IOException
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     */
    public static void copyToDirectory(File src, File destDir,
            boolean createDestDir) throws IOException {
        if (src == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (!src.exists()) {
            throw new FileNotFoundException("Source '" + src
                    + "' does not exist");
        }
        if (src.isDirectory()) {
            copyDirectoryToDirectory(src, destDir);
        } else {
            copyFileToDirectory(src, destDir, createDestDir);
        }
    }

    /**
     * Copies a directory to within another directory preserving the file dates.
     * <p>
     * This method copies the source directory and all its contents to a
     * directory of the same name in the specified destination directory.
     * <p>
     * The destination directory is created if it does not exist. If the
     * destination directory did exist, then this method merges the source with
     * the destination, with the source taking precedence.
     * 
     * @param srcDir
     *            an existing directory to copy, must not be <code>null</code>
     * @param destDir
     *            the directory to place the copy in, must not be
     *            <code>null</code>
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     */
    public static void copyDirectoryToDirectory(File srcDir, File destDir)
            throws IOException {
        if (srcDir == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (srcDir.exists() && srcDir.isDirectory() == false) {
            throw new IllegalArgumentException("Source '" + destDir
                    + "' is not a directory");
        }
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (destDir.exists() && destDir.isDirectory() == false) {
            throw new IllegalArgumentException("Destination '" + destDir
                    + "' is not a directory");
        }
        copyDirectory(srcDir, new File(destDir, srcDir.getName()), true);
    }

    /**
     * Copies a whole directory to a new location preserving the file dates.
     * <p>
     * This method copies the specified directory and all its child directories
     * and files to the specified destination. The destination is the new
     * location and name of the directory.
     * <p>
     * The destination directory is created if it does not exist. If the
     * destination directory did exist, then this method merges the source with
     * the destination, with the source taking precedence.
     * 
     * @param srcDir
     *            an existing directory to copy, must not be <code>null</code>
     * @param destDir
     *            the new directory, must not be <code>null</code>
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     */
    public static void copyDirectory(File srcDir, File destDir)
            throws IOException {
        copyDirectory(srcDir, destDir, true);
    }

    /**
     * Copies a whole directory to a new location.
     * <p>
     * This method copies the contents of the specified source directory to
     * within the specified destination directory.
     * <p>
     * The destination directory is created if it does not exist. If the
     * destination directory did exist, then this method merges the source with
     * the destination, with the source taking precedence.
     * 
     * @param srcDir
     *            an existing directory to copy, must not be <code>null</code>
     * @param destDir
     *            the new directory, must not be <code>null</code>
     * @param preserveFileDate
     *            true if the file date of the copy should be the same as the
     *            original
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     */
    public static void copyDirectory(File srcDir, File destDir,
            boolean preserveFileDate) throws IOException {
        copyDirectory(srcDir, destDir, null, preserveFileDate);
    }

    /**
     * Copies a filtered directory to a new location preserving the file dates.
     * <p>
     * This method copies the contents of the specified source directory to
     * within the specified destination directory.
     * <p>
     * The destination directory is created if it does not exist. If the
     * destination directory did exist, then this method merges the source with
     * the destination, with the source taking precedence.
     * 
     * <h4>Example: Copy directories only</h4>
     * 
     * <pre>
     * // only copy the directory structure
     * FileUtils.copyDirectory(srcDir, destDir, DirectoryFileFilter.DIRECTORY);
     * </pre>
     * 
     * <h4>Example: Copy directories and txt files</h4>
     * 
     * <pre>
     * // Create a filter for &quot;.txt&quot; files
     * IOFileFilter txtSuffixFilter = FileFilterUtils.suffixFileFilter(&quot;.txt&quot;);
     * IOFileFilter txtFiles = FileFilterUtils.andFileFilter(FileFileFilter.FILE,
     *         txtSuffixFilter);
     * 
     * // Create a filter for either directories or &quot;.txt&quot; files
     * FileFilter filter = FileFilterUtils.orFileFilter(DirectoryFileFilter.DIRECTORY,
     *         txtFiles);
     * 
     * // Copy using the filter
     * FileUtils.copyDirectory(srcDir, destDir, filter);
     * </pre>
     * 
     * @param srcDir
     *            an existing directory to copy, must not be <code>null</code>
     * @param destDir
     *            the new directory, must not be <code>null</code>
     * @param filter
     *            the filter to apply, null means copy all directories and files
     *            should be the same as the original
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     */
    public static void copyDirectory(File srcDir, File destDir,
            FileFilter filter) throws IOException {
        copyDirectory(srcDir, destDir, filter, true);
    }

    /**
     * Copies a filtered directory to a new location.
     * <p>
     * This method copies the contents of the specified source directory to
     * within the specified destination directory.
     * <p>
     * The destination directory is created if it does not exist. If the
     * destination directory did exist, then this method merges the source with
     * the destination, with the source taking precedence.
     * 
     * <h4>Example: Copy directories only</h4>
     * 
     * <pre>
     * // only copy the directory structure
     * FileUtils.copyDirectory(srcDir, destDir, DirectoryFileFilter.DIRECTORY, false);
     * </pre>
     * 
     * <h4>Example: Copy directories and txt files</h4>
     * 
     * <pre>
     * // Create a filter for &quot;.txt&quot; files
     * IOFileFilter txtSuffixFilter = FileFilterUtils.suffixFileFilter(&quot;.txt&quot;);
     * IOFileFilter txtFiles = FileFilterUtils.andFileFilter(FileFileFilter.FILE,
     *         txtSuffixFilter);
     * 
     * // Create a filter for either directories or &quot;.txt&quot; files
     * FileFilter filter = FileFilterUtils.orFileFilter(DirectoryFileFilter.DIRECTORY,
     *         txtFiles);
     * 
     * // Copy using the filter
     * FileUtils.copyDirectory(srcDir, destDir, filter, false);
     * </pre>
     * 
     * @param srcDir
     *            an existing directory to copy, must not be <code>null</code>
     * @param destDir
     *            the new directory, must not be <code>null</code>
     * @param filter
     *            the filter to apply, null means copy all directories and files
     * @param preserveFileDate
     *            true if the file date of the copy should be the same as the
     *            original
     * 
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs during copying
     */
    public static void copyDirectory(File srcDir, File destDir,
            FileFilter filter, boolean preserveFileDate) throws IOException {
        if (srcDir == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (srcDir.exists() == false) {
            throw new FileNotFoundException("Source '" + srcDir
                    + "' does not exist");
        }
        if (srcDir.isDirectory() == false) {
            throw new IOException("Source '" + srcDir
                    + "' exists but is not a directory");
        }
        if (srcDir.getCanonicalPath().equals(destDir.getCanonicalPath())) {
            throw new IOException("Source '" + srcDir + "' and destination '"
                    + destDir + "' are the same");
        }

        // Cater for destination being directory within the source directory
        List exclusionList = null;
        if (destDir.getCanonicalPath().startsWith(srcDir.getCanonicalPath())) {
            File[] srcFiles = filter == null ? srcDir.listFiles() : srcDir
                    .listFiles(filter);
            if (srcFiles != null && srcFiles.length > 0) {
                exclusionList = new ArrayList(srcFiles.length);
                for (int i = 0; i < srcFiles.length; i++) {
                    File copiedFile = new File(destDir, srcFiles[i].getName());
                    exclusionList.add(copiedFile.getCanonicalPath());
                }
            }
        }
        doCopyDirectory(srcDir, destDir, filter, preserveFileDate,
                exclusionList);
    }

    /**
     * Internal copy directory method.
     * 
     * @param srcDir
     *            the validated source directory, must not be <code>null</code>
     * @param destDir
     *            the validated destination directory, must not be
     *            <code>null</code>
     * @param filter
     *            the filter to apply, null means copy all directories and files
     * @param preserveFileDate
     *            whether to preserve the file date
     * @param exclusionList
     *            List of files and directories to exclude from the copy, may be
     *            null
     * @throws IOException
     *             if an error occurs
     */
    private static void doCopyDirectory(File srcDir, File destDir,
            FileFilter filter, boolean preserveFileDate, List exclusionList)
            throws IOException {
        if (destDir.exists()) {
            if (destDir.isDirectory() == false) {
                throw new IOException("Destination '" + destDir
                        + "' exists but is not a directory");
            }
        } else {
            if (destDir.mkdirs() == false) {
                throw new IOException("Destination '" + destDir
                        + "' directory cannot be created");
            }
            if (preserveFileDate) {
                destDir.setLastModified(srcDir.lastModified());
            }
        }
        if (destDir.canWrite() == false) {
            throw new IOException("Destination '" + destDir
                    + "' cannot be written to");
        }
        // recurse
        File[] files = filter == null ? srcDir.listFiles() : srcDir
                .listFiles(filter);
        if (files == null) { // null if security restricted
            throw new IOException("Failed to list contents of " + srcDir);
        }
        for (int i = 0; i < files.length; i++) {
            File copiedFile = new File(destDir, files[i].getName());
            if (exclusionList == null
                    || !exclusionList.contains(files[i].getCanonicalPath())) {
                if (files[i].isDirectory()) {
                    doCopyDirectory(files[i], copiedFile, filter,
                            preserveFileDate, exclusionList);
                } else {
                    doCopyFile(files[i], copiedFile, preserveFileDate);
                }
            }
        }
    }

    // ----------------------------------------------------------------------------------
    /**
     * Moves a directory.
     * <p>
     * When the destination directory is on another file system, do a
     * "copy and delete".
     * 
     * @param srcDir
     *            the directory to be moved
     * @param destDir
     *            the destination directory
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     */
    public static void moveDirectory(File srcDir, File destDir)
            throws IOException {
        if (srcDir == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (!srcDir.exists()) {
            throw new FileNotFoundException("Source '" + srcDir
                    + "' does not exist");
        }
        if (!srcDir.isDirectory()) {
            throw new IOException("Source '" + srcDir + "' is not a directory");
        }
        if (destDir.exists()) {
            throw new IOException("Destination '" + destDir
                    + "' already exists");
        }
        boolean rename = srcDir.renameTo(destDir);
        if (!rename) {
            copyDirectory(srcDir, destDir);
            deleteDirectory(srcDir);
            if (srcDir.exists()) {
                throw new IOException("Failed to delete original directory '"
                        + srcDir + "' after copy to '" + destDir + "'");
            }
        }
    }

    /**
     * Moves a directory to another directory.
     * 
     * @param src
     *            the file to be moved
     * @param destDir
     *            the destination file
     * @param createDestDir
     *            If <code>true</code> create the destination directory,
     *            otherwise if <code>false</code> throw an IOException
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     */
    public static void moveDirectoryToDirectory(File src, File destDir,
            boolean createDestDir) throws IOException {
        if (src == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException(
                    "Destination directory must not be null");
        }
        if (!destDir.exists() && createDestDir) {
            destDir.mkdirs();
        }
        if (!destDir.exists()) {
            throw new FileNotFoundException("Destination directory '" + destDir
                    + "' does not exist [createDestDir=" + createDestDir + "]");
        }
        if (!destDir.isDirectory()) {
            throw new IOException("Destination '" + destDir
                    + "' is not a directory");
        }
        moveDirectory(src, new File(destDir, src.getName()));

    }

    /**
     * Moves a file.
     * <p>
     * When the destination file is on another file system, do a
     * "copy and delete".
     * 
     * @param srcFile
     *            the file to be moved
     * @param destFile
     *            the destination file
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     * @throws IOException
     *             if the destination already exists.
     */
    public static void moveFile(File srcFile, File destFile) throws IOException {
        if (srcFile == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destFile == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (!srcFile.exists()) {
            throw new FileNotFoundException("Source '" + srcFile
                    + "' does not exist");
        }
        if (srcFile.isDirectory()) {
            throw new IOException("Source '" + srcFile + "' is a directory");
        }
        if (destFile.exists()) {
            throw new IOException("Destination '" + destFile
                    + "' already exists");
        }
        if (destFile.isDirectory()) {
            throw new IOException("Destination '" + destFile
                    + "' is a directory");
        }
        boolean rename = srcFile.renameTo(destFile);
        if (!rename) {
            copyFile(srcFile, destFile);
            if (!srcFile.delete()) {
                deleteQuietly(destFile);
                throw new IOException("Failed to delete original file '"
                        + srcFile + "' after copy to '" + destFile + "'");
            }
        }
    }

    /**
     * Moves a file to a directory.
     * 
     * @param srcFile
     *            the file to be moved
     * @param destDir
     *            the destination file, this directory can not contain the file
     *            with the same name with <code>srcFile<code>}
     * @param createDestDir
     *            If <code>true</code> create the destination directory,
     *            otherwise if <code>false</code> throw an IOException
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     */
    public static void moveFileToDirectory(File srcFile, File destDir,
            boolean createDestDir) throws IOException {
        if (srcFile == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException(
                    "Destination directory must not be null");
        }
        if (!destDir.exists() && createDestDir) {
            destDir.mkdirs();
        }
        if (!destDir.exists()) {
            throw new FileNotFoundException("Destination directory '" + destDir
                    + "' does not exist [createDestDir=" + createDestDir + "]");
        }
        if (!destDir.isDirectory()) {
            throw new IOException("Destination '" + destDir
                    + "' is not a directory");
        }
        moveFile(srcFile, new File(destDir, srcFile.getName()));
    }

    /**
     * Moves a file or directory to the destination directory.
     * <p>
     * When the destination is on another file system, do a "copy and delete".
     * 
     * @param src
     *            the file or directory to be moved
     * @param destDir
     *            the destination directory
     * @param createDestDir
     *            If <code>true</code> create the destination directory,
     *            otherwise if <code>false</code> throw an IOException
     * @throws NullPointerException
     *             if source or destination is <code>null</code>
     * @throws IOException
     *             if source or destination is invalid
     * @throws IOException
     *             if an IO error occurs moving the file
     */
    public static void moveToDirectory(File src, File destDir,
            boolean createDestDir) throws IOException {
        if (src == null) {
            throw new NullPointerException("Source must not be null");
        }
        if (destDir == null) {
            throw new NullPointerException("Destination must not be null");
        }
        if (!src.exists()) {
            throw new FileNotFoundException("Source '" + src
                    + "' does not exist");
        }
        if (src.isDirectory()) {
            moveDirectoryToDirectory(src, destDir, createDestDir);
        } else {
            moveFileToDirectory(src, destDir, createDestDir);
        }
    }

    // -----------------------------------------------------------------------
    /**
     * Deletes a directory recursively.
     * 
     * @param directory
     *            directory to delete
     * @throws IOException
     *             in case deletion is unsuccessful
     */
    public static void deleteDirectory(File directory) throws IOException {
        if (!directory.exists()) {
            return;
        }

        cleanDirectory(directory);
        if (!directory.delete()) {
            String message = "Unable to delete directory " + directory + ".";
            throw new IOException(message);
        }
    }

    /**
     * Deletes a file, never throwing an exception. If file is a directory,
     * delete it and all sub-directories.
     * <p>
     * The difference between File.delete() and this method are:
     * <ul>
     * <li>A directory to be deleted does not have to be empty.</li>
     * <li>No exceptions are thrown when a file or directory cannot be deleted.</li>
     * </ul>
     * 
     * @param file
     *            file or directory to delete, can be <code>null</code>
     * @return <code>true</code> if the file or directory was deleted, otherwise
     *         <code>false</code>
     * 
     */
    public static boolean deleteQuietly(File file) {
        if (file == null) {
            return false;
        }
        try {
            if (file.isDirectory()) {
                cleanDirectory(file);
            }
        } catch (Exception e) {
            // can not clean directory.
        }

        try {
            return file.delete();
        } catch (Exception e) {
            return false;
        }
    }

    public static boolean deleteFiles(File file) {
        if (!file.exists()) {
            return false;
        }
        if (file.isFile()) {
            file.delete();
        } else if (file.isDirectory()) {
            File[] files = file.listFiles();
            for (int i = 0; i < files.length; i++) {
                deleteFiles(files[i]);
            }
            file.delete();
        }
        return true;
    }

    /**
     * Cleans a directory without deleting it.
     * 
     * @param directory
     *            directory to clean
     * @throws IOException
     *             in case cleaning is unsuccessful
     */
    public static void cleanDirectory(File directory) throws IOException {
        if (!directory.exists()) {
            String message = directory + " does not exist";
            throw new FileNotFoundException(message);
        }

        if (!directory.isDirectory()) {
            String message = directory + " is not a directory";
            throw new IllegalArgumentException(message);
        }

        File[] files = directory.listFiles();
        if (files == null) { // null if security restricted
            throw new IOException("Failed to list contents of " + directory);
        }

        IOException exception = null;
        for (int i = 0; i < files.length; i++) {
            File file = files[i];
            try {
                forceDelete(file);
            } catch (IOException ioe) {
                exception = ioe;
            }
        }

        if (null != exception) {
            throw exception;
        }
    }

    // -----------------------------------------------------------------------
    /**
     * Deletes a file. If file is a directory, delete it and all
     * sub-directories.
     * <p>
     * The difference between File.delete() and this method are:
     * <ul>
     * <li>A directory to be deleted does not have to be empty.</li>
     * <li>You get exceptions when a file or directory cannot be deleted.
     * (java.io.File methods returns a boolean)</li>
     * </ul>
     * 
     * @param file
     *            file or directory to delete, must not be <code>null</code>
     * @throws NullPointerException
     *             if the directory is <code>null</code>
     * @throws FileNotFoundException
     *             if the file was not found
     * @throws IOException
     *             in case deletion is unsuccessful
     */
    public static void forceDelete(File file) throws IOException {
        if (file.isDirectory()) {
            deleteDirectory(file);
        } else {
            boolean filePresent = file.exists();
            if (!file.delete()) {
                if (!filePresent) {
                    throw new FileNotFoundException("File does not exist: "
                            + file);
                }
                String message = "Unable to delete file: " + file;
                throw new IOException(message);
            }
        }
    }

    /**
     * Makes a directory, including any necessary but nonexistent parent
     * directories. If there already exists a file with specified name or the
     * directory cannot be created then an exception is thrown.
     * 
     * @param directory
     *            directory to create, must not be <code>null</code>
     * @throws NullPointerException
     *             if the directory is <code>null</code>
     * @throws IOException
     *             if the directory cannot be created
     */
    public static void forceMkdir(File directory) throws IOException {
        if (directory.exists()) {
            if (directory.isFile()) {
                String message = "File " + directory + " exists and is "
                        + "not a directory. Unable to create directory.";
                throw new IOException(message);
            }
        } else {
            if (!directory.mkdirs()) {
                String message = "Unable to create directory " + directory;
                throw new IOException(message);
            }
        }
    }

    // -----------------------------------------------------------------------
    /**
     * Returns a human-readable version of the file size, where the input
     * represents a specific number of bytes.
     * 
     * @param size
     *            the number of bytes
     * @return a human-readable display value (includes units)
     */
    public static String byteCountToDisplaySize(long size) {
        String displaySize;

        if (size / ONE_GB > 0) {
            displaySize = String.valueOf(size / ONE_GB) + " GB";
        } else if (size / ONE_MB > 0) {
            displaySize = String.valueOf(size / ONE_MB) + " MB";
        } else if (size / ONE_KB > 0) {
            displaySize = String.valueOf(size / ONE_KB) + " KB";
        } else {
            displaySize = String.valueOf(size) + " bytes";
        }
        return displaySize;
    }

    /**
     * This api is used to sum size of all files in a directory or size of a
     * file. Later this will be implemented as native.
     * 
     * @param file
     *            directory or file to inspect. must not be <code>null</code>
     * @return size of folder.
     */
    public static long getSize(File file) {
        long folderSize = 0;
        if (!file.exists()) {
            String message = file + " does not exist";
            throw new IllegalArgumentException(message);
        }
        if (file.isDirectory()) {
            folderSize = sizeOfDirectory(file);
        } else
            folderSize = file.length();
        return folderSize;
    }

    /**
     * This api is used to sum size of all files in the array.
     * 
     * @param files
     *            files array.
     * @return size of files.
     */
    public static long getSize(Object[] files) {
        long size = 0;
        if (files == null)
            throw new NullPointerException("from is null.");
        for (Object file : files) {
            File f = (File) file;
            Log.d(TAG, "zip file " + f.getAbsolutePath());
            size += getSize(f);
        }
        return size;
    }

    // -----------------------------------------------------------------------
    /**
     * Counts the size of a directory recursively (sum of the length of all
     * files).
     * 
     * @param directory
     *            directory to inspect, must not be <code>null</code>
     * @return size of directory in bytes, 0 if directory is security restricted
     * @throws NullPointerException
     *             if the directory is <code>null</code>
     */
    public static long sizeOfDirectory(File directory) {
        if (!directory.exists()) {
            String message = directory + " does not exist";
            throw new IllegalArgumentException(message);
        }

        if (!directory.isDirectory()) {
            String message = directory + " is not a directory";
            throw new IllegalArgumentException(message);
        }

        long size = 0;

        File[] files = directory.listFiles();
        if (files == null) { // null if security restricted
            return 0L;
        }
        for (int i = 0; i < files.length; i++) {
            File file = files[i];

            if (file.isDirectory()) {
                size += sizeOfDirectory(file);
            } else {
                size += file.length();
            }
        }
        // Log.d(TAG, "folder size " + size);
        return size;
    }

    /**
     * verify whether one string is a directory path name. now only check if it
     * starts with ".".
     * 
     * @param directory
     * @return true if the string is a directory path name. else false;
     */
    public static boolean isValidDirName(String directory) {
        return (!directory.contains(".") && isValidName(directory));

    }

    public static boolean isValidName(String name) {
        // Log.d(TAG, "entering isValidName " + name);
        int len = invalidCharacter.length();
        for (int i = 0; i < len; i++) {
            // Log.d(TAG, "check character " +
            // invalidCharacter.substring(i,i+1));
            if (name.contains(invalidCharacter.substring(i, i + 1))) {
                // Log.d(TAG, "contains invalid character.");
                return false;
            }
        }
        return true;
    }

    public static boolean isForwardLocked(String path) {
        return path.endsWith(".dm");
    }

    public static boolean isForwardLocked(File file) {
        return file.getName().endsWith(".dm");
    }

    /*
     * Check if the file/folder name's head is invalid return: boolean true: the
     * head of the name if VALID false : the head of the name is INVALID
     * 
     * Notice: suggest calling this function after fixFileName()
     */
    public static boolean isValidHead(String fileName) {

        char head = '.';
        if (null == fileName || fileName.length() == 0) {
            Log.d(TAG, "in isValidHead, the fileName is invalid");
            return false;
        }

        head = fileName.charAt(0);

        // for (int i = 0; i < HEADLEN; i++) {
        // if (head == INVALIDHEAD[i]) {
        // return false;
        // }
        // }
        if (!Character.isLetterOrDigit(head))
            return false;

        return true;
    }

    /*
     * Fix the name ends with space and ".", and starts with space
     */
    public static String fixFileName(String filename) {
        String fixedName = null;
        if (null == filename || filename.trim().length() == 0) {
            return fixedName;
        }

        if (!filename.endsWith(" ") && !filename.endsWith(".")
                && !filename.startsWith(" ")) {
            return filename;
        }

        if (filename.startsWith(" ")) {
            fixedName = filename.substring(1, filename.length());
        }
        if (filename.endsWith(" ") || filename.endsWith(".")) {
            fixedName = filename.substring(0, filename.length() - 1);
        }
        if (fixedName.endsWith(" ") || fixedName.endsWith(".")
                || filename.startsWith(" ")) {
            fixedName = fixFileName(fixedName);
        }

        return fixedName;
    }

    /*
     * Get the screen size.
     */
    public static int getScreenSize(Context context) {
        int height = SCREEN_HEIGHT;
        int width = SCREEN_WIDTH;

        if (null == context) {
            Log.d(TAG, "The param for getScreenSize() is invalid!");
            return SCREEN_DEFAULT;
        }

        WindowManager wm = (WindowManager) context
                .getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        height = display.getHeight();
        width = display.getWidth();
        Log.v(TAG, "height = " + height);
        Log.v(TAG, "width = " + width);
        if (height == 480 && width == 360) {
            return SCREEN_360X480;
        } else if (height == 320 && width == 480) {
            return SCREEN_480X320;
        } else if (height == 480 && width == 320) {
            return SCREEN_320X480;
        } else if (height == 640 && width == 360) {
            return SCREEN_360X640;
        } else if (height == 800 && width == 480) {
            return SCREEN_480X800;
        } else if (height == 854 && width == 480) {
            return SCREEN_480X854;
        } else if (height == 480 && width == 854) {
            return SCREEN_854X480;
        } else if (height == 400 && width == 240) {
            return SCREEN_240X400;
        } else if (height == 240 && width == 400) {
            return SCREEN_400X240;
        } else if (height == 480 && width == 800) {
            return SCREEN_800X480;
        }

        return SCREEN_DEFAULT;
    }

    public static void displayDatabaseError(Activity a) {
        String status = Environment.getExternalStorageState();
        int title = R.string.sdcard_missing_message;
        int message = R.string.sdcard_missing_message;

        if (status.equals(Environment.MEDIA_SHARED)) {
            // title = R.string.sdcard_busy_title;
            message = R.string.sdcard_busy_message;
        } else if (status.equals(Environment.MEDIA_REMOVED)) {
            // title = R.string.sdcard_missing_title;
            message = R.string.sdcard_missing_message;
        } else if (status.equals(Environment.MEDIA_MOUNTED)) {
            // The card is mounted, but we didn't get a valid cursor.
            // This probably means the mediascanner hasn't started scanning the
            // card yet (there is a small window of time during boot where this
            // will happen).
            a.setTitle("");
            // Intent intent = new Intent();
            // intent.setClass(a, ScanningProgress.class);
            // a.startActivityForResult(intent, Defs.SCAN_DONE);
        } else {
            Log.d(TAG, "sd card: " + status);
        }

        // title = R.string.title_for_no_sdcard;

        a.setTitle(title);
        if (a instanceof ExpandableListActivity) {
            a.setContentView(R.layout.no_sd_card_expanding);
        } else {
            a.setContentView(R.layout.no_sd_card);
        }
        TextView tv = (TextView) a.findViewById(R.id.sd_message);
        tv.setText(message);
    }

    public static int getIconByMimeType(String mimeType) {
        return MediaFile.getIconForMimeType(mimeType);
    }

    public static boolean checkSpaceRestriction(Context context, String path,
            long size) {
        long diskfree = FileUtil.getDiskFreeSpace(context, path);
        if (diskfree < size
                || (path.startsWith(Defs.PATH_SDCARD_1) && diskfree < Defs.SDCARD_OBLIGATED_SIZE)) {
            Log.v(TAG, "diskfree ==== " + diskfree);
            return false;
        }

        return true;
    }

    /*
     * function : for the struct statfs works incorrectly, use the command "df"
     * to get the free space of disk
     */
    public static long getDiskFreeSpace(Context context, String filePath) {
        if (null == context || null == filePath) {
            Log.d(TAG, "The param for getDiskFreeSpace is invalid!");
            return 0;
        }

        String path = "sdcard";
        if (filePath.startsWith(Defs.PATH_LOCAL)) {
            path = "local";
        } else if (has2ndCard(context)
                && filePath.startsWith(PATH_SDCARD_2)) {
            path = PATH_SDCARD_2;
        } else if (has3thCard(context)
                && filePath.startsWith(PATH_SDCARD_3)) {
            path = PATH_SDCARD_3;
        }
        String status = Environment.getExternalStorageState();

        if (path.equals("sdcard")
                && !(status.equals(Environment.MEDIA_MOUNTED))) {
            try {
                String str = context.getText(R.string.error_sd_readonly)
                        .toString();
                Toast.makeText(context, str, Toast.LENGTH_SHORT).show();
            } catch (Exception ex) {
                Log.e(TAG, "showToast(int resid) Exception : " + ex.toString());
            }
            return 0;
        }
        Long freesize = Long.MIN_VALUE;
        try {
            Log.d(TAG, "entering get free space by StatFs() ...");
            StatFs stat = new StatFs(path);
            long blockSize = stat.getBlockSize();
            long totalBlocks = stat.getBlockCount();
            long availableBlocks = stat.getAvailableBlocks();
            freesize = blockSize * availableBlocks;
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(TAG, "ERROR:" + e);
            return 0;
        }

        /*
         * Process processOnLinux = null; try{ processOnLinux =
         * Runtime.getRuntime().exec("df " + path); }catch(Exception e){
         * Log.e(TAG,"exp "+e); } InputStream reader = new
         * BufferedInputStream(processOnLinux.getInputStream() ); StringBuffer
         * buffer = new StringBuffer(); for (;;) { int charCount=0; try{
         * charCount = reader.read(); }catch(IOException e){
         * Log.e(TAG,"EXP "+e); } if (charCount == -1) break;
         * buffer.append((char) charCount); } String outputText =
         * buffer.toString(); try{ reader.close(); }catch(IOException e){
         * 
         * } // parse the output text for the bytes free info Long freesize =
         * Long.MIN_VALUE; StringTokenizer tokenizer = new
         * StringTokenizer(outputText, "\n"); if (tokenizer.hasMoreTokens()) {
         * String line = tokenizer.nextToken(); StringTokenizer tokenizerSecond
         * = new StringTokenizer(line, " "); if (tokenizerSecond.countTokens()
         * >= 7) { tokenizerSecond.nextToken(); tokenizerSecond.nextToken();
         * tokenizerSecond.nextToken(); tokenizerSecond.nextToken();
         * tokenizerSecond.nextToken(); String free =
         * tokenizerSecond.nextToken(); free =
         * free.substring(0,free.length()-1); freesize = Long.parseLong(free);
         * freesize = freesize << 10; } }
         */
        return freesize;
    }

    public static long getFreeSpace(Context context, String path) {
        if (null == path) {
            return 0;
        }
        long freespace = 0;

        String externalRoot = Environment.getExternalStorageDirectory()
                .getAbsolutePath();
        String root = path.startsWith(externalRoot) ? externalRoot : "/local";
        if (getMountedSDCardCount(context) > 1 ) {
             if(path.startsWith(PATH_SDCARD_2)) {
                root = PATH_SDCARD_2;
            } else if (path.startsWith(PATH_SDCARD_3)) {
                root = PATH_SDCARD_3;
            }
        }
        // android not support two sdcard, to support it, some system define 
        // the second sdcard path by themselves, this may lead to some path is invalid
        long avilableSize = 0;
        try {
            StatFs stat = new StatFs(root);
            int availableBlock = stat.getAvailableBlocks();
            int blockSize = stat.getBlockSize();
            avilableSize = availableBlock * blockSize;
        } catch (IllegalArgumentException e){
            Log.d(TAG, "the path is invalid");
        }

        return avilableSize;
    }

    public static boolean has2ndCard(Context context) {
        return getMountedSDCardCount(context) == 2;
    }

    public static boolean has3thCard(Context context) {
        return getMountedSDCardCount(context) == 3;
    }

    public static boolean isInternalMaster() {
        boolean isInternalMaster = true;
        String primaryStr = "0,-1";
        Class sysProp;
        try {
            sysProp = Class.forName("android.os.SystemProperties");
            Method get = sysProp.getMethod("get", String.class);
            primaryStr = (String) get.invoke(null, "sys.primary.sdcard");
            if (!TextUtils.isEmpty(primaryStr) && primaryStr.charAt(0) == '1') {
                isInternalMaster = false;
            }
        } catch (Exception e) {
            Log.e(TAG, "when get system properties, get error=" + e.toString());
        }
        return isInternalMaster;
    }

    public static boolean isMounted(Context context, String path) {
        if (TextUtils.isEmpty(path))
            return false;
        
        StorageManager storageManager = (StorageManager) context
                .getSystemService(Context.STORAGE_SERVICE);
        Method method;
        Object obj;
        boolean mounted = false;
        try {
            method = storageManager.getClass().getMethod("getVolumeState",
                    new Class[] { String.class });
            obj = method.invoke(storageManager, new Object[] { path });
            if (Environment.MEDIA_MOUNTED.equals(obj))
                mounted = true;
        } catch (NoSuchMethodException ex) {
            throw new RuntimeException(ex);
        } catch (IllegalAccessException ex) {
            throw new RuntimeException(ex);
        } catch (InvocationTargetException ex) {
            throw new RuntimeException(ex);
        }
        return mounted;
    }
    
    /*
     * check if there is second card exists
     */
    public static int getMountedSDCardCount(Context context) {
        int readyCount = 0;
        StorageManager storageManager = (StorageManager) context
                .getSystemService(Context.STORAGE_SERVICE);
        if (storageManager == null)
            return 0;
        Method method;
        Object obj;
        try {
            method = storageManager.getClass().getMethod("getVolumePaths",
                    (Class[]) null);
            obj = method.invoke(storageManager, (Object[]) null);

            String[] paths = (String[]) obj;
            if (paths == null)
                return 0;
            
            method = storageManager.getClass().getMethod("getVolumeState",
                    new Class[] { String.class });
            for (String path : paths) {
                obj = method.invoke(storageManager, new Object[] { path });
                if (Environment.MEDIA_MOUNTED.equals(obj)){
                    readyCount++;
                    if (2 == readyCount) PATH_SDCARD_2 = path;
                    if (3 == readyCount) PATH_SDCARD_3 = path;
                }
            }
        } catch (NoSuchMethodException ex) {
            throw new RuntimeException(ex);
        } catch (IllegalAccessException ex) {
            throw new RuntimeException(ex);
        } catch (InvocationTargetException ex) {
            if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)){
                readyCount = 1;
            }
            Log.d(TAG,ex.getMessage());
            return readyCount;
        }

        Log.d(TAG, "mounted sdcard unmber: " + readyCount);
        return readyCount;
    }
    
    public static boolean checkIfSDExist(Context context){
        return FileUtil.getMountedSDCardCount(context) >= 1;
    }
    
    public static boolean getBooleanProperties(String key, boolean b) {
        boolean result = b;
        Class rSystemProperties;
        try {
            rSystemProperties = Class.forName("android.os.SystemProperties");
            Class[] paras = {String.class};
            Method get = rSystemProperties.getMethod("get", paras);
            get.setAccessible(true);
            String str = (String)get.invoke(null, key);
            result = new Boolean(str);
        } catch (Exception e) {
            Log.e(TAG, "when getsystem properties, get error=" + e.toString());
        }
        return  result;
    }

    public static int getIntProperty(String propertyName, int def) {
        Class sysProp;
        int ob = def;
        try {
            sysProp = Class.forName("android.os.SystemProperties");
            Method get = sysProp.getMethod("get", String.class);
            ob = Integer.valueOf((String) get.invoke(null, propertyName));
        } catch (Exception e) {
            if (Log.DEBUG)
                Log.d(TAG,
                        "when get system properties, get error=" + e.toString());
        }
        return ob;
    }

    public static boolean isExternalCard(Context context, String path,
            boolean has2ndcard) {
        if (null == context || null == path) {
            Log.d(TAG, "the param for is2ndCard is invalid!!!");
            return false;
        }
        if (isInternalMaster()) {
            return path.equals(PATH_SDCARD_2);
        } else {
            return path.equals(Defs.PATH_SDCARD_1);
        }
    }

    public static boolean isInternalCard(Context context, String path,
            boolean has2ndcard) {
        if (null == context || null == path) {
            Log.d(TAG, "the param for is2ndCard is invalid!!!");
            return false;
        }
        if (isInternalMaster()) {
            return path.equals(Defs.PATH_SDCARD_1);
        } else {
            return path.equals(PATH_SDCARD_2);
        }
    }

    /*
     * function: check if the current folder is the PATH_SDCARD_2
     */
    public static boolean is2ndCard(Context context, String path,
            boolean has2ndcard) {
        if (null == context || null == path) {
            Log.d(TAG, "the param for is2ndCard is invalid!!!");
            return false;
        }
        String tempPath = "/sdcard/"
                + context.getString(R.string.external_card);

        if (!has2ndcard) {
            return false;
        } else if (path.equals(PATH_SDCARD_2) || path.equals(tempPath)) {
            return true;
        }
        return false;
    }

    public static boolean is3thCard(Context context, String path,
            boolean has3thcard) {
        if (null == context || null == path) {
            Log.d(TAG, "the param for is3thCard is invalid!!!");
            return false;
        }
        String tempPath = "/sdcard/"
                + context.getString(R.string.external_card);

        if (!has3thcard) {
            return false;
        } else if (path.equals(PATH_SDCARD_3) || path.equals(tempPath)) {
            return true;
        }
        return false;
    }


    public static boolean changePermission(File f) {
        f.getAbsolutePath();
        if (f.getAbsolutePath().startsWith(Defs.PATH_LOCAL)) {
            FileDBOP.changePermission(f.getAbsolutePath());
        }
        return true;
    }

    public static boolean isFolderCanWrite(String path) {
        if (TextUtils.isEmpty(path))
            return false;
        File f = new File(path, ".temp_test");
        if (f.mkdirs()) {
            f.delete();
            return true;
        } else {
            return false;
        }
    }

    public static int checkFileName(String input, String curPath) {
        String name = FileUtil.fixFileName(input);

        if (TextUtils.isEmpty(name))
            return Defs.ERR_EMPTY;

        if (name.contains("/") || name.contains("\\")
                || name.contains(":") || name.contains("?")
                || name.contains("*") || name.contains("\"")
                || name.contains("|") || name.contains("'")
                || name.contains(">") || name.contains("<")
                || name.contains(";") || name.contains(getUnicodeString("/"))
                || name.contains(getUnicodeString("\\"))
                || name.contains(getUnicodeString(":"))
                || name.contains(getUnicodeString("?"))
                || name.contains(getUnicodeString("*"))
                || name.contains(getUnicodeString("\""))
                || name.contains(getUnicodeString("|"))
                || name.contains(getUnicodeString("'"))
                || name.contains(getUnicodeString(">"))
                || name.contains(getUnicodeString("<"))
                || name.contains(getUnicodeString(";"))
                || name.contains(getUnicodeString(":"))) {
            return Defs.ERR_INVALID_CHAR;
        }

        if (!FileUtil.isValidHead(name)) {
            return Defs.ERR_INVALID_INIT_CHAR;
        }

        byte[] utf8Str = null;
        byte[] utf8StrFullPath = null;
        if (!TextUtils.isEmpty(name)) {
            try {
                utf8Str = name.getBytes("utf-8");
                utf8StrFullPath = (curPath + "/" + name).getBytes("utf-8");

                boolean isNameValid = (null != utf8Str && utf8Str.length > 254)
                        || (null != utf8StrFullPath && utf8StrFullPath.length > 1024);
                if (isNameValid)
                    return Defs.ERR_TOO_LONG;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        File f = new File(curPath + "/" + name);
        if (f.exists()) {
            if (f.isDirectory())
                return Defs.ERR_FOLDER_ALREADY_EXIST;
            else if (f.isFile())
                return Defs.ERR_FILE_ALREADY_EXIST;
        }

        if (!f.getName().equals(name))
            return Defs.ERR_INVALID_CHAR;
        return 0;
    }

    public static String getUnicodeString(String input) {
        char[] c = input.toCharArray();
        for (int i = 0; i < c.length; i++) {
            c[i] = convertDBCtoSBC(c[i]);
        }
        return new String(c);
    }

    static public char convertDBCtoSBC(char c) {
        if (c == 32)
            return (char) 12288;
        else if (c == ',')
            return '\uff0c';
        else if (c == '.')
            return '\u3002';
        else if (c == 0 || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9'))
            return c;
        else if (c < 127)
            return (char) (c + 65248);
        else
            return c;
    }

    public static int getCategoryByPath(String path) {
        if (MediaFile.isAudioFileType(path)) {
            return FileUtil.CATEGORY_AUDIO;
        } else if (MediaFile.isVideoFileType(path)) {
            return FileUtil.CATEGORY_VIDEO;
        } else if (MediaFile.isImageFileType(path)) {
            return FileUtil.CATEGORY_IMAGE;
        } else if (MediaFile.isZipFileType(path)) {
            return FileUtil.CATEGORY_ARC;
        } else {
            return FileUtil.CATEGORY_UNKNOWN;
        }
    }

    public static void updateFileInfo(Context context, String newPath,
            String oldPath) {
        // when rename file or folder, must register in DB;
        File newFile = new File(newPath);
        // send a scan dir intent to nofiy scanner delete or update old media
        if (newFile.isDirectory()) {
            Log.d(TAG, "update directory, send scan intent, newPath: "
                    + newPath + ", oldPath: " + oldPath);
            FileDBOP.startMediaScan(context, newFile.getParent());
        } else {
            FileDBOP.updateFileInDB(context, newPath, oldPath);
            FMThumbnailHelper.updateThumbnailInCache(newPath, oldPath);
        }

    }

    public static void showToast(Context context, String str) {
        Log.v(TAG, "show toast(String) str = " + str);
        Toast toast = Toast.makeText(context, "", Toast.LENGTH_SHORT);
        toast.setGravity(Gravity.BOTTOM, 0, 0);
        toast.setText(str);
        toast.setDuration(Toast.LENGTH_SHORT);
        toast.show();
    }

    public static void showToast(Context context, int resid) {
        Log.v(TAG, "show toast(int) resid = " + resid);
        String str = context.getText(resid).toString();
        showToast(context, str);
    }

    public static final String ACTION_ENABLE_SD_INDEX = "com.intel.filemanager.action.enable_sd_index";
    public static final String EXTRA_ENABLE_SD_INDEX = "extra_enable_sd_index";
    public static final String EXTRA_SD_INDEX_RECEAWL = "extra_sd_index_recrawl";
    
    public static void openFolder(Context context, String path){
        if (context instanceof SubCategoryActivity) {
            ((SubCategoryActivity)context).finish();
        }

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setType("com.intel.filemanager/OpenDirectory");
        intent.putExtra(FileMainActivity.EXTRA_FILE_PATH, path);
        intent.setClass(context, FileMainActivity.class);
        context.startActivity(intent);
    }
    
    public  static String formatSize(long size) {
        long appsize = 0;
        if (size == 0) {
            return "0Byte";
        }
        String suffix = null;
        // add K or M suffix if size is greater than 1K or 1M
        if (size >= 1024) {
            suffix = "K";
            appsize = size % 1024;
            size = size >> 10;
            if (size >= 1024) {
                suffix = "M";
                appsize = size % 1024;
                size = size >> 10;
            }
        } else {
            suffix = "Byte";
        }

        StringBuilder resultBuffer = new StringBuilder(Long.toString(size));

        int commaOffset = resultBuffer.length() - 3;
        while (commaOffset > 0) {
            resultBuffer.insert(commaOffset, ',');
            commaOffset -= 3;
        }
        // if(size < 10){
        resultBuffer.append("." + (appsize / 100));
        // }
        if (suffix != null)
            resultBuffer.append(suffix);
        return resultBuffer.toString();
    }

}
