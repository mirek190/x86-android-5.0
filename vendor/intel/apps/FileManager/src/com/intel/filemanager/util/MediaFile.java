package com.intel.filemanager.util;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import android.util.Xml;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import android.provider.MediaStore;
import android.database.Cursor;
import android.content.ContentResolver;
import android.content.Context;
import android.mtp.MtpConstants;
import android.net.Uri;

import java.util.HashMap;
import java.util.Iterator;
import com.intel.filemanager.R;

import android.os.Environment;

public class MediaFile {
    // comma separated list of all file extensions supported by the media
    // scanner
    public static String sFileExtensions;

    private static LogHelper Log = LogHelper.getLogger();

    public static final String TAG = "MediaFile-Java";

    public static final boolean DBG = true;

    public static final String BINARY_MIMETYPE = "unknown";

    /*
     * File type map Common Fields Audio Types: 0001 - 1000 Midi Types: 1001 -
     * 2000 Video Types: 2001 - 3000 Image Types: 3001 - 4000 Playlist Types:
     * 4001 - 4100 DRM Types: 4101 - 5000 Text Types: 5001 - 6000 Compressed
     * Types: 6001 - 6100 J2ME Types: 6101 - 6200 Description Types: 6201 - 6300
     * Reserved: 6301 - 8000 Company Ext. MS Ext Types: 8001 - 8100 Adobe Ext
     * Types: 8101 - 8200 Android Ext Types: 8201 - 8300 Borqs Ext Types: 8301 -
     * 8400 OMS Ext Types: 8401 - 8500 Misc Types: 9001 - 10000
     */

    // Audio file types
    public static final int FILE_TYPE_MP3     = 1;
    public static final int FILE_TYPE_M4A     = 2;
    public static final int FILE_TYPE_WAV     = 3;
    public static final int FILE_TYPE_AMR     = 4;
    public static final int FILE_TYPE_AWB     = 5;
    public static final int FILE_TYPE_WMA     = 6;
    public static final int FILE_TYPE_OGG     = 7;
    public static final int FILE_TYPE_AAC     = 8;
    public static final int FILE_TYPE_MKA     = 9;
    public static final int FILE_TYPE_FLAC    = 10;
    private static final int FIRST_AUDIO_FILE_TYPE = FILE_TYPE_MP3;
    private static final int LAST_AUDIO_FILE_TYPE = FILE_TYPE_FLAC;

    // MIDI file types
    public static final int FILE_TYPE_MID     = 11;
    public static final int FILE_TYPE_SMF     = 12;
    public static final int FILE_TYPE_IMY     = 13;
    private static final int FIRST_MIDI_FILE_TYPE = FILE_TYPE_MID;
    private static final int LAST_MIDI_FILE_TYPE = FILE_TYPE_IMY;
   
    // Video file types
    public static final int FILE_TYPE_MP4     = 21;
    public static final int FILE_TYPE_M4V     = 22;
    public static final int FILE_TYPE_3GPP    = 23;
    public static final int FILE_TYPE_3GPP2   = 24;
    public static final int FILE_TYPE_WMV     = 25;
    public static final int FILE_TYPE_ASF     = 26;
    public static final int FILE_TYPE_MKV     = 27;
    public static final int FILE_TYPE_MP2TS   = 28;
    public static final int FILE_TYPE_AVI     = 29;
    public static final int FILE_TYPE_WEBM    = 30;
    private static final int FIRST_VIDEO_FILE_TYPE = FILE_TYPE_MP4;
    private static final int LAST_VIDEO_FILE_TYPE = FILE_TYPE_WEBM;
    
    // More video file types
    public static final int FILE_TYPE_MP2PS   = 200;
    private static final int FIRST_VIDEO_FILE_TYPE2 = FILE_TYPE_MP2PS;
    private static final int LAST_VIDEO_FILE_TYPE2 = FILE_TYPE_MP2PS;

    // Image file types
    public static final int FILE_TYPE_JPEG    = 31;
    public static final int FILE_TYPE_GIF     = 32;
    public static final int FILE_TYPE_PNG     = 33;
    public static final int FILE_TYPE_BMP     = 34;
    public static final int FILE_TYPE_WBMP    = 35;
    public static final int FILE_TYPE_WEBP    = 36;
    private static final int FIRST_IMAGE_FILE_TYPE = FILE_TYPE_JPEG;
    private static final int LAST_IMAGE_FILE_TYPE = FILE_TYPE_WEBP;
    
