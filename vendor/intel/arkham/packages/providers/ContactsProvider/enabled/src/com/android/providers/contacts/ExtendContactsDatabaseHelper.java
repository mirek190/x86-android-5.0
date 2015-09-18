/*
 * Copyright (C) 2013 Intel Corporation
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
 * limitations under the License
 */

package com.android.providers.contacts;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.os.UserHandle;
import android.os.UserManager;
import android.util.Log;


/** @hide */
public class ExtendContactsDatabaseHelper extends ContactsDatabaseHelper {
    private static final String TAG = "ExtendContactsDatabaseHelper";

    // ARKHAM - 292, 447: Used to add unique _id in tables for primary user and
    // containers
    private long mUniqueStartId;

    protected ExtendContactsDatabaseHelper(
            Context context, String databaseName, boolean optimizationEnabled) {
        super(context, databaseName, optimizationEnabled);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        Log.i(TAG, "Bootstrapping database version: " + DATABASE_VERSION);
        super.onCreate(db);
        // ARKHAM - 292, 447: If the current user is a container one, offset the
        // start of _id for each table with cid * 1000000, in order to have
        // unique _id for each table for primary user and containers
        int userId = UserHandle.myUserId();
        UserManager userManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        if (userManager == null || userManager.getUserInfo(userId) == null || !userManager
                .getUserInfo(userId).isContainer()) {
            return;
        }

        mUniqueStartId = userId * 1000000;

        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.ACCOUNTS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.CONTACTS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.RAW_CONTACTS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.STREAM_ITEMS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.STREAM_ITEM_PHOTOS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.PHOTO_FILES + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.PACKAGES + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.MIMETYPES + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.DATA + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.GROUPS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.AGGREGATION_EXCEPTIONS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.CALLS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.VOICEMAIL_STATUS + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.DATA_USAGE_STAT + "', '" + mUniqueStartId + "')");
        db.execSQL("INSERT INTO sqlite_sequence (name, seq) values ('"
                + Tables.DIRECTORIES + "', '" + mUniqueStartId + "')");
    }
}
