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

package com.android.sdkmanager;


import com.android.SdkConstants;
import com.android.sdklib.IAndroidTarget;
import com.android.sdklib.SdkManager;
import com.android.sdklib.SdkManagerTestCase;
import com.android.sdklib.SystemImage;
import com.android.sdklib.internal.avd.AvdInfo;
import com.android.sdklib.internal.repository.CanceledByUserException;
import com.android.sdklib.internal.repository.DownloadCache;
import com.android.sdklib.internal.repository.DownloadCache.Strategy;
import com.android.sdklib.internal.repository.NullTaskMonitor;
import com.android.sdklib.repository.SdkAddonConstants;
import com.android.sdklib.repository.SdkRepoConstants;
import com.android.sdklib.repository.descriptors.PkgType;
import com.android.utils.Pair;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Set;
import java.util.TreeSet;

import junit.framework.AssertionFailedError;

/**
 * Tests the SDK Manager command-line tool using the temp fake SDK
 * created by {@link SdkManagerTestCase}.
 * <p/>
 * These tests directly access the internal methods of the SDK or AVD
 * manager as the Main should invoke them.
 * @see SdkManagerTest2 SdkManagerTest2 for command-line "black box" tests
 */
public class SdkManagerTest extends SdkManagerTestCase {

    private IAndroidTarget mTarget;
    private File mAvdFolder;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        // add 2 tag/abi folders with a new skin each
        File siX86 = makeSystemImageFolder(TARGET_DIR_NAME_0, "tag-1", "x86");
        makeFakeSkin(siX86, "Tag1X86Skin");
        File siArm = makeSystemImageFolder(TARGET_DIR_NAME_0, "tag-1", "armeabi");
        makeFakeSkin(siArm, "Tag1ArmSkin");