    // Playlist file types
    public static final int FILE_TYPE_M3U = 4001;

    public static final int FILE_TYPE_PLS = 4002;

    public static final int FILE_TYPE_WPL = 4003;
    
    public static final int FILE_TYPE_HTTPLIVE = 4004;

    private static final int FIRST_PLAYLIST_FILE_TYPE = FILE_TYPE_M3U;

    private static final int LAST_PLAYLIST_FILE_TYPE = FILE_TYPE_HTTPLIVE;

    // Borqs: drm support
    public static final int FILE_TYPE_OMADRM_V1 = 4101;
    public static final int FILE_TYPE_OMADRM_V2 = 4102;
    public static final int FILE_TYPE_FL        = 4103;

    private static final int FIRST_DRM_FILE_TYPE = FILE_TYPE_OMADRM_V1;

    private static final int LAST_DRM_FILE_TYPE = FILE_TYPE_FL;

    // Borqs: Text
    public static final int FILE_TYPE_TXT = 5001;

    public static final int FILE_TYPE_HTML = 5002;

    public static final int FILE_TYPE_VCARD = 5003;

    public static final int FILE_TYPE_RTF = 5004;

    public static final int FILE_TYPE_CSV = 5005;

    public static final int FILE_TYPE_RTX = 5006;

    public static final int FILE_TYPE_TSV = 5007;

    public static final int FILE_TYPE_HPP = 5008;

    public static final int FILE_TYPE_CPP = 5009;

    public static final int FILE_TYPE_H = 5010;

    public static final int FILE_TYPE_C = 5011;

    public static final int FILE_TYPE_JAVA = 5012;

    public static final int FILE_TYPE_MOC = 5013;

    public static final int FILE_TYPE_PAS = 5014;

    public static final int FILE_TYPE_ETX = 5015;

    public static final int FILE_TYPE_TCL = 5016;

    public static final int FILE_TYPE_TEX = 5017;

    public static final int FILE_TYPE_VCS = 5018;

    public static final int FILE_TYPE_ICS = 5019;
    

    // Borqs: Compressed
    public static final int FILE_TYPE_ZIP = 6001;

    public static final int FILE_TYPE_BZ2 = 6002;

    public static final int FILE_TYPE_Z = 6003;

    public static final int FILE_TYPE_TGZ = 6004;

    public static final int FILE_TYPE_TAR = 6005;
    

    private static final int FIRST_ZIP_FILE_TYPE = FILE_TYPE_ZIP;
    
    private static final int LAST_ZIP_FILE_TYPE = FILE_TYPE_TAR;


    // S1: flash
    private static final int FILE_TYPE_SWF = 6301;

    // Beihai: RMVB
    private static final int FILE_TYPE_RMVB = 6302;

    // Borqs: JavaME install package
    public static final int FILE_TYPE_JAR = 6101;

    public static final int FILE_TYPE_JAD = 6102;

    // Borqs: Description
    public static final int FILE_TYPE_SDP = 6201;

    public static final int FILE_TYPE_WML = 6202;

    // Borqs: MS Ext.
    public static final int FILE_TYPE_DOC = 8001;

    public static final int FILE_TYPE_XLS = 8002;

    public static final int FILE_TYPE_PPT = 8003;

    public static final int FILE_TYPE_COM = 8004;

    public static final int FILE_TYPE_EXE = 8005;

    public static final int FILE_TYPE_BAT = 8006;

    public static final int FILE_TYPE_DOCX = 8007;

    public static final int FILE_TYPE_DOTX = 8008;

    public static final int FILE_TYPE_DOCM = 8009;

    public static final int FILE_TYPE_DOTM = 8010;

    public static final int FILE_TYPE_XLSX = 8011;

    public static final int FILE_TYPE_XLTX = 8012;

    public static final int FILE_TYPE_XLSM = 8013;

    public static final int FILE_TYPE_XLTM = 8014;

    public static final int FILE_TYPE_PPTX = 8015;

    public static final int FILE_TYPE_PPSX = 8016;

    public static final int FILE_TYPE_POTX = 8017;

    public static final int FILE_TYPE_PPTM = 8018;

    public static final int FILE_TYPE_PPSM = 8019;

    public static final int FILE_TYPE_POTM = 8020;

    // Borqs: Adobe Ext.
    public static final int FILE_TYPE_PDF = 8101;

