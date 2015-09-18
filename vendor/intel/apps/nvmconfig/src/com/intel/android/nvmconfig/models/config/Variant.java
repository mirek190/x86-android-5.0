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

package com.intel.android.nvmconfig.models.config;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;


public class Variant implements Serializable {

    private static final long serialVersionUID = -2715463387978803963L;

    private Integer id = 0;
    private String name = "";
    private String description = "";
    private String creationDate = "";
    private String onCompleteMsg = "";
    private boolean verbose = false;
    private ArrayList<Config> configs = new ArrayList<Config>();

    public Variant() {
    }

    public Variant(Integer id, String name, String description) {

        this.setId(id);
        this.setName(name);
        this.setDescription(description);
    }

    public Integer getId() {
        return this.id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    public String getName() {
        return this.name;
    }

    public void setName(String name) {

        this.name = name;
    }

    public String getDescription() {
        return this.description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getCreationDate() {
        return this.creationDate;
    }

    public void setCreationDate(String creationDate) {
        this.creationDate = creationDate;
    }

    public List<Config> getConfigs() {
        return this.configs;
    }

    public String getOnCompleteMsg() {
        return this.onCompleteMsg;
    }

    public void setOnCompleteMsg(String onCompleteMsg) {
        this.onCompleteMsg = onCompleteMsg;
    }

    public boolean isVerbose() {
        return this.verbose;
    }

    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }
}