        mTarget = getSdkManager().getTargets()[0];
        mAvdFolder = AvdInfo.getDefaultAvdFolder(getAvdManager(), getName());
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
    }

    public void testDisplayEmptyAvdList() {
        Main main = new Main();
        main.setLogger(getLog());
        getLog().clear();
        main.displayAvdList(getAvdManager());
        assertEquals("P Available Android Virtual Devices:\n", getLog().toString());
    }

    public void testDisplayAvdList_OneNonSnapshot() {
        Main main = new Main();
        main.setLogger(getLog());
        getAvdManager().createAvd(
                mAvdFolder,
                this.getName(),
                mTarget,
                SystemImage.DEFAULT_TAG,
                SdkConstants.ABI_ARMEABI,
                null,   // skinFolder
                null,   // skinName
                null,   // sdName
                null,   // hardware properties
                null,   // bootProps
                false,  // createSnapshot
                false,  // removePrevious
                false,  // editExisting
                getLog());

        getLog().clear();
        main.displayAvdList(getAvdManager());
        assertEquals(
                "P Available Android Virtual Devices:\n" +
                "P     Name: " + this.getName() + "\n" +
                "P     Path: " + mAvdFolder + "\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n",
                getLog().toString());
    }

    public void testDisplayAvdList_OneSnapshot() {
        Main main = new Main();
        main.setLogger(getLog());

        getAvdManager().createAvd(
                mAvdFolder,
                this.getName(),
                mTarget,
                SystemImage.DEFAULT_TAG,
                SdkConstants.ABI_ARMEABI,
                null,   // skinFolder
                null,   // skinName
                null,   // sdName
                null,   // hardware properties
                null,   // bootProps
                true,   // createSnapshot
                false,  // removePrevious
                false,  // editExisting
                getLog());

        getLog().clear();
        main.displayAvdList(getAvdManager());
        assertEquals(
                "P Available Android Virtual Devices:\n" +
                "P     Name: " + this.getName() + "\n" +
                "P     Path: " + mAvdFolder + "\n" +
                "P   Target: Android 0.0 (API level 0)\n" +
                "P  Tag/ABI: default/armeabi\n" +
                "P     Skin: HVGA\n" +
                "P Snapshot: true\n",
                getLog().toString());
    }

    public void testDisplayTargetList() {
        Main main = new Main();
        main.setupForTest(getSdkManager(), getLog(), null);
        getLog().clear();
        main.displayTargetList();
        assertEquals(
                "P Available Android targets:\n" +
                "P ----------\n" +
                "P id: 1 or \"android-0\"\n" +
                "P      Name: Android 0.0\n" +
                "P      Type: Platform\n" +
                "P      API level: 0\n" +
                "P      Revision: 1\n" +
                "P      Skins: HVGA (default), Tag1ArmSkin, Tag1X86Skin\n" +
                "P  Tag/ABIs : default/armeabi, tag-1/armeabi, tag-1/x86\n",
                getLog().toString());
    }

    public void testDisplayTagAbiList() {
        Main main = new Main();
        main.setupForTest(getSdkManager(), getLog(), null);
        getLog().clear();
        main.displayTagAbiList(mTarget, "Tag/ABIs: ");
        assertEquals(
                "P Tag/ABIs: default/armeabi, tag-1/armeabi, tag-1/x86\n",
                getLog().toString());
    }

    public void testDisplaySkinList() {
        Main main = new Main();
        main.setupForTest(getSdkManager(), getLog(), null);
        getLog().clear();
        main.displaySkinList(mTarget, "Skins: ");
        assertEquals(
                "P Skins: HVGA (default), Tag1ArmSkin, Tag1X86Skin\n",
                getLog().toString());
    }

    public void testSdkManagerHasChanged() throws IOException {
        try {
            Main main = new Main();
            SdkManager sdkman = getSdkManager();
            main.setupForTest(getSdkManager(), getLog(), null);
            getLog().clear();

            assertFalse(sdkman.hasChanged(getLog()));
            getLog().clear();
            sdkman.getLocalSdk().getPkgsInfos(PkgType.PKG_ALL); // parse everything

            File addonsDir = new File(sdkman.getLocation(), SdkConstants.FD_ADDONS);
            assertTrue(addonsDir.isDirectory());

            FileWriter readme = new FileWriter(new File(addonsDir, "android.txt"));
            readme.write("test\n");
            readme.close();

            // Adding a file does alter sdk.hasChanged
            assertTrue(sdkman.hasChanged(getLog()));
            getLog().clear();
            // Once reloaded & reparsed, sdk.hasChanged is reset
            sdkman.reloadSdk(getLog());
            sdkman.getLocalSdk().getPkgsInfos(PkgType.PKG_ALL); // parse everything
            assertFalse(sdkman.hasChanged(getLog()));
            getLog().clear();

            File fakeAddon = new File(addonsDir, "google-addon");
            fakeAddon.mkdirs();
            File sourceProps = new File(fakeAddon, SdkConstants.FN_SOURCE_PROP);
            FileWriter propsWriter = new FileWriter(sourceProps);
            propsWriter.write("revision=7\n");
            propsWriter.close();

            // Adding a directory does alter sdk.hasChanged even if not a real add-on
            assertTrue(sdkman.hasChanged(getLog()));
            getLog().clear();
            sdkman.reloadSdk(getLog());
            sdkman.getLocalSdk().getPkgsInfos(PkgType.PKG_ALL); // parse everything
            assertFalse(sdkman.hasChanged(getLog()));
            getLog().clear();

            // Changing the source.properties file alters sdk.hasChanged
            propsWriter = new FileWriter(sourceProps);
            propsWriter.write("revision=8\n");
            propsWriter.close();
            assertTrue(sdkman.hasChanged(getLog()));
            getLog().clear();
            // Once reloaded, sdk.hasChanged will be reset
            sdkman.reloadSdk(getLog());
            sdkman.getLocalSdk().getPkgsInfos(PkgType.PKG_ALL); // parse everything
            assertFalse(sdkman.hasChanged(getLog()));
            getLog().clear();
        } catch (AssertionFailedError e) {
            String s = e.getMessage();
            if (s != null) {
                s += "\n";
            } else {
                s = "";
            }
            s += "Log:" + getLog().toString();
            AssertionFailedError e2 = new AssertionFailedError(s);
            e2.initCause(e);
            throw e2;
        }
    }

    public void testCheckFilterValues() {
        // These are the values we expect checkFilterValues() to match.
        String[] expectedValues = {
                "platform",
                "system-image",
                "tool",
                "platform-tool",
                "doc",
                "sample",
                "add-on",
                "extra",
                "source"
        };

        Set<String> expectedSet = new TreeSet<String>(Arrays.asList(expectedValues));

        // First check the values are actually defined in the proper arrays
        // in the Sdk*Constants.NODES
        for (String node : SdkRepoConstants.NODES) {
            assertTrue(
                String.format(
                    "Error: value '%1$s' from SdkRepoConstants.NODES should be used in unit-test",
                    node),
                expectedSet.contains(node));
        }
        for (String node : SdkAddonConstants.NODES) {
            assertTrue(
                String.format(
                    "Error: value '%1$s' from SdkAddonConstants.NODES should be used in unit-test",
                    node),
                expectedSet.contains(node));
        }

        // Now check none of these values are NOT present in the NODES arrays
        for (String node : SdkRepoConstants.NODES) {
            expectedSet.remove(node);
        }
        for (String node : SdkAddonConstants.NODES) {
            expectedSet.remove(node);
        }
        assertTrue(
            String.format(
                    "Error: values %1$s are missing from Sdk[Repo|Addons]Constants.NODES",
                    Arrays.toString(expectedSet.toArray())),
            expectedSet.isEmpty());

        // We're done with expectedSet now
        expectedSet = null;

        // Finally check that checkFilterValues accepts all these values, one by one.
        Main main = new Main();
        main.setLogger(getLog());

        for (int step = 0; step < 3; step++) {
            for (String value : expectedValues) {
                switch(step) {
                // step 0: use value as-is
                case 1:
                    // add some whitespace before and after
                    value = "  " + value + "   ";
                    break;
                case 2:
                    // same with some empty arguments that should get ignored
                    value = "  ," + value + " ,  ";
                    break;
                    }

                Pair<String, ArrayList<String>> result = main.checkFilterValues(value);
                assertNull(
                        String.format("Expected error to be null for value '%1$s', got: %2$s",
                                value, result.getFirst()),
                        result.getFirst());
                assertEquals(
                        String.format("[%1$s]", value.replace(',', ' ').trim()),
                        Arrays.toString(result.getSecond().toArray()));
            }
        }
    }

    public void testLocalFileDownload() throws IOException, CanceledByUserException {
        Main main = new Main();
        SdkManager sdkman = getSdkManager();
        main.setupForTest(getSdkManager(), getLog(), null);
        getLog().clear();

        IAndroidTarget target = sdkman.getTargets()[0];
        File sourceProps = new File(target.getLocation(), SdkConstants.FN_SOURCE_PROP);
        assertTrue(sourceProps.isFile());

        String urlStr = getFileUrl(sourceProps);
        assertTrue(urlStr.startsWith("file:///"));

        DownloadCache cache = new DownloadCache(Strategy.DIRECT);
        NullTaskMonitor monitor = new NullTaskMonitor(getLog());
        Pair<InputStream, Integer> result = cache.openDirectUrl(urlStr, monitor);
        assertNotNull(result);
        assertEquals(200, result.getSecond().intValue());

        int len = (int) sourceProps.length();
        byte[] buf = new byte[len];
        FileInputStream is = new FileInputStream(sourceProps);
        is.read(buf);
        is.close();
        String expected = new String(buf, "UTF-8");

        buf = new byte[len];
        result.getFirst().read(buf);
        result.getFirst().close();
        String actual = new String(buf, "UTF-8");
        assertEquals(expected, actual);
    }

    private String getFileUrl(File file) throws IOException {
        // Note: to create a file:// URL, one would typically use something like
        // f.toURI().toURL().toString(). However this generates a broken path on
        // Windows, namely "C:\\foo" is converted to "file:/C:/foo" instead of
        // "file:///C:/foo" (i.e. there should be 3 / after "file:"). So we'll
        // do the correct thing manually.

        String path = file.getCanonicalPath();
        if (File.separatorChar != '/') {
            path = path.replace(File.separatorChar, '/');
        }
        // A file:// should start with 3 // (2 for file:// and 1 to make it an absolute
        // path. On Windows that should look like file:///C:/. Linux/Mac will already
        // have that leading / in their path so we need to compensate for windows.
        if (!path.startsWith("/")) {
            path = "/" + path;
        }

        // For some reason the URL class doesn't add the mandatory "//" after
        // the "file:" protocol name, so it has to be hacked into the path.
        URL url = new URL("file", null, "//" + path);  //$NON-NLS-1$ //$NON-NLS-2$
        String result = url.toString();
        return result;
    }

}
