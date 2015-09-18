/* Android Modem Traces and Logs
 *
 * Copyright (C) Intel 2013
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
 *
 * Author: Morgane Butscher <morganeX.butscher@intel.com>
 */

package com.intel.amtl.mts;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.R;

public class MtsProperties {
    private static AMTLApplication mApp = AMTLApplication.getAMTLApp();

    public static String getInput() {
        return mApp.getString(R.string.mts_input_prop);
    }

    public static String getOutput() {
        return mApp.getString(R.string.mts_output_prop);
    }

    public static String getOutputType() {
        return mApp.getString(R.string.mts_output_type_prop);
    }

    public static String getRotateNum() {
        return mApp.getString(R.string.mts_rotate_num_prop);
    }

    public static String getRotateSize() {
        return mApp.getString(R.string.mts_rotate_size_prop);
    }

    public static String getInterface() {
        return mApp.getString(R.string.mts_interface_prop);
    }

    public static String getBufferSize() {
        return mApp.getString(R.string.mts_buffer_size_prop);
    }

    public static String getService() {
        return mApp.getString(R.string.mts_service);
    }

    public static String getStatus() {
        return mApp.getString(R.string.mts_status_prop);
    }
}
