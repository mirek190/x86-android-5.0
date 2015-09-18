/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.sdkuilib.internal.repository.ui;

import com.android.sdklib.SdkManager;
import com.android.sdklib.SdkManagerTestCase;
import com.android.sdklib.internal.repository.MockDownloadCache;
import com.android.sdklib.internal.repository.updater.ISettingsPage;
import com.android.sdklib.repository.SdkRepoConstants;
import com.android.sdkuilib.internal.repository.MockSwtUpdaterData;

import java.io.File;
import java.util.Arrays;

public class SdkManagerUpgradeTest extends SdkManagerTestCase {

    @Override
    public void setUp() throws Exception {
        super.setUp();
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    /**
     * Create a mock page and list the current SDK state
     */
    public void testPackagesPage1() throws Exception {
        SdkManager sdkman = getSdkManager();
        File siArm = makeSystemImageFolder(TARGET_DIR_NAME_0, "tag-1", "armeabi");
        makeFakeSkin(siArm, "Tag1ArmSkin");

        MockSwtUpdaterData updaterData = new MockSwtUpdaterData(sdkman);
        MockDownloadCache cache = (MockDownloadCache) updaterData.getDownloadCache();
        updaterData.setupDefaultSources();

        MockPackagesPageImpl pageImpl = new MockPackagesPageImpl(updaterData);
        pageImpl.postCreate();
        pageImpl.performFirstLoad();

        // We have no network access possible and no mock download cache items.
        // The only thing visible in the display are the local packages as set by
        // the fake locally-installed SDK.
        String actual = pageImpl.getMockTreeDisplay();
        assertEquals(
                "[]    Tools                           |  |            |          \n" +
                " L_[] Android SDK Tools               |  |      1.0.1 | Installed\n" +
                " L_[] Android SDK Platform-tools      |  |     17.1.2 | Installed\n" +
                " L_[] Android SDK Build-tools         |  |      3.0.1 | Installed\n" +
                " L_[] Android SDK Build-tools         |  |          3 | Installed\n" +
                "[]    Tools (Preview Channel)         |  |            |          \n" +
                " L_[] Android SDK Build-tools         |  | 18.3.4 rc5 | Installed\n" +
                "[]    Android 0.0 (API 0)             |  |            |          \n" +
                " L_[] SDK Platform                    |  |          1 | Installed\n" +
                " L_[] Sys-Img v0 for (Tag 1, armeabi) |  |          0 | Installed\n" +
                " L_[] Sources for Android SDK         |  |          0 | Installed\n" +
                "[]    Extras                          |  |            |          ",
                actual);

        assertEquals(
                "[]",  // there are no direct downloads till we try to install.
                Arrays.toString(cache.getDirectHits()));
        assertEquals(
                "[<https://dl-ssl.google.com/android/repository/addons_list-1.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/addons_list-2.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-10.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-5.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-6.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-7.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-8.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-9.xml : 2>, " +
                 "<https://dl-ssl.google.com/android/repository/repository.xml : 2>]",
                Arrays.toString(cache.getCachedHits()));


        // Now prepare a tools update on the server and reload, with previews disabled.
        setupToolsXml1(cache);
        cache.clearDirectHits();
        cache.clearCachedHits();
        updaterData.overrideSetting(ISettingsPage.KEY_ENABLE_PREVIEWS, false);
        pageImpl.fullReload();

        actual = pageImpl.getMockTreeDisplay();
        assertEquals(
                "[]    Tools                           |  |            |                                   \n" +
                " L_[] Android SDK Tools               |  |      1.0.1 | Update available: rev. 20.0.3     \n" +
                " L_[] Android SDK Platform-tools      |  |     17.1.2 | Update available: rev. 18         \n" +
                " L_[] Android SDK Build-tools         |  |         18 | Not installed                     \n" +
                " L_[] Android SDK Build-tools         |  |      3.0.1 | Installed                         \n" +
                " L_[] Android SDK Build-tools         |  |          3 | Installed                         \n" +
                "[]    Tools (Preview Channel)         |  |            |                                   \n" +
                // Note: locally installed previews are always shown, even when enable previews is false.
                " L_[] Android SDK Build-tools         |  | 18.3.4 rc5 | Update available: rev. 18.3.4 rc15\n" +
                "[]    Android 0.0 (API 0)             |  |            |                                   \n" +
                " L_[] SDK Platform                    |  |          1 | Installed                         \n" +
                " L_[] Sys-Img v0 for (Tag 1, armeabi) |  |          0 | Installed                         \n" +
                " L_[] Sources for Android SDK         |  |          0 | Installed                         \n" +
                "[]    Extras                          |  |            |                                   ",
                actual);

        assertEquals(
                "[]",  // there are no direct downloads till we try to install.
                Arrays.toString(cache.getDirectHits()));
        assertEquals(
                "[<https://dl-ssl.google.com/android/repository/repository-10.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-5.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-6.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-7.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-8.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository-9.xml : 1>, " +
                 "<https://dl-ssl.google.com/android/repository/repository.xml : 1>]",
                Arrays.toString(cache.getCachedHits()));


        // We should get the same display if we restart the manager page from scratch
        // (e.g. simulate a first load)

        cache.clearDirectHits();
        cache.clearCachedHits();
        pageImpl = new MockPackagesPageImpl(updaterData);
        pageImpl.postCreate();
        pageImpl.performFirstLoad();

        actual = pageImpl.getMockTreeDisplay();
        assertEquals(
                "[]    Tools                           |  |            |                                   \n" +
                " L_[] Android SDK Tools               |  |      1.0.1 | Update available: rev. 20.0.3     \n" +
                " L_[] Android SDK Platform-tools      |  |     17.1.2 | Update available: rev. 18         \n" +
                " L_[] Android SDK Build-tools         |  |         18 | Not installed                     \n" +
                " L_[] Android SDK Build-tools         |  |      3.0.1 | Installed                         \n" +
                " L_[] Android SDK Build-tools         |  |          3 | Installed                         \n" +
                "[]    Tools (Preview Channel)         |  |            |                                   \n" +
                " L_[] Android SDK Build-tools         |  | 18.3.4 rc5 | Update available: rev. 18.3.4 rc15\n" +
                "[]    Android 0.0 (API 0)             |  |            |                                   \n" +
                " L_[] SDK Platform                    |  |          1 | Installed                         \n" +
                " L_[] Sys-Img v0 for (Tag 1, armeabi) |  |          0 | Installed                         \n" +
                " L_[] Sources for Android SDK         |  |          0 | Installed                         \n" +
                "[]    Extras                          |  |            |                                   ",
                actual);

        assertEquals(
                "[]",  // there are no direct downloads till we try to install.
                Arrays.toString(cache.getDirectHits()));
        assertEquals(
               "[<https://dl-ssl.google.com/android/repository/repository-10.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-5.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-6.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-7.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-8.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-9.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository.xml : 1>]",
                Arrays.toString(cache.getCachedHits()));


        // Now simulate a reload but this time enable previews.

        cache.clearDirectHits();
        cache.clearCachedHits();
        pageImpl = new MockPackagesPageImpl(updaterData);
        pageImpl.postCreate();
        updaterData.overrideSetting(ISettingsPage.KEY_ENABLE_PREVIEWS, true);
        pageImpl.performFirstLoad();

        actual = pageImpl.getMockTreeDisplay();
        assertEquals(
                "[]    Tools                           |  |            |                                   \n" +
                " L_[] Android SDK Tools               |  |      1.0.1 | Update available: rev. 20.0.3     \n" +
                " L_[] Android SDK Platform-tools      |  |     17.1.2 | Update available: rev. 18         \n" +
                " L_[] Android SDK Build-tools         |  |         18 | Not installed                     \n" +
                " L_[] Android SDK Build-tools         |  |      3.0.1 | Installed                         \n" +
                " L_[] Android SDK Build-tools         |  |          3 | Installed                         \n" +
                "[]    Tools (Preview Channel)         |  |            |                                   \n" +
                " L_[] Android SDK Build-tools         |  | 18.3.4 rc5 | Update available: rev. 18.3.4 rc15\n" +
                "[]    Android 0.0 (API 0)             |  |            |                                   \n" +
                " L_[] SDK Platform                    |  |          1 | Installed                         \n" +
                " L_[] Sys-Img v0 for (Tag 1, armeabi) |  |          0 | Installed                         \n" +
                " L_[] Sources for Android SDK         |  |          0 | Installed                         \n" +
                "[]    Extras                          |  |            |                                   ",
                actual);

        assertEquals(
                "[]",  // there are no direct downloads till we try to install.
                Arrays.toString(cache.getDirectHits()));
        assertEquals(
               "[<https://dl-ssl.google.com/android/repository/repository-10.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-5.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-6.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-7.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-8.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository-9.xml : 1>, " +
                "<https://dl-ssl.google.com/android/repository/repository.xml : 1>]",
                Arrays.toString(cache.getCachedHits()));
    }

    private void setupToolsXml1(MockDownloadCache cache) throws Exception {
        String repoXml =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
            "<sdk:sdk-repository xmlns:sdk=\"http://schemas.android.com/sdk/android/repository/10\" " +
            "                    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n" +
            "<sdk:license id=\"android-sdk-license\" type=\"text\">Blah blah blah.</sdk:license>\n" +
            "\n" +
            "<sdk:build-tool>\n" +
            "    <sdk:revision>\n" +
            "        <sdk:major>18</sdk:major>\n" +
            "    </sdk:revision>\n" +
            "    <sdk:archives>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>windows</sdk:host-os>\n" +
            "            <sdk:size>11159472</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6028258d8f2fba14d8b40c3cf507afa0289aaa13</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-windows.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>linux</sdk:host-os>\n" +
            "            <sdk:size>10985068</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6e2bc329c9485eb383172cbc2cde8b0c0cd1843f</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-linux.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>macosx</sdk:host-os>\n" +
            "            <sdk:size>11342461</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">4a015090c6a209fc33972acdbc65745e0b3c08b9</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-macosx.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "    </sdk:archives>\n" +
            "</sdk:build-tool>\n" +
            "\n" +
            "<sdk:build-tool>\n" +
            "    <sdk:revision>\n" +
            "        <sdk:major>18</sdk:major>\n" +
            "        <sdk:minor>3</sdk:minor>\n" +
            "        <sdk:micro>4</sdk:micro>\n" +
            "        <sdk:preview>15</sdk:preview>\n" +
            "    </sdk:revision>\n" +
            "    <sdk:archives>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>windows</sdk:host-os>\n" +
            "            <sdk:size>11159472</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6028258d8f2fba14d8b40c3cf507afa0289aaa13</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-windows.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>linux</sdk:host-os>\n" +
            "            <sdk:size>10985068</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6e2bc329c9485eb383172cbc2cde8b0c0cd1843f</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-linux.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>macosx</sdk:host-os>\n" +
            "            <sdk:size>11342461</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">4a015090c6a209fc33972acdbc65745e0b3c08b9</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-macosx.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "    </sdk:archives>\n" +
            "</sdk:build-tool>\n" +
            "\n" +
            "<sdk:platform-tool>\n" +
            "    <sdk:revision>\n" +
            "        <sdk:major>18</sdk:major>\n" +
            "    </sdk:revision>\n" +
            "    <sdk:archives>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>windows</sdk:host-os>\n" +
            "            <sdk:size>11159472</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6028258d8f2fba14d8b40c3cf507afa0289aaa13</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-windows.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>linux</sdk:host-os>\n" +
            "            <sdk:size>10985068</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">6e2bc329c9485eb383172cbc2cde8b0c0cd1843f</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-linux.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>macosx</sdk:host-os>\n" +
            "            <sdk:size>11342461</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">4a015090c6a209fc33972acdbc65745e0b3c08b9</sdk:checksum>\n" +
            "            <sdk:url>platform-tools_r18-macosx.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "    </sdk:archives>\n" +
            "</sdk:platform-tool>\n" +
            "\n" +
            "<sdk:tool>\n" +
            "    <sdk:list-display>Tools 20 (list display string)</sdk:list-display>\n" +
            "    <sdk:revision>\n" +
            "        <sdk:major>20</sdk:major>\n" +
            "        <sdk:minor>0</sdk:minor>\n" +
            "        <sdk:micro>3</sdk:micro>\n" +
            "    </sdk:revision>\n" +
            "    <sdk:min-platform-tools-rev>\n" +
            "        <sdk:major>18</sdk:major>\n" +
            "    </sdk:min-platform-tools-rev>\n" +
            "    <sdk:archives>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>windows</sdk:host-os>\n" +
            "            <sdk:size>90272048</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">54fb94168e631e211910f88aa40c532205730dd4</sdk:checksum>\n" +
            "            <sdk:url>tools_r20.0.3-windows.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>linux</sdk:host-os>\n" +
            "            <sdk:size>82723559</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">09bc633b406ae81981e3a0db19426acbb01ef219</sdk:checksum>\n" +
            "            <sdk:url>tools_r20.0.3-linux.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "        <sdk:archive>\n" +
            "            <sdk:host-os>macosx</sdk:host-os>\n" +
            "            <sdk:size>58197071</sdk:size>\n" +
            "            <sdk:checksum type=\"sha1\">09cee5ff3226277a6f0c07dcd29cba4ffc2e1da4</sdk:checksum>\n" +
            "            <sdk:url>tools_r20.0.3-macosx.zip</sdk:url>\n" +
            "        </sdk:archive>\n" +
            "    </sdk:archives>\n" +
            "</sdk:tool>\n" +
            "\n" +
            "</sdk:sdk-repository>\n";

        String url = SdkRepoConstants.URL_GOOGLE_SDK_SITE +
           String.format(SdkRepoConstants.URL_DEFAULT_FILENAME, SdkRepoConstants.NS_LATEST_VERSION);

        cache.registerCachedPayload(url, repoXml.getBytes("UTF-8"));
    }

}
