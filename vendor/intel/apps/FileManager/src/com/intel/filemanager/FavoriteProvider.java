/* Copyright (C) 2007 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.intel.filemanager;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.provider.BaseColumns;
import android.provider.MediaStore;
import android.provider.MediaStore.Audio.Media;
import android.text.TextUtils;

import com.intel.filemanager.util.LogHelper;

public class FavoriteProvider extends ContentProvider {

    private static LogHelper Log = LogHelper.getLogger();
    private static final String TAG = "FileProvider";

    private SQLiteOpenHelper mOpenHelper;
    private static final int FAVORITES = 1;
    private static final int FAVORITES_ID = 2;
    private static final UriMatcher sURLMatcher = new UriMatcher(
            UriMatcher.NO_MATCH);

    static {
        sURLMatcher.addURI("com.intel.filemanager", "favorites", FAVORITES);
        sURLMatcher.addURI("com.intel.filemanager", "favorites/#", FAVORITES_ID);
    }

    public static class FavoriteColumns implements BaseColumns {
        public static final Uri CONTENT_URI = Uri
                .parse("content://com.intel.filemanager/favorites");
        public static final String PATH = MediaStore.MediaColumns.DATA;
        public static final String CATEGORY = "category";
        public static final String WEIGHT = "weight";
    }

    private static class DatabaseHelper extends SQLiteOpenHelper {
        private static final String DATABASE_NAME = "file.db";
        private static final int DATABASE_VERSION = 1;

        public DatabaseHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("CREATE TABLE favorites(_id integer primary key autoincrement, "
                    + FavoriteColumns.PATH + " text, " + FavoriteColumns.CATEGORY
                    + " integer default -1, "
                    + FavoriteColumns.WEIGHT + " integer default 0);");
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion,
                int currentVersion) {
            Log.d(TAG, "Upgrading favorites database from version " + oldVersion
                    + " to " + currentVersion
                    + ", which will destroy all old data");
            db.execSQL("DROP TABLE IF EXISTS favorites");
            db.execSQL("CREATE TABLE favorites(_id integer primary key autoincrement, "
                    + FavoriteColumns.PATH + " text, " + FavoriteColumns.CATEGORY
                    + " integer default -1, "
                    + FavoriteColumns.WEIGHT + " integer default 0);");
        }
    }

    @Override
    public boolean onCreate() {
        mOpenHelper = new DatabaseHelper(getContext());
        return true;
    }

    @Override
    public Cursor query(Uri url, String[] projectionIn, String selection,
            String[] selectionArgs, String sort) {
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();

        // Generate the body of the query
        int match = sURLMatcher.match(url);
        switch (match) {
            case FAVORITES:
                qb.setTables("favorites");
                break;
            case FAVORITES_ID:
                qb.setTables("favorites");
                qb.appendWhere("_id=");
                qb.appendWhere(url.getPathSegments().get(1));
                break;
            default:
                throw new IllegalArgumentException("Unknown URL " + url);
        }

        SQLiteDatabase db = mOpenHelper.getReadableDatabase();
        Cursor ret = qb.query(db, projectionIn, selection, selectionArgs, null,
                null, sort);

        if (ret == null) {
            Log.d(TAG, "query: failed");
        } else {
            ret.setNotificationUri(getContext().getContentResolver(), url);
        }
        return ret;
    }

    @Override
    public String getType(Uri url) {
        int match = sURLMatcher.match(url);
        switch (match) {
            case FAVORITES:
                return "vnd.android.cursor.dir/favorites";
            case FAVORITES_ID:
                return "vnd.android.cursor.item/favorites";
            default:
                throw new IllegalArgumentException("Unknown URL");
        }
    }

    @Override
    public int update(Uri url, ContentValues values, String where,
            String[] whereArgs) {
        int count = 0;
        long rowId = 0;
        int match = sURLMatcher.match(url);
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        try {
            switch (match) {
                case FAVORITES_ID: {
                    String segment = url.getPathSegments().get(1);
                    rowId = Long.parseLong(segment);
                    count = db.update("favorites", values, "_id=" + rowId, null);
                    break;
                }
                default: {
                    throw new UnsupportedOperationException("Cannot update URL: "
                            + url);
                }
            }
            Log.d(TAG, "*** notifyChange() rowId: " + rowId + " url " + url);
            getContext().getContentResolver().notifyChange(url, null);

        } catch (Exception e) {
        }

        return count;
    }

    @Override
    public Uri insert(Uri url, ContentValues initialValues) {
        ContentValues values;
        if (initialValues != null) {
            values = new ContentValues(initialValues);
        } else {
            values = new ContentValues();
        }
        if (!values.containsKey(FavoriteColumns.PATH)) {
            throw new IllegalArgumentException(
                    "Can not insert into favorites without geving a path");
        }
        if (!values.containsKey(FavoriteColumns.CATEGORY)) {
            values.put(FavoriteColumns.CATEGORY, -1);
        }
        if (!values.containsKey(FavoriteColumns.WEIGHT)) {
            values.put(FavoriteColumns.WEIGHT, 0);
        }
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long rowId = db.insert("favorites", null, values);
        Uri newUrl = ContentUris.withAppendedId(FavoriteColumns.CONTENT_URI, rowId);
        getContext().getContentResolver().notifyChange(newUrl, null);
        return newUrl;
    }

    public int delete(Uri url, String where, String[] whereArgs) {
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int count;
        long rowId = 0;
        switch (sURLMatcher.match(url)) {
            case FAVORITES:
                count = db.delete("favorites", where, whereArgs);
                break;
            case FAVORITES_ID:
                String segment = url.getPathSegments().get(1);
                rowId = Long.parseLong(segment);
                if (TextUtils.isEmpty(where)) {
                    where = "_id=" + segment;
                } else {
                    where = "_id=" + segment + " AND (" + where + ")";
                }
                count = db.delete("favorites", where, whereArgs);
                break;
            default:
                throw new IllegalArgumentException("Cannot delete from URL: " + url);
        }

        getContext().getContentResolver().notifyChange(url, null);
        return count;
    }
}
