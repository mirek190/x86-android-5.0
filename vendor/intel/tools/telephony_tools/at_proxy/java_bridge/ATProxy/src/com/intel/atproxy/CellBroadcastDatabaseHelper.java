/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.intel.atproxy;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.provider.Telephony;

public class CellBroadcastDatabaseHelper extends SQLiteOpenHelper {

    private static final String TAG = "CellBroadcastDatabaseHelper";

    static final String DATABASE_NAME = "cell_broadcasts.db";
    static final String TABLE_NAME = "broadcasts";
    static final int DATABASE_VERSION = 11;

    CellBroadcastDatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE " + TABLE_NAME + " ("
                + Telephony.CellBroadcasts._ID + " INTEGER PRIMARY KEY AUTOINCREMENT,"
                + Telephony.CellBroadcasts.GEOGRAPHICAL_SCOPE + " INTEGER,"
                + Telephony.CellBroadcasts.PLMN + " TEXT,"
                + Telephony.CellBroadcasts.LAC + " INTEGER,"
                + Telephony.CellBroadcasts.CID + " INTEGER,"
                + Telephony.CellBroadcasts.SERIAL_NUMBER + " INTEGER,"
                + Telephony.CellBroadcasts.SERVICE_CATEGORY + " INTEGER,"
                + Telephony.CellBroadcasts.LANGUAGE_CODE + " TEXT,"
                + Telephony.CellBroadcasts.MESSAGE_BODY + " TEXT,"
                + Telephony.CellBroadcasts.DELIVERY_TIME + " INTEGER,"
                + Telephony.CellBroadcasts.MESSAGE_READ + " INTEGER,"
                + Telephony.CellBroadcasts.MESSAGE_FORMAT + " INTEGER,"
                + Telephony.CellBroadcasts.MESSAGE_PRIORITY + " INTEGER,"
                + Telephony.CellBroadcasts.ETWS_WARNING_TYPE + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_MESSAGE_CLASS + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_CATEGORY + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_RESPONSE_TYPE + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_SEVERITY + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_URGENCY + " INTEGER,"
                + Telephony.CellBroadcasts.CMAS_CERTAINTY + " INTEGER);" );

        createDeliveryTimeIndex(db);
    }

    private void createDeliveryTimeIndex(SQLiteDatabase db) {
        db.execSQL("CREATE INDEX IF NOT EXISTS deliveryTimeIndex ON " + TABLE_NAME
                + " (" + Telephony.CellBroadcasts.DELIVERY_TIME + ");");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
    }
}
