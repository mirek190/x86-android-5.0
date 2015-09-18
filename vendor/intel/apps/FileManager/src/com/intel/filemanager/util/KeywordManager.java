package com.intel.filemanager.util;

import java.util.LinkedList;
import java.util.List;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.text.TextUtils;


public class KeywordManager {
    private static LogHelper Log = LogHelper.getLogger();
    
    private static final String TAG = "KeywordManager";

    private static Context _context;

    private static KeywordManager _instance;

    private static final String KEYWORD_DATABASE = "keyword.db";

    private static final String KEYWORD_TABLE = "keyword_table";

    private static final String NAME_COLUMN = "name";

    private static final String FREQUENCY_COLUMN = "frequency";

    private static final String TIMESTAMP_COLUMN = "timestamp";

    private SQLiteDatabase _database;

    public static void init(Context context) {
        _context = context;
    }

    public static synchronized KeywordManager getInstance() {
        if (_instance != null) {
            return _instance;
        }
        _instance = new KeywordManager();
        return _instance;
    }

    public synchronized List<String> getKeywordList(String prefix, int topN) {
        List<String> result = new LinkedList<String>();
        Cursor cursor = getKeywordCursor(prefix, topN);
        if (cursor == null) {
            return result;
        }
        try {
            while (cursor.moveToNext()) {
                String name = cursor.getString(0);
                result.add(name);
            }
        } finally {
            cursor.close();
        }
        return result;
    }

    public synchronized Cursor getKeywordCursor(String prefix, int topN) {
        ensureIntializeDatabase();
        Cursor cursor = null;
        try {
            String sortByFrequencyAndTimestamp = FREQUENCY_COLUMN + " desc,"
                    + TIMESTAMP_COLUMN + " desc";
            if (isEmpty(prefix)) {
                cursor = _database.query(KEYWORD_TABLE,
                        new String[] { NAME_COLUMN }, null, null, null, null,
                        sortByFrequencyAndTimestamp, String.valueOf(topN));
            } else {
                cursor = _database.query(KEYWORD_TABLE,
                        new String[] { NAME_COLUMN }, NAME_COLUMN + " LIKE ?",
                        new String[] { prefix + "%" }, null, null,
                        sortByFrequencyAndTimestamp, String.valueOf(topN));
            }
        } catch (SQLException e) {
            Log.e(TAG, e.getMessage());
        }
        return cursor;
    }

    public synchronized void increaseFrequency(String name) {
        if(TextUtils.isEmpty(name)){
            return;
        }
        
        try {
            int frequency = getKeywordFrequency(name);
            ++frequency;
            ContentValues values = new ContentValues();
            values.put(NAME_COLUMN, name);
            values.put(FREQUENCY_COLUMN, frequency);
            values.put(TIMESTAMP_COLUMN, System.currentTimeMillis());
            if (frequency == 1) {
                Log.i(TAG, "insert into keyword table " + name);
                _database.insert(KEYWORD_TABLE, null, values);
            } else {
                Log.i(TAG, "update keyword table " + name);
                _database.update(KEYWORD_TABLE, values, NAME_COLUMN + "=?",
                        new String[] { name });
            }
        } catch (SQLException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    public synchronized void clear() {
        ensureIntializeDatabase();
        _database.delete(KEYWORD_TABLE, null, null);
    }
    
    private int getKeywordFrequency(String keyword) {
        Cursor cursor = null;
        int frequency = 0;
        try {
            ensureIntializeDatabase();
            cursor = _database.query(KEYWORD_TABLE,
                    new String[] { FREQUENCY_COLUMN }, NAME_COLUMN + "=?",
                    new String[] { keyword }, null, null, null);
            if (cursor != null && cursor.moveToNext()) {
                frequency = cursor.getInt(0);
                Log.i(TAG, keyword + ", frequency =  "+frequency);
            }
        } catch(Exception e){
            Log.e(TAG,e.getMessage());
        }finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return frequency;
    }

    private void ensureIntializeDatabase() {
        if (_database != null) {
            return;
        }
        _database = _context.openOrCreateDatabase(KEYWORD_DATABASE,
                Context.MODE_WORLD_WRITEABLE, null);
        _database.execSQL(createTableStatement());
    }

    private String createTableStatement() {
        return "create table if not exists " + KEYWORD_TABLE + " ( "
                + NAME_COLUMN + " text not null primary key , "
                + FREQUENCY_COLUMN + " integer not null, " + TIMESTAMP_COLUMN
                + " long not null)";
    }
    
    private boolean isEmpty(String s) {
        return s == null || s.length() == 0;
    }

}
