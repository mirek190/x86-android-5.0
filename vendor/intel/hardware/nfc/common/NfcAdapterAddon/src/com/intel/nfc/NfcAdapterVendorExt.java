/*
 * Copyright (C) 2010 The Android Open Source Project
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
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2013 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

package com.intel.nfc;

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.nfc.NfcAdapter;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;

import java.io.IOException;
import java.lang.RuntimeException;

/**
 * Represents the local NFC adapter for PN547 Nfc adapter extentions.
 */
public final class NfcAdapterVendorExt  {
    static final String TAG = "Vendor NFC Extensions";

    public static final String SERVICE_NAME = "nfc.vendor_ext";

    private static INfcAdapterVendorExt sInterface = null;

    /**
     * Active Swp interface
     */
    public static void activeSwp() throws RemoteException {
        getInterface().activeSwp();
    }

    /**
     * Allows to enable/disable the EMV-CO profile from host or uicc or ese.
     * Note: When EMVCO profile is enabled the NFC Forum profile is disabled.
     *       At present only emv-co profile from DH only supported.
     *
     * <p>Requires {@link android.Manifest.permission#NFC} permission.
     *
     * @param enable enable/disable the emv-co profile
     * @param route the endpoint 0 for DH, 1 for UICC
     * @return 0 in case of success, otherwise fail.
     *
     * @throws IOException If a failure occurred during emvco profile setup.
     */
     public int enableEmvcoPolling(boolean enable, int route) throws IOException {
        int response = -1;

        // Perform Receive
        try {
            response = getInterface().setEmvCoPollProfile(enable, route);
        } catch (RemoteException e) {
            Log.e(TAG, "RemoteException in setEmvCoPollProfile(): ", e);
            throw new IOException("RemoteException in setEmvCoPollProfile()");
        }

        // Handle potential errors
        if (response < 0) throw new IOException("Emv-Co poll profile failed");

        return response;
    }

    /**
     * Returns the INfcAdapterVendorExt interface if available
     */
    public static INfcAdapterVendorExt getInterface() {
        if (sInterface == null) {
            IBinder b = ServiceManager.getService(SERVICE_NAME);
            if (b == null) 
               throw new RuntimeException("Cannot retrieve service :" + SERVICE_NAME);
            sInterface = INfcAdapterVendorExt.Stub.asInterface(b);
        }
        return sInterface;
    }
}
