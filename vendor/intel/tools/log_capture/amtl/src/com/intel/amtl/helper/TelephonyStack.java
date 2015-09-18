/* Android AMTL
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
 */

package com.intel.amtl.helper;

import android.os.SystemProperties;

public class TelephonyStack {

    public static final String PERSIST_TELEPHONY = "persist.sys.telephony.off";
    private String Enabled = "0";
    private String Disabled = "1";

    public boolean isEnabled() {
        String value = SystemProperties.get(PERSIST_TELEPHONY, Enabled);
        boolean state = true;
        if (value.equals(Disabled)) {
            state = false;
        }
        return state;
    }

    public void enableStack() {
        SystemProperties.set(PERSIST_TELEPHONY, Enabled);
    }

    public void disableStack() {
        SystemProperties.set(PERSIST_TELEPHONY, Disabled);
    }
}
