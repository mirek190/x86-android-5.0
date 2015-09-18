/* Android Modem NVM Configuration Tool
 *
 * Copyright (C) Intel 2012
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
 * Author: Edward Marmounier <edwardx.marmounier@intel.com>
 */

package com.intel.android.nvmconfig.helpers;

import com.intel.android.nvmconfig.models.config.Config;
import com.intel.android.nvmconfig.models.config.Variant;
import com.intel.android.nvmconfig.models.modem.ModemControlCommandList;
import com.intel.android.nvmconfig.models.modem.at.ATControlCommand;

public class CommandHelper {

    public static ModemControlCommandList<ATControlCommand> extractCommandsFromVariant(Variant variant) {

        ModemControlCommandList<ATControlCommand> ret = new ModemControlCommandList<ATControlCommand>();

        if (variant != null) {
            for (int i = 0; i < variant.getConfigs().size(); i++) {
                ret.getCommands().addAll(CommandHelper.extractCommandsFromConfig(variant.getConfigs().get(i)).getCommands());
            }
        }
        return ret;
    }

    public static ModemControlCommandList<ATControlCommand> extractCommandsFromConfig(Config config) {

        ModemControlCommandList<ATControlCommand> ret = new ModemControlCommandList<ATControlCommand>();

        if (config != null) {
            for (int i = 0; i < config.getCommands().size(); i++) {
                ATControlCommand command = new ATControlCommand(config.getCommands().get(i).getValue());
                ret.getCommands().add(command);
            }
        }
        return ret;
    }
}
