package com.intel.filemanager.util;

import android.util.Log;

public class LogHelper {

	private final static String LOGTAG = "FileManager";
	public final static boolean DEBUG = true;

	private static LogHelper instance = null;

	public synchronized static LogHelper getLogger() {
		if (instance == null) {
			Log.v("LogHelper", "[LogHelper] Create LogHelper Config.DEBUG=" + DEBUG);
			instance = new LogHelper();
		}
		return instance;
	}

	private LogHelper() {

	}

	public void v(String LOG_TAG, String log) {
		if (DEBUG) {
			Log.v(LOGTAG, LOG_TAG +  ": " + log);
		}
	}

	public void v(String LOG_TAG, String log, Throwable tr) {
		if (DEBUG) {
			Log.v(LOGTAG,
					LOG_TAG + ": " + log + "\n" + Log.getStackTraceString(tr));
		}
	}

	public void d(String LOG_TAG, String log) {
		if (DEBUG) {
			Log.d(LOGTAG, LOG_TAG +  ": " + log);
		}
	}

	public void i(String LOG_TAG, String log) {
		if (DEBUG) {
			Log.i(LOGTAG, LOG_TAG +  ": " + log);
		}
	}

	public void i(String LOG_TAG, String log, Throwable tr) {
		if (DEBUG) {
			Log.i(LOGTAG,
					LOG_TAG + ": " + log + "\n" + Log.getStackTraceString(tr));
		}
	}

	public void w(String LOG_TAG, String log) {
		if (DEBUG) {
			Log.w(LOGTAG, LOG_TAG + ": " + log);
		}
	}

	public void w(String LOG_TAG, String log, Throwable tr) {
		if (DEBUG) {
			Log.w(LOGTAG,
					LOG_TAG + ": " + log + "\n" + Log.getStackTraceString(tr));
		}
	}

	public void w(String LOG_TAG, Throwable tr) {
		if (DEBUG) {
			Log.w(LOGTAG, LOG_TAG + ": " + Log.getStackTraceString(tr));
		}
	}

	public void e(String LOG_TAG, String log) {
		if (DEBUG) {
			Log.e(LOGTAG, LOG_TAG + ": " + log);
		}

	}

	public void e(String LOG_TAG, String log, Throwable tr) {
		if (DEBUG) {
			Log.e(LOGTAG,
					LOG_TAG + ": " + log + "\n" + Log.getStackTraceString(tr));
		}
	}
}
