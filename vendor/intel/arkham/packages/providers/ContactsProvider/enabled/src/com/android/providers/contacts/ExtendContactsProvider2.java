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

import android.accounts.Account;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.UriMatcher;
import android.content.pm.UserInfo;
import android.content.res.AssetFileDescriptor;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.Binder;
import android.os.CancellationSignal;
import android.os.UserHandle;
import android.os.UserManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.StreamItems;
import android.provider.ContactsContract.StreamItemPhotos;
import android.util.Log;

import com.android.providers.contacts.ContactsDatabaseHelper.AccountsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.GroupsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.PresenceColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.RawContactsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.Tables;
import com.intel.arkham.ContainerConstants;
import com.intel.arkham.ContainerCommons;
import com.intel.arkham.ContainerInfo;
import com.intel.arkham.ContainerManager;
import com.intel.arkham.ContainerPolicyManager;

import java.io.FileNotFoundException;
import java.util.List;
import java.util.Set;

/** @hide */
public class ExtendContactsProvider2 extends ContactsProvider2 {
    private static final String TAG = "ExtendContactsProvider2";
    private static final boolean DEBUG = false;

    // ARKHAM-292,447 START:
    // Used to detect the calling package
    private static final String CONTACTS_APP_PACKAGE = "com.android.contacts";
    // ContactsProviderProxy authorities
    private static final String PROXY_URI = "content://com.intel.arkham.contactsproviderproxy";
    // ARKHAM-292,447 END

    private static final int CONTAINER_ID_OFFSET = 1000000;
    private static final int BASE_CID = 10;

    // ARKHAM-292,447
    private ContainerManager mContainerManager;
    private ContainerPolicyManager mContainerPolicyManager;
    private UserManager mUserManager;

    // ARKHAM-780 - field whitelist used for contact restriction
    private List<String> mContactFieldWhiteList = null;

    protected boolean initialize() {
        super.initialize();

        // ARKHAM-292,447: get container manager
        mContainerManager = new ContainerManager(getContext());
        mUserManager = getUserManager();
        /* ARKHAM-824: Remove ContainerLauncher settings; Use CPM for retrieving
         * the contacts merging option. */
        IntentFilter containerDisabledFilter = new IntentFilter();
        containerDisabledFilter.addAction(ContainerConstants.ACTION_CLEAR_CONTACTS);
        getContext().registerReceiver(new BroadcastReceiver() {
                public void onReceive(Context context, Intent intent) {
                    Object[] objAccount = intent.getParcelableArrayExtra(
                            ContainerConstants.EXTRA_INTENT_ACCOUNTS);
                    if (objAccount == null)
                        return;
                    final ContactsDatabaseHelper dbHelper = mDbHelper.get();
                    if (dbHelper == null) {
                        return;
                    }
                    final SQLiteDatabase db = dbHelper.getWritableDatabase();
                    if (db == null)
                        return;
                    db.beginTransaction();
                    final Set<AccountWithDataSet> knownAccountsWithDataSets =
                            dbHelper.getAllAccountsWithDataSets();
                    for (AccountWithDataSet knownAccountWithDataSet : knownAccountsWithDataSets) {
                        for (int i = 0; i < objAccount.length; i++) {
                            Account account = (Account)objAccount[i];
                            if (account == null) {
                                return;
                            }
                            if (account.name.equals(knownAccountWithDataSet.getAccountName())) {
                                final Long accountIdOrNull = dbHelper.getAccountIdOrNull(
                                        knownAccountWithDataSet);
                                if (accountIdOrNull != null) {
                                    final String accountId = Long.toString(accountIdOrNull);
                                    final String[] accountIdParams =
                                            new String[] {accountId};
                                    db.execSQL(
                                            "DELETE FROM " + Tables.GROUPS
                                            + " WHERE " + GroupsColumns.ACCOUNT_ID + " = ?",
                                            accountIdParams);
                                    db.execSQL(
                                            "DELETE FROM " + Tables.PRESENCE
                                            + " WHERE " + PresenceColumns.RAW_CONTACT_ID + " IN ("
                                            + "SELECT " + RawContacts._ID
                                            + " FROM " + Tables.RAW_CONTACTS
                                            + " WHERE " + RawContactsColumns.ACCOUNT_ID + " = ?)",
                                            accountIdParams);
                                    db.execSQL(
                                            "DELETE FROM " + Tables.STREAM_ITEM_PHOTOS
                                            + " WHERE " + StreamItemPhotos.STREAM_ITEM_ID + " IN ("
                                            + "SELECT " + StreamItems._ID
                                            + " FROM " + Tables.STREAM_ITEMS
                                            + " WHERE " + StreamItems.RAW_CONTACT_ID + " IN ("
                                            + "SELECT " + RawContacts._ID
                                            + " FROM " + Tables.RAW_CONTACTS
                                            + " WHERE " + RawContactsColumns.ACCOUNT_ID + "=?))",
                                            accountIdParams);
                                    db.execSQL(
                                            "DELETE FROM " + Tables.STREAM_ITEMS
                                            + " WHERE " + StreamItems.RAW_CONTACT_ID + " IN ("
                                            + "SELECT " + RawContacts._ID
                                            + " FROM " + Tables.RAW_CONTACTS
                                            + " WHERE " + RawContactsColumns.ACCOUNT_ID + " = ?)",
                                            accountIdParams);
                                    db.execSQL(
                                            "DELETE FROM " + Tables.RAW_CONTACTS
                                            + " WHERE " + RawContactsColumns.ACCOUNT_ID + " = ?",
                                            accountIdParams);
                                    db.execSQL(
                                            "DELETE FROM " + Tables.ACCOUNTS
                                            + " WHERE " + AccountsColumns._ID + "=?",
                                            accountIdParams);
                                }
                            }
                        }
                    }
                    db.setTransactionSuccessful();
                    db.endTransaction();
                }
            }, containerDisabledFilter,
                    ContainerConstants.PERMISSION_SEND_CONTAINER_BROADCAST, null);
        mContainerPolicyManager = ContainerPolicyManager.getInstance(getContext());
        return true;
    }