    // Borqs: Android Ext.
    public static final int FILE_TYPE_APK = 8201;

    public static final int FILE_TYPE_ISO = 8202;

    public static final int FILE_TYPE_SUPK = 8203;
    
    // Borqs: OMS Ext.
    public static final int FILE_TYPE_UPK = 8401;

    // Borqs: Misc.
    public static final int FILE_TYPE_OBJ = 9001;

    public static final int FILE_TYPE_SH = 9002;

    public static final int FILE_TYPE_BIN = 9003;

    // Borqs: Message
    public static final int FILE_TYPE_EML = 9101;

    static class MediaFileType {

        int fileType;

        String mimeType;

        int supported;

        MediaFileType(int fileType, String mimeType, int supported) {
            this.fileType = fileType;
            this.mimeType = mimeType;
            this.supported = supported;
        }

        public int getFileType() {
            return fileType;
        }

        public String getMimeType() {
            return mimeType;
        }
    }

    static class MediaMimeType {
        int fileType;

        int supported;

        MediaMimeType(int fileType, int supported) {
            this.fileType = fileType;
            this.supported = supported;
        }
    }

    private static HashMap<String, MediaFileType> sFileTypeMap = new HashMap<String, MediaFileType>();

    // private static HashMap<String, Integer> sMimeTypeMap
    // = new HashMap<String, Integer>();
    private static HashMap<String, MediaMimeType> sMimeTypeMap = new HashMap<String, MediaMimeType>();

    static void addFileType(String extension, int fileType, String mimeType, int supported) {
        sFileTypeMap.put(extension, new MediaFileType(fileType, mimeType, supported));
        // sMimeTypeMap.put(mimeType, new Integer(fileType));
        sMimeTypeMap.put(mimeType, new MediaMimeType(fileType, supported));
    }

    // Borqs: Load file types from XML file.
//    private static String FILE_TYPE_XML = "/opl/etc/media_filetypes.xml";
//
//    private static boolean loadFileTypesFromXML() {
//        FileReader verReader;
//
//        if (!sFileTypeMap.isEmpty()) {
//            // Log.d(TAG, "loadFileTypesFromXML is not empty.");
//            return true;
//        }
//
//        final File verFile = new File(FILE_TYPE_XML);
//        try {
//            verReader = new FileReader(verFile);
//        } catch (FileNotFoundException e) {
//            Log.e(TAG, "Couldn't find or open profile file " + verFile);
//            return false;
//        }
//
//        try {
//            XmlPullParser parser = Xml.newPullParser();
//            parser.setInput(verReader);
//
//            XmlUtils.beginDocument(parser, "filetypes");
//
//            while (parser.getEventType() != parser.END_DOCUMENT) {
//                XmlUtils.nextElement(parser);
//                String name = parser.getName();
//                if ("filetype".equals(name)) {
//                    String extension = parser.getAttributeValue(null, "extension");
//                    String mimetype = parser.getAttributeValue(null, "mimetype");
//                    int filetype = Integer.parseInt(parser.getAttributeValue(null, "filetype"));
//                    int supported = Integer.parseInt(parser.getAttributeValue(null, "supported"));
//
//                    addFileType(extension, filetype, mimetype, supported);
//                }
//            }
//        } catch (XmlPullParserException e) {
//            Log.e(TAG, "Got execption parsing file types.", e);
//            return false;
//        } catch (IOException e) {
//            Log.e(TAG, "Got execption parsing file types.", e);
//            return false;
//        }
//
//        Log.d(TAG, "loadFileTypesFromXML success.");
//        return true;
//    }

