/*
* aidl file : frameworks/base/core/java/com/intel/arkham/IContainerManager.aidl
* This file contains definitions of functions which are exposed by service
*/
package com.intel.arkham;

import java.util.List;

import android.accounts.Account;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;

import com.intel.arkham.ContainerInfo;

interface IContainerManager {
    int createContainer(in int containerType, in int options, in String containerName,
            in String mdmPackageName, in Bundle bundle);

    ContainerInfo getContainerFromCid(int cid);

    ContainerInfo getContainerFromCGid(int cGid);

    ContainerInfo getContainerFromName(String containerName);

    String getLauncherPackageName(int cid);

    boolean enableContainer();

    boolean wipeOrDisableContainer(in int cid);

    boolean disableContainer(in int cid);

    boolean removeContainer(in int cid);

    int unlockContainer(in byte[] passwordHash, in int cid);

    void lockContainer(int cid);

    void lockContainerNow(int cid);

    boolean changePassword(in int cid, in byte[] newPasswordHash, in int passwordType);

    List listContainers();

    int getContainerOwnerId(in int cid);

    boolean isContainerActive(int cid);

    boolean isContainerOpened(int cid);

    boolean isContainerDisabled(int cid);

    boolean isContainerInitialized(int cid);

    int installApplicationToContainer(int cid, String apkFilePath);

    void installApplication(String apkFilePath);

    int installOwnerUserApplication(int cid, String pkgName);

    boolean isApplicationRemovable(int cid, String pkgName);

    int removeApplication(int cid, String pkgName);

    int createContainerUser(in int cid, in String containerName);

    boolean removeContainerUser(in int cid);

    int unmountInternalStorageForAllContainers();

    int setContainerPolicy(int cid, in Bundle bundle);

    boolean sendDeviceAdminBroadcast(in Intent intent);

    void policyUpdateResult(int cid, in Bundle result);

    int mountContainerSystemData(int cid, in byte[] passwordHash);

    int unmountContainerSystemData(int cid);

    boolean isContainerSystemDataMounted(in int cid);

    int getPasswordType(int cid);

    int getMergeContactsPolicy(int cid);

    boolean setMergeContactsPolicy(int cid, int policy);

    boolean isContainerUserMode();

    void setContainerUserMode();
    void resetContainerUserMode();

    void setsContainerUserId(int userId);
    int getsContainerUserId();

    /*@hide*/
    boolean isContainerUser(in int userId);

    String getContainerMdmPackageName();

    boolean isTopRunningActivityInContainer(int cid);

    String getLauncherBackgroundImagePath(int cid);

    boolean setLauncherBackgroundImagePath(int cid, String image);

    void actionLauncherBackgroundSet(int cid);

    void actionPasswordExpiring(int cid, String message, String contentText);

    void actionPasswordChanged(int cid);

    Account[] getContainerAccounts(in Account[] accounts);

    int getPasswordMaxAttempts(int cid);

    boolean setPasswordMaxAttempts(int cid, int maximum);

    String getPasswordMaxAttemptsAction(int cid);

    boolean setPasswordMaxAttemptsAction(int cid, String action);

    void logContainerUnmountedAccess(int userId, in String path);

    int isContainerAccount(String accountName);

    Bundle getSystemWhitelist();

    List<ActivityInfo> getAvailableAppsForDeletion(int cid);

    boolean isContainerLauncherInstalled(int cid);

    boolean installContainerLauncher(int cid);

    void rebootDevice(int cid, String reason);
}