    @Override
    protected Uri insertInTransaction(Uri uri, ContentValues values) {
        if (VERBOSE_LOGGING) {
            Log.v(TAG, "insertInTransaction: uri=" + uri + "  values=[" + values + "]");
        }

        // ARKHAM-770,777,780 - START
        // Restrict contact fields which are not present in the whitelist
        int callerId = Binder.getCallingUid();
        int uid = UserHandle.getUserId(callerId);
        if (getUserManager() != null) {
            UserInfo user = mUserManager.getUserInfo(uid);
            final boolean callerIsSyncAdapter =
                    readBooleanQueryParameter(uri, ContactsContract.CALLER_IS_SYNCADAPTER, false);
            if (user != null && user.isContainer()
                    && containerExportsContacts(uid) == ContainerConstants.MergeContacts.NORMAL
                    && callerIsSyncAdapter) {
                Object mimetype = values.get(ContactsContract.Data.MIMETYPE);
                if (mimetype != null) {
                    initContactFieldWhitelist(uid);

                    if (mContactFieldWhiteList != null) {
                        boolean whitelisted = false;
                        for (String element : mContactFieldWhiteList) {
                            if (mimetype.equals(element)) {
                                whitelisted = true;
                                break;
                            }
                        }
                        if (!whitelisted) {
                            return ContentUris.withAppendedId(uri, 0);
                        }
                    } else {
                        Log.e(TAG, "The ContactFieldWhiteList is empty!");
                    }
                }
            }
            // ARKHAM-770,777,780 - END
        }
        return super.insertInTransaction(uri, values);
    }

    /**
     * ARKHAM - 292, 447 START: check to see if the caller is the com.android.contacts package
     * @param callerId
     * @return
     */
    private boolean isFromContacts(int callerId) {
        String[] packages = getContext().getPackageManager().getPackagesForUid(
                callerId);
        for (String appPackage : packages) {
            if (appPackage.equals(CONTACTS_APP_PACKAGE)) {
                return true;
            }
        }
        return false;
    }

    private ContainerConstants.MergeContacts containerExportsContacts(int cid) {
        ContainerConstants.MergeContacts mergeContacts = ContainerConstants.MergeContacts.DISABLED;
        mergeContacts = mContainerManager.getMergeContactsPolicy(cid);
        return mergeContacts;
    }

