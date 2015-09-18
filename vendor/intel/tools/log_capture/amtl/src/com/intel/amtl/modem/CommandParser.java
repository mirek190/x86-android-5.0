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

package com.intel.amtl.modem;

import android.util.Log;

import com.intel.amtl.AMTLApplication;
import com.intel.amtl.exceptions.ModemControlException;
import com.intel.amtl.models.config.Master;

import java.io.IOException;
import java.util.ArrayList;

public class CommandParser {

    private final String TAG = AMTLApplication.getAMTLApp().getLogTag();
    private final String MODULE = "CommandParser";

    public String parseXsioResponse(String xsio) {
        int indexXsio = xsio.indexOf("+XSIO: ");
        return xsio.substring(indexXsio + 7, indexXsio + 8);
    }

    public String parseMasterResponse(String xsystrace, String master) {
        int indexOfMaster;
        String masterResponse = "";
        int sizeOfMaster = master.length();

        if (sizeOfMaster <= 0) {
            Log.e(TAG, MODULE + ": cannot parse at+xsystrace=10 response");
        } else {
            indexOfMaster = xsystrace.indexOf(master);
            if ((indexOfMaster == -1)) {
                Log.e(TAG, MODULE + ": cannot parse at+xsystrace=10 response for master: "
                        + master);
            } else {
                String sub = xsystrace.substring(indexOfMaster + sizeOfMaster + 2);
                masterResponse = (sub.substring(0, sub.indexOf("\r\n"))).toUpperCase();
            }
        }
        return masterResponse;
    }

    public ArrayList<Master> parseXsystraceResponse(String xsystrace,
            ArrayList<Master> masterArray) {
        if (masterArray != null) {
            for (Master m: masterArray) {
                m.setDefaultPort(parseMasterResponse(xsystrace, m.getName()));
            }
        }
        return masterArray;
    }

    public String parseOct(String xsystrace) {
        String oct = "";

        if (xsystrace != null) {
            int indexOfOct = xsystrace.indexOf("mode");
            String sub = xsystrace.substring(indexOfOct + 5);
            oct = sub.substring(0, 1);
        }
        return oct;
    }

    public String parseTraceResponse(String trace) {
        int indexTrace = trace.indexOf("+TRACE: ");
        return trace.substring(indexTrace + 8, indexTrace + 9);
    }
}