    private static void loadFileTypesStatic() {
        Log.w(TAG, "load file types from XML failed! Try to add basic file types.");

        addFileType("MP3", FILE_TYPE_MP3, "audio/mpeg", 1);
        addFileType("MP3", FILE_TYPE_MP3, "audio/x-mp3", 1);
        addFileType("MP3", FILE_TYPE_MP3, "audio/mp3", 1);
        addFileType("M4A", FILE_TYPE_M4A, "audio/mp4", 1);
        addFileType("M4A", FILE_TYPE_M4A, "audio/m4a", 1);
        addFileType("WAV", FILE_TYPE_WAV, "audio/x-wav", 1);
        addFileType("WAV", FILE_TYPE_WAV, "audio/wav", 1);
        addFileType("AMR", FILE_TYPE_AMR, "audio/amr", 1);
        addFileType("AWB", FILE_TYPE_AWB, "audio/amr-wb", 1);
        addFileType("WMA", FILE_TYPE_WMA, "audio/x-ms-wma", 1);
        addFileType("AAC", FILE_TYPE_AAC, "audio/aac", 1);
        addFileType("OGG", FILE_TYPE_OGG, "application/ogg", 1);
        addFileType("OGA", FILE_TYPE_OGG, "application/ogg", 1);
        addFileType("AAC", FILE_TYPE_AAC, "audio/aac", 1);
        addFileType("AAC", FILE_TYPE_AAC, "audio/aac-adts", 1);
        addFileType("MKA", FILE_TYPE_MKA, "audio/x-matroska", 1);

        addFileType("MID", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("MID", FILE_TYPE_MID, "audio/mid", 1);
        addFileType("MIDI", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("XMF", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("XMF", FILE_TYPE_MID, "audio/xmf", 1);
        addFileType("RTTTL", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("SMF", FILE_TYPE_SMF, "audio/sp-midi", 1);
        addFileType("IMY", FILE_TYPE_IMY, "audio/imelody", 1);
        addFileType("RTX", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("OTA", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("MXMF", FILE_TYPE_MID, "audio/midi", 1);
        addFileType("FLAC", FILE_TYPE_FLAC, "audio/flac", 1);

        addFileType("MP4", FILE_TYPE_MP4, "video/mp4", 1);
        addFileType("MP4", FILE_TYPE_MP4, "video/mpeg4", 1);
        addFileType("M4V", FILE_TYPE_M4V, "video/mp4", 1);
        addFileType("3GP", FILE_TYPE_3GPP, "video/3gp", 1);
        addFileType("3GP", FILE_TYPE_3GPP, "video/3gpp", 1);
        addFileType("3GPP", FILE_TYPE_3GPP, "video/3gpp", 1);
        addFileType("3G2", FILE_TYPE_3GPP2, "video/3gpp2", 1);
        addFileType("3GPP2", FILE_TYPE_3GPP2, "video/3gpp2", 1);
        addFileType("WMV", FILE_TYPE_WMV, "video/x-ms-wmv", 1);
        addFileType("ASF", FILE_TYPE_ASF, "video/x-ms-asf", 1);
        addFileType("AVI", FILE_TYPE_AVI, "video/avi", 1);
        addFileType("MKV", FILE_TYPE_MKV, "video/x-matroska", 1);
        addFileType("MPEG", FILE_TYPE_MP4, "video/mpeg", 1);
        addFileType("MPG", FILE_TYPE_MP4, "video/mpeg", 1);
        addFileType("TS", FILE_TYPE_MP2TS, "video/mp2ts", 1);
        addFileType("WEBM", FILE_TYPE_WEBM, "video/webm", 1);


        addFileType("JPG", FILE_TYPE_JPEG, "image/jpeg", 1);
        addFileType("JPEG", FILE_TYPE_JPEG, "image/jpeg", 1);
        addFileType("GIF", FILE_TYPE_GIF, "image/gif", 1);
        addFileType("PNG", FILE_TYPE_PNG, "image/png", 1);
        addFileType("BMP", FILE_TYPE_BMP, "image/x-ms-bmp", 1);
        addFileType("BMP", FILE_TYPE_BMP, "image/bmp", 1);
        addFileType("WBMP", FILE_TYPE_WBMP, "image/vnd.wap.wbmp", 1);
        addFileType("WEBP", FILE_TYPE_WEBP, "image/webp", 1);

        addFileType("M3U", FILE_TYPE_M3U, "audio/x-mpegurl", 1);
        addFileType("M3U", FILE_TYPE_M3U, "application/x-mpegurl", 1);
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "application/vnd.apple.mpegurl", 1);
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "audio/mpegurl", 1);
        addFileType("M3U8", FILE_TYPE_HTTPLIVE, "audio/x-mpegurl", 1);
        
        addFileType("FL", FILE_TYPE_FL, "application/x-android-drm-fl", 1);

        addFileType("PLS", FILE_TYPE_PLS, "audio/x-scpls", 1);
        addFileType("WPL", FILE_TYPE_WPL, "application/vnd.ms-wpl", 1);
        
        // Borqs: drm support
        addFileType("DCF", FILE_TYPE_OMADRM_V1, "application/vnd.oma.drm.content", 1);
        addFileType("DM", FILE_TYPE_OMADRM_V1, "application/vnd.oma.drm.message", 1);

        // Borqs: JavaME install package
        addFileType("JAR", FILE_TYPE_JAR, "application/java-archive", 1);
        addFileType("JAD", FILE_TYPE_JAD, "text/vnd.sun.j2me.app-descriptor", 1);

        // Borqs: zip
        addFileType("ZIP", FILE_TYPE_ZIP, "application/zip", 1);


        // Borqs: android package
        addFileType("APK", FILE_TYPE_APK, "application/vnd.android.package-archive", 1);
        addFileType("SUPK", FILE_TYPE_SUPK, "application/system-update", 1);
        addFileType("ISO", FILE_TYPE_ISO, "application/system-update", 1);

        addFileType("HTM", FILE_TYPE_HTML, "text/html", 1);
        addFileType("HTML", FILE_TYPE_HTML, "text/html", 1);
        addFileType("SHTML", FILE_TYPE_HTML, "text/html", 1);

        addFileType("TXT", FILE_TYPE_TXT, "text/plain", 1);
        addFileType("TEXT", FILE_TYPE_TXT, "text/plain", 1);
        
        addFileType("VCF", FILE_TYPE_VCARD, "text/x-vcard", 1);
        addFileType("VCARD", FILE_TYPE_VCARD, "text/x-vcard", 1);
        
        addFileType("VCS", FILE_TYPE_VCS, "text/x-vcalendar", 1);
        addFileType("ICS", FILE_TYPE_ICS, "text/calendar", 1);
        
        addFileType("DOC",FILE_TYPE_DOC, "application/msword", 1);
        addFileType("DOT",FILE_TYPE_DOC, "application/msword", 1);
        addFileType("DOCX",FILE_TYPE_DOC, "application/msword", 1);
        addFileType("XLS",FILE_TYPE_XLS, "application/vnd.ms-excel", 1);
        addFileType("XLT",FILE_TYPE_XLS, "application/vnd.ms-excel", 1);
        addFileType("PPT",FILE_TYPE_PPT, "application/vnd.ms-powerpoint", 1);
        addFileType("PPS",FILE_TYPE_PPT, "application/vnd.ms-powerpoint", 1);
        addFileType("SUPK",FILE_TYPE_SUPK, "application/system-update", 1);
        addFileType("PDF",FILE_TYPE_PDF, "application/pdf", 1);
    }

    static {
        loadFileTypesStatic();
        // compute file extensions list for native Media Scanner
        StringBuilder builder = new StringBuilder();
        Iterator<String> iterator = sFileTypeMap.keySet().iterator();

        while (iterator.hasNext()) {
            if (builder.length() > 0) {
                builder.append(',');
            }
            builder.append(iterator.next());
        }
        sFileExtensions = builder.toString();
    }

    public static final String UNKNOWN_STRING = "<unknown>";
    
    public static boolean isMediaFile(String path){
        return isAudioFileType(path) || isVideoFileType(path) || isImageFileType(path); 
    }

    public static boolean isAudioFileType(int fileType) {
        return ((fileType >= FIRST_AUDIO_FILE_TYPE && fileType <= LAST_AUDIO_FILE_TYPE) || (fileType >= FIRST_MIDI_FILE_TYPE && fileType <= LAST_MIDI_FILE_TYPE));
    }

    public static boolean isAudioFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        return isAudioFileType(filetype.fileType);
    }

    public static boolean isVideoFileType(int fileType) {
        return (fileType >= FIRST_VIDEO_FILE_TYPE && fileType <= LAST_VIDEO_FILE_TYPE);
    }

    public static boolean isVideoFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }

        return isVideoFileType(filetype.fileType);
    }

    public static boolean isImageFileType(int fileType) {
        return (fileType >= FIRST_IMAGE_FILE_TYPE && fileType <= LAST_IMAGE_FILE_TYPE);
    }

    public static boolean isImageFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        return isImageFileType(filetype.fileType);
    }