    /**
     * ARKHAM - 292, 447: Rebuild the URI using the proxy authority URI, then
     * append the original path segments
     * @param uri
     * @param containerId
     * @return
     */
    private Uri buildProxyUri(Uri uri, int containerId) {
        String queryParams = new String();
        List<String> pathSegments = uri.getPathSegments();
        for (String segment : pathSegments)
            queryParams += "/" + segment;
        if (uri.getQuery() != null)
            queryParams += "?" + uri.getQuery();

        // ARKHAM-903: append "_container_{containerId}" suffix to authority
        Uri proxyUri = Uri.parse(String.format("%s_container_%s", PROXY_URI, containerId) + "/"
                + containerId + queryParams);
        if (VERBOSE_LOGGING)
            Log.d(TAG, "Built proxy URI: " + proxyUri);
        return proxyUri;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        int ret = super.update(uri, values, selection, selectionArgs);
        if (!mapsToProfileDb(uri) &&  ret <= 0) {
            // ARKHAM-292,447 START: Otherwise if the calling user is not
            // a container and the calling is from com.android.contacts package,
            // try update in containers.
            int callerId = Binder.getCallingUid();
            int uid = UserHandle.getUserId(callerId);
            ret = 0;
            if (getUserManager() == null) return ret;
            UserInfo user = mUserManager.getUserInfo(uid);
            if (user != null && !user.isContainer() && isFromContacts(callerId)) {
                List<ContainerInfo> containers = mContainerManager.listContainers();
                if (containers == null) return 0;
                for (int i = 0; i < containers.size(); i++) {
                    ContainerInfo container = containers.get(i);
                    String containerName = container.getContainerName();
                    int cid = container.getContainerId();
                    UserInfo containerInfo = mUserManager.getUserInfo(cid);
                    if (containerExportsContacts(cid)
                            != ContainerConstants.MergeContacts.ENCRYPTED
                            || (containerInfo != null && uid != containerInfo.containerOwner))
                        continue;
                    if (VERBOSE_LOGGING)
                        Log.e(TAG, "Update container " + containerName
                                + ", cid: " + cid);
                    Uri proxyUri = buildProxyUri(uri, cid);
                    ret = getContext().getContentResolver().update(proxyUri,
                            values, selection, selectionArgs);
                    if (ret > 0) {
                        return ret;
                    }
                }
            }
            // ARKHAM-292,447 END
        }
        return ret;
    }


    /**
     * Get Contacts field whitelist policy from Container Policy Manager
     * ARKHAM-891
     */
    private void initContactFieldWhitelist(int cid) {
        if (mContactFieldWhiteList == null) {
            if (mContainerPolicyManager != null) {
                long ident = Binder.clearCallingIdentity();
                try {
                    mContactFieldWhiteList =
                            mContainerPolicyManager.getContactFieldWhiteListForContainer(cid);
                } finally {
                    Binder.restoreCallingIdentity(ident);
                }
            } else {
                Log.e(TAG, "Couldn't connect to CPM!");
            }
        }
    }

    protected ContactsDatabaseHelper getDatabaseHelper(final Context context) {
        return ExtendContactsDatabaseHelper.getInstance(context);
    }

    /**
     * ARKHAM-534 - Fetch container contact photos through the proxy provider
     */
    protected AssetFileDescriptor openContainerAssetFile(Uri uri, String mode)
        throws FileNotFoundException {
        AssetFileDescriptor ret = null;
        if (sUriMatcher.match(uri) == DISPLAY_PHOTO_ID) {
            // If the photo belongs to a container, fetch it through the ContactsProviderProxy.
            long photoFileId = ContentUris.parseId(uri);
            if (photoFileId > (CONTAINER_ID_OFFSET * BASE_CID)) {
                int cid = (int) photoFileId / CONTAINER_ID_OFFSET;
                int uid = UserHandle.getUserId(Binder.getCallingUid());

                if (cid == uid) {
                    return ret;
                }

                initContactFieldWhitelist(cid);

                if (mContactFieldWhiteList == null) {
                    return ret;
                }

                String type = ContactsContract.CommonDataKinds.Photo.CONTENT_ITEM_TYPE;
                if (mContactFieldWhiteList.contains(type)) {
                    Uri proxyUri = buildProxyUri(uri, cid);
                    if (VERBOSE_LOGGING) Log.d(TAG, "openAssetFile -> Redirecting to proxy...");
                    ret = getContext().getContentResolver().openAssetFileDescriptor(proxyUri, mode);
                }
            }
        }
        return ret;
    }

