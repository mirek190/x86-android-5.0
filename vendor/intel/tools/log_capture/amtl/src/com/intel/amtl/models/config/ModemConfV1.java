/* Android AMTL
 *
 * Copyright (C) Intel 2014
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

package com.intel.amtl.models.config;

import android.os.Bundle;

import com.intel.amtl.mts.MtsConf;

public class ModemConfV1 extends ModemConf{

    public ModemConfV1(LogOutput config) {
        super(config);
    }

    public ModemConfV1(Bundle bundle) {
        super(bundle);
        super.setTrace(bundle.getString(ModemConf.KEY_TRACE));
        if (!bundle.getString(ModemConf.KEY_TRACE).equals("AT+TRACE=1\r\n")) {
            super.setMtsConf(new MtsConf());
        }
    }

    @Override
    public boolean confTraceEnable() {
        return !getTrace().equals("0");
    }

    @Override
    public boolean confTraceOutput() {
        return !getTrace().equals("");
    }
}