    public static boolean isZipFileType(int fileType) {
        return (fileType >= MediaFile.FIRST_ZIP_FILE_TYPE && fileType <= MediaFile.LAST_ZIP_FILE_TYPE);
    }

    public static boolean isZipFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        return isZipFileType(filetype.fileType);
    }

    public static boolean isDrmFileType(int fileType) {
        return (fileType >= FIRST_DRM_FILE_TYPE && fileType <= LAST_DRM_FILE_TYPE);
    }
    
    public static boolean isDrmFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        return isDrmFileType(filetype.fileType);
    }

    public static boolean isPlayListFileType(int fileType) {
        return (fileType >= FIRST_PLAYLIST_FILE_TYPE && fileType <= LAST_PLAYLIST_FILE_TYPE);
    }

    public static boolean isPlayListFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        return isPlayListFileType(filetype.fileType);
    }

    public static boolean isDocumentFileType(String path) {
        MediaFileType filetype = getFileType(path);
        if (null == filetype) {
            return false;
        }
        boolean isDoc = false;
        int type = filetype.fileType;
        isDoc = (type >= FILE_TYPE_TXT && type <= FILE_TYPE_HTML)
                || (type >= FILE_TYPE_DOC && type <= FILE_TYPE_PDF);

        return isDoc;
    }
    public static MediaFileType getFileType(String path) {
        int lastDot = path.lastIndexOf(".");
        if (lastDot < 0)
            return null;

        return sFileTypeMap.get(path.substring(lastDot + 1).toUpperCase());
    }

    public static int getFileTypeForMimeType(String mimeType) {
        // Integer value = sMimeTypeMap.get(mimeType.toLowerCase());
        // return (value == null ? 0 : value.intValue());
        MediaMimeType mmimeType = sMimeTypeMap.get(mimeType.toLowerCase());
        return (mmimeType == null ? 0 : mmimeType.fileType);
    }

    public static boolean checkSupport(String path) {
        boolean ret = false;
        if (null == path || 0 == path.length())
            return ret;
        MediaFileType ft = getFileType(path);
        if (null != ft)
            ret = (ft.supported == 1) ? (true) : (false);
        else
            Log.d(TAG, "error on get file type in hash map");

        return ret;
    }

    /* @hide */
    public static int getFileTypeForSupportedMimeType(String mimeType) {
        MediaMimeType mmimeType = sMimeTypeMap.get(mimeType.toLowerCase());
        if (mmimeType == null) {
            return 0;
        }
        if (mmimeType.supported == 0) {
            return 0;
        } else {
            return mmimeType.fileType;
        }
    }

    public static String getRealMimeType(Context ctx, String path) {
        Log.d(TAG, "Enter getRealMimeType, path:" + path);
        if (null == path) {
            Log.d(TAG, "path is null");
            return null;
        }
        if (null == ctx) {
            Log.d(TAG, "context is null, call 'getMimeType(path)'");
            return getMimeType(path);
        }
        if (!path.startsWith("/")) {
            Log.d(TAG, "path is not absolute one");
            return null;
        }

        if (path.contains("'"))
            path = path.replaceAll("'", "''");

        ContentResolver resolver = ctx.getContentResolver();
        if (null == resolver) {
            Log.d(TAG, "ContentResolver is null, call 'getMimeType(path)'");
            return getMimeType(path);
        }

        Log.d(TAG, "###### lookup in video table");
        // lookup in video table
        Uri uri = path.startsWith(Environment.getExternalStorageDirectory().getAbsolutePath()) ? MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                : MediaStore.Video.Media.INTERNAL_CONTENT_URI;

        Cursor c = null;
        try {
            c = resolver.query(uri, new String[] {
                    MediaStore.Video.Media.MIME_TYPE
                }, MediaStore.Video.Media.DATA + "='" + path + "'", null, null);
        
            if (null == c || 0 == c.getCount())
                Log.d(TAG, "###### Item is not in 'Video' table, will lookup it in 'Audio' table");
            else {
                c.moveToFirst();
                String mimeType = c
                        .getString(c
                                .getColumnIndexOrThrow(MediaStore.Video.Media.MIME_TYPE));
                Log.d(TAG, "###### Video mimetype:" + mimeType);
                return mimeType;
            }
        } catch(Exception e){
            Log.d(TAG, e.getMessage());
            return BINARY_MIMETYPE;
        }finally {
            if (null != c)
                c.close();
        }

        Log.d(TAG, "###### lookup in audio table");
        // lookup in audio table
        uri = path.startsWith(Environment.getExternalStorageDirectory().getAbsolutePath()) ? MediaStore.Audio.Media.EXTERNAL_CONTENT_URI
                : MediaStore.Audio.Media.INTERNAL_CONTENT_URI;

        
        try {
            c = resolver.query(uri, new String[] {
                    MediaStore.Audio.Media.MIME_TYPE
                }, MediaStore.Audio.Media.DATA + "='" + path + "'", null, null);
        
            if (null == c || 0 == c.getCount()) {
                Log.d(TAG, "###### there is no such item in the audio_meta table, try lookup it in static mime table");
                // there is no such item in the audio_meta table, try lookup it in static mime table
                return getMimeType(path);
            } else {
                c.moveToFirst();
                String mimeType = c
                        .getString(c
                                .getColumnIndexOrThrow(MediaStore.Audio.Media.MIME_TYPE));
                Log.d(TAG, "###### Audio mimetype:" + mimeType);
                return mimeType;
            }
        } catch(Exception e){
            Log.d(TAG, e.getMessage());
            return BINARY_MIMETYPE;
        }finally {
            if (null != c)
                c.close();
        }
    }

    /* @hide */
    public static String getMimeType(String path) {
        MediaFileType mtype = getFileType(path);
        if (null == mtype)
            return BINARY_MIMETYPE;
        return mtype.getMimeType();
    }

    /* @hide */
    public static String getMimeTypeForFile(File file) {
        return getMimeType(file.getName());
    }

    /* @hide */
    public static int getIconForPath(String path) {
        int fileType;
        MediaFileType filetype = getFileType(path);
        if (null == filetype)
            fileType = 0;
        else
            fileType = filetype.fileType;
        return getIconForFileType(fileType);
    }

    /* @hide */
    public static int getIconForMimeType(String mimeType) {
        return getIconForFileType(getFileTypeForMimeType(mimeType));
    }

    private static int getIconForFileType(int fileType) {
        // Log.d(TAG, "getIconForFileType: " + fileType);
        if (isAudioFileType(fileType)) {
            return R.drawable.file_icon_audio;
        }

        if (isVideoFileType(fileType)) {
            return R.drawable.file_icon_video;
        }

        if (isImageFileType(fileType)) {
            return R.drawable.file_icon_image;
        }

        if (isZipFileType(fileType)) {
            return R.drawable.file_icon_zip;
        }

        if (fileType == MediaFile.FILE_TYPE_JAD || fileType == MediaFile.FILE_TYPE_JAR
                || fileType == MediaFile.FILE_TYPE_ISO) {
            return R.drawable.file_icon_zip;
        }

        if (fileType == MediaFile.FILE_TYPE_APK){
            return android.R.drawable.sym_def_app_icon;
        }

        if (fileType == MediaFile.FILE_TYPE_ICS || fileType == MediaFile.FILE_TYPE_VCS) {
            return R.drawable.fm_list_vcalendar;
        }

        if (fileType == MediaFile.FILE_TYPE_RMVB) {
            return R.drawable.file_icon_video;
        }

        int icon = -1;

        switch (fileType) {
            case MediaFile.FILE_TYPE_VCARD:
                icon = R.drawable.file_icon_vcard;
                break;
            case MediaFile.FILE_TYPE_XLS:
                icon = R.drawable.file_icon_excel;
                break;
            case MediaFile.FILE_TYPE_XLSX:
                icon = R.drawable.file_icon_excel;
                break;
            case MediaFile.FILE_TYPE_PDF:
                icon = R.drawable.file_icon_pdf;
                break;
            case MediaFile.FILE_TYPE_PPT:
                icon = R.drawable.file_icon_ppt;
                break;
            case MediaFile.FILE_TYPE_PPTX:
                icon = R.drawable.file_icon_ppt;
                break;
            case MediaFile.FILE_TYPE_DOC:
                icon = R.drawable.file_icon_word;
                break;
            case MediaFile.FILE_TYPE_DOCX:
                icon = R.drawable.file_icon_word;
                break;
            case MediaFile.FILE_TYPE_TXT:
                icon = R.drawable.file_icon_text;
                break;
            case MediaFile.FILE_TYPE_HTML:
                icon = R.drawable.file_icon_html;
                break;
            case MediaFile.FILE_TYPE_SWF:
                icon = R.drawable.file_icon_swf;
                break;
            case MediaFile.FILE_TYPE_EML:
                icon = R.drawable.file_icon_eml;
                break;
            case MediaFile.FILE_TYPE_SUPK:
                icon = R.drawable.file_icon_upgrade;
                break;
            default:
                icon = R.drawable.file_icon_unkonwn;
                break;
        }

        return icon;
    }
}