    protected int deleteContainerContact(Uri uri, String selection, String[] selectionArgs) {
        String accountName = getQueryParameter(uri, RawContacts.ACCOUNT_NAME);
        String accountType = getQueryParameter(uri, RawContacts.ACCOUNT_TYPE);

        int callerId = Binder.getCallingUid();
        int uid = UserHandle.getUserId(callerId);
        if (getUserManager() == null) return 0;

        UserInfo user = mUserManager.getUserInfo(uid);
        if (user != null && !user.isContainer() && isFromContacts(callerId)) {
            List<ContainerInfo> containers = mContainerManager.listContainers();
            if (containers == null) {
                return 0;
            }
            // If the account name is passed via uri and the account is a
            // SYNC_ACCOUNT_TYPE query the specific container
            if (accountName != null && accountType != null && accountType.equals(
                    ContainerConstants.SYNC_ACCOUNT_TYPE)) {
                for (int i = 0; i < containers.size(); i++) {
                    ContainerInfo container = containers.get(i);
                    String containerName = container.getContainerName();
                    if (!accountName.equals(containerName)) {
                        continue;
                    }
                    int cid = container.getContainerId();
                    UserInfo ui = mUserManager.getUserInfo(cid);
                    if (containerExportsContacts(cid)
                            != ContainerConstants.MergeContacts.ENCRYPTED
                            || (ui != null && uid != ui.containerOwner)) {
                        return 0;
                    }
                    Uri proxyUri = buildProxyUri(uri, cid);
                    return getContext().getContentResolver().delete(
                            proxyUri, selection, selectionArgs);
                }
            } else {
                for (int i = 0; i < containers.size(); i++) {
                    ContainerInfo container = containers.get(i);
                    int cid = container.getContainerId();
                    UserInfo ui = mUserManager.getUserInfo(cid);
                    if (containerExportsContacts(cid) != ContainerConstants.MergeContacts.ENCRYPTED
                            || (ui != null && uid != ui.containerOwner)) {
                        continue;
                    }
                    Uri proxyUri = buildProxyUri(uri, cid);
                    int ret = getContext().getContentResolver().delete(
                            proxyUri, selection, selectionArgs);
                    if (ret > 0) {
                        return ret;
                    }
                }
            }
        }
        return 0;
    }

    protected Cursor[] queryContainerContact(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder, CancellationSignal cancellationSignal) {
        // ARKHAM - 292, 447 START: First, query container contacts providers if it's the case
        // We do that ONLY as the non-container user and if the caller is com.android.contacts
        String accountName = getQueryParameter(uri, RawContacts.ACCOUNT_NAME);
        String accountType = getQueryParameter(uri, RawContacts.ACCOUNT_TYPE);

        Cursor[] cursorArray = null;
        int callerId = Binder.getCallingUid();
        int uid = UserHandle.getUserId(callerId);
        if (getUserManager() == null) return null;

        Cursor containerCursor;
        UserInfo user = mUserManager.getUserInfo(uid);
        if (user != null && !user.isContainer() && isFromContacts(callerId)) {
            List<ContainerInfo> containers = mContainerManager.listContainers();
            if (containers == null) {
                return null;
            }
            // If the account name is passed via uri and the account is a
            // SYNC_ACCOUNT_TYPE query the specific container
            if (accountName != null && accountType != null
                && accountType.equals(ContainerConstants.SYNC_ACCOUNT_TYPE)) {
                cursorArray = new Cursor[1];
                for (int i = 0; i < containers.size(); i++) {
                    ContainerInfo container = containers.get(i);
                    String containerName = container.getContainerName();
                    if (!accountName.equals(containerName)) {
                        continue;
                    }
                    int cid = container.getContainerId();
                    UserInfo ui = mUserManager.getUserInfo(cid);
                    if (containerExportsContacts(cid)
                            != ContainerConstants.MergeContacts.ENCRYPTED
                            || (ui != null && uid != ui.containerOwner)) {
                        return null;
                    }
                    Uri proxyUri = buildProxyUri(uri, cid);
                    cursorArray[0] = getContext().getContentResolver().query(proxyUri, projection,
                            selection, selectionArgs, sortOrder, cancellationSignal);
                    break;
                }
            } else { // Otherwise query all containers
                cursorArray = new Cursor[containers.size() + 1];
                for (int i = 0; i < containers.size(); i++) {
                    ContainerInfo container = containers.get(i);
                    int cid = container.getContainerId();
                    UserInfo ui = mUserManager.getUserInfo(cid);
                    if (containerExportsContacts(cid)
                            != ContainerConstants.MergeContacts.ENCRYPTED
                            || (ui != null && uid != ui.containerOwner)) {
                        continue;
                    }
                    Uri proxyUri = buildProxyUri(uri, cid);
                    containerCursor = getContext().getContentResolver().query(proxyUri, projection,
                            selection, selectionArgs, sortOrder, cancellationSignal);
                    if (containerCursor != null) {
                        cursorArray[i + 1] = containerCursor;
                    }
                }
            }
        }
        return cursorArray;
    }

    private UserManager getUserManager() {
        if (mUserManager == null) {
            mUserManager = (UserManager) getContext().getSystemService(Context.USER_SERVICE);
        }
        if (mUserManager == null) {
            Log.e(TAG, "Failed to retrieve a UserManager instance.");
        }
        return mUserManager;
    }
}
