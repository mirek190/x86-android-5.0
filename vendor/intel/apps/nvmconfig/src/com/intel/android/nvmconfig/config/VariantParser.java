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

package com.intel.android.nvmconfig.config;

import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;

import android.util.Log;
import android.util.Xml;

import com.intel.android.nvmconfig.models.config.Command;
import com.intel.android.nvmconfig.models.config.Config;
import com.intel.android.nvmconfig.models.config.Variant;

public class VariantParser {

    public Variant parseVariant(InputStream inputStream) throws XmlPullParserException, IOException {

        Variant ret = null;

        XmlPullParser parser = Xml.newPullParser();
        int eventType = 0;

        parser.setInput(inputStream, null);

        eventType = parser.getEventType();

        while (eventType != XmlPullParser.END_DOCUMENT) {

            switch (eventType) {
            case XmlPullParser.START_TAG:
                ret = this.handleVariantElement(parser);
                break;
            }
            eventType = parser.next();
        }
        return ret;
    }

    public List<Variant> parseVariants(File directory) throws XmlPullParserException {

        ArrayList<Variant> ret = new ArrayList<Variant>();

        if (directory != null && directory.isDirectory()) {

            File[] files = directory.listFiles(new FileFilter() {

                public boolean accept(File pathname) {
                    return pathname.getName().endsWith(".xml");
                }
            });
            for (int i = 0; files != null && i < files.length; i++) {

                FileInputStream inputStream = null;

                try {
                    inputStream = new FileInputStream(files[i]);
                    Variant variant = this.parseVariant(inputStream);
                    if (variant != null) {
                        ret.add(variant);
                    }
                }
                catch (IOException ex) {
                    Log.e("VariantParser", ex.toString());
                }
                finally {
                    if (inputStream != null) {
                        try { inputStream.close(); } catch (IOException ex) { Log.e("VariantParser", ex.toString()); }
                    }
                }
            }
        }
        else {
            throw new IllegalArgumentException("directory");
        }
        return ret;
    }

    private Variant handleVariantElement(XmlPullParser parser) throws XmlPullParserException, IOException {

        Variant ret = null;

        if (isStartOf(parser, "variant")) {

            ret = new Variant();
            ret.setId(Integer.parseInt(parser.getAttributeValue(null, "id")));
            ret.setName(parser.getAttributeValue(null, "name"));
            ret.setCreationDate(parser.getAttributeValue(null, "creationDate"));
            ret.setOnCompleteMsg(parser.getAttributeValue(null, "onCompleteMsg"));
            ret.setVerbose(Boolean.parseBoolean(parser.getAttributeValue(null, "verbose")));
            ret.setDescription(parser.getAttributeValue(null, "description"));

            while (!isEndOf(parser, "variant")) {

                List<Config> configs = this.handleConfigElements(parser);
                if (configs != null) {
                    ret.getConfigs().addAll(configs);
                }
                parser.next();
            }
        }
        return ret;
    }

    private Command handleCommandElement(XmlPullParser parser) throws XmlPullParserException, IOException {

        Command ret = null;

        if (isStartOf(parser, "command")) {

            ret = new Command();
            ret.setType(parser.getAttributeValue(null, "type"));
            ret.setValue(parser.nextText());
        }
        return ret;
    }

    private Config handleConfigElement(XmlPullParser parser) throws XmlPullParserException, IOException {

        Config ret = null;

        if (isStartOf(parser, "config")) {

            ret = new Config();
            ret.setId(Integer.parseInt(parser.getAttributeValue(null, "id")));
            ret.setName(parser.getAttributeValue(null, "name"));
            ret.setDescription(parser.getAttributeValue(null, "description"));

            while (!isEndOf(parser, "config")) {

                List<Command> commands = this.handleCommandElements(parser);
                if (commands != null) {
                    ret.getCommands().addAll(commands);
                }
                parser.next();
            }
        }
        return ret;
    }

    private List<Config> handleConfigElements(XmlPullParser parser) throws XmlPullParserException, IOException {

        ArrayList<Config> ret = null;

        if (isStartOf(parser, "configs")) {

            ret = new ArrayList<Config>();

            while (!isEndOf(parser, "configs")) {

                Config config = this.handleConfigElement(parser);
                if (config != null) {
                    ret.add(config);
                }
                parser.next();
            }
        }
        return ret;
    }

    private List<Command> handleCommandElements(XmlPullParser parser) throws XmlPullParserException, IOException {

        ArrayList<Command> ret = null;

        if (isStartOf(parser, "commands")) {

            ret = new ArrayList<Command>();
            while (!isEndOf(parser, "commands")) {

                Command command = this.handleCommandElement(parser);
                if (command != null) {
                    ret.add(command);
                }
                parser.next();
            }
        }
        return ret;
    }

    private static boolean isEndOf(XmlPullParser parser, String tagName) throws XmlPullParserException {

        return (parser.getEventType() == XmlPullParser.END_TAG && tagName.equalsIgnoreCase(parser.getName()));
    }

    private static boolean isStartOf(XmlPullParser parser, String tagName) throws XmlPullParserException {

        return (parser.getEventType() == XmlPullParser.START_TAG && tagName.equalsIgnoreCase(parser.getName()));
    }
}
