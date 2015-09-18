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

import android.os.SystemProperties;
import android.util.Log;

public class MtsConf {

    private String mtsInput = "";
    private String mtsOutput = "";
    private String mtsOutputType = "";
    private String mtsRotateNum = "";
    private String mtsRotateSize = "";
    private String mtsInterface = "";
    private String mtsBufferSize = "";


    public MtsConf(String input, String output, String outputType, String rotateNum,
            String rotateSize, String interFace, String bufferSize) {
        this.mtsInput = (input != null) ? input : this.mtsInput;
        this.mtsOutput = (output != null) ? output : this.mtsOutput;
        this.mtsOutputType = (outputType != null) ? outputType : this.mtsOutputType;
        this.mtsRotateNum = (rotateNum != null) ? rotateNum : this.mtsRotateNum;
        this.mtsRotateSize = (rotateSize != null) ? rotateSize : this.mtsRotateSize;
        this.mtsInterface = (interFace != null) ? interFace : this.mtsInterface;
        this.mtsBufferSize = (bufferSize != null) ? bufferSize : this.mtsBufferSize;
    }

    public MtsConf() {
    }

    public String getInput() {
        return this.mtsInput;
    }

    public String getOutput() {
        return this.mtsOutput;
    }

    public String getOutputType() {
        return this.mtsOutputType;
    }

    public String getRotateNum() {
        return this.mtsRotateNum;
    }

    public String getRotateSize() {
        return this.mtsRotateSize;
    }

    public String getInterface() {
        return this.mtsInterface;
    }

    public String getBufferSize() {
        return this.mtsBufferSize;
    }

    public void applyParameters() {

        SystemProperties.set(MtsProperties.getInput(), this.mtsInput);
        SystemProperties.set(MtsProperties.getOutput(), this.mtsOutput);
        SystemProperties.set(MtsProperties.getOutputType(), this.mtsOutputType);
        SystemProperties.set(MtsProperties.getRotateNum(), this.mtsRotateNum);
        SystemProperties.set(MtsProperties.getRotateSize(), this.mtsRotateSize);
        SystemProperties.set(MtsProperties.getInterface(), this.mtsInterface);
        SystemProperties.set(MtsProperties.getBufferSize(), this.mtsBufferSize);
    }
}
