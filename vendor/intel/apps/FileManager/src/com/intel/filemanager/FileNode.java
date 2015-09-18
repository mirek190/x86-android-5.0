package com.intel.filemanager;

import java.io.File;
import java.io.Serializable;

import com.intel.filemanager.R;
import com.intel.filemanager.util.FileUtil;
import com.intel.filemanager.util.MediaFile;

import android.content.Context;

public class FileNode implements Serializable {
    /**
     * 
     */
    private static final long serialVersionUID = -4836329459309821873L;

    public final static String FILE_FOLDER = "folder";

    public int id;
    public int iconRes;
    public String path;
    public String name;
    public boolean isFile;
    public long lastModifiedTime;
    public long lengthInByte;
    public String type;
    public boolean hasThumbnail;

    public FileNode() {
        // TODO Auto-generated constructor stub
    }

    public FileNode(Context ctx, String path) {
        init(ctx, path);
    }

    public FileNode(Context ctx, String path, int position) {
        init(ctx, path);
        this.id = position;
    }

    public FileNode init(Context ctx, String path) {
        File file = new File(path);
        String filePath = file.getAbsolutePath();
        String fileName = file.getName();
        boolean hasThumbnail = false;
        int fileIcon = 0;
        String fileType = "";
        // FileNode temp = new FileNode();

        if (file.isFile()) {
            if (fileName.endsWith(".mp4") || fileName.endsWith(".3gp") || fileName.endsWith(".asf")) {
                fileType = MediaFile.getRealMimeType(ctx, file.getAbsolutePath());
            } else if (fileName.endsWith(".dcf") || fileName.endsWith("dm")) {
                fileType = MediaFile.getRealMimeType(ctx, file.getAbsolutePath());
            } else {
                fileType = MediaFile.getMimeType(file.getAbsolutePath());
            }
            fileIcon = FileUtil.getIconByMimeType(fileType);
            // if (fileIcon == R.drawable.video || fileIcon == R.drawable.image
            // || fileIcon == R.drawable.apk) {
            // hasThumbnail = true;
            // }
        } else {
            fileIcon = R.drawable.file_icon_folder;
            fileType = FILE_FOLDER;
            // mFolderCount++;
        }

        this.id = 0;
        this.iconRes = fileIcon;
        this.path = filePath;
        this.name = fileName;
        this.isFile = file.isFile();
        this.lastModifiedTime = file.lastModified();
        this.lengthInByte = file.length();
        this.type = fileType;
        this.hasThumbnail = hasThumbnail;
        return this;
    }

    @Override
    public String toString() {
        return "FileNode [id=" + id + ", iconRes=" + iconRes + ", path=" + path
                + ", name=" + name + ", isFile=" + isFile
                + ", lastModifiedTime=" + lastModifiedTime + ", lengthInByte="
                + lengthInByte + ", type=" + type + ", hasThumbnail="
                + hasThumbnail + "]";
    }

}
