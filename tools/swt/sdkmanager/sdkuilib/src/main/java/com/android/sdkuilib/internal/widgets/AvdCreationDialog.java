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

package com.android.sdkuilib.internal.widgets;

import com.android.SdkConstants;
import com.android.annotations.NonNull;
import com.android.annotations.Nullable;
import com.android.prefs.AndroidLocation.AndroidLocationException;
import com.android.resources.Density;
import com.android.resources.ScreenSize;
import com.android.sdklib.IAndroidTarget;
import com.android.sdklib.ISystemImage;
import com.android.sdklib.SystemImage;
import com.android.sdklib.devices.Camera;
import com.android.sdklib.devices.CameraLocation;
import com.android.sdklib.devices.Device;
import com.android.sdklib.devices.DeviceManager;
import com.android.sdklib.devices.Hardware;
import com.android.sdklib.devices.Screen;
import com.android.sdklib.devices.Software;
import com.android.sdklib.devices.Storage;
import com.android.sdklib.internal.avd.AvdInfo;
import com.android.sdklib.internal.avd.AvdManager;
import com.android.sdklib.internal.avd.AvdManager.AvdConflict;
import com.android.sdklib.internal.avd.HardwareProperties;
import com.android.sdklib.repository.descriptors.IdDisplay;
import com.android.sdklib.repository.local.LocalSdk;
import com.android.sdkuilib.internal.repository.icons.ImageFactory;
import com.android.sdkuilib.ui.GridDialog;
import com.android.utils.ILogger;
import com.android.utils.Pair;
import com.google.common.base.Joiner;

import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AvdCreationDialog extends GridDialog {

    private AvdManager mAvdManager;
    private ImageFactory mImageFactory;
    private ILogger mSdkLog;
    private AvdInfo mAvdInfo;

    private final TreeMap<String, IAndroidTarget> mCurrentTargets =
            new TreeMap<String, IAndroidTarget>();

    private Button mOkButton;

    private Text mAvdName;

    private Combo mDevice;

    private Combo mTarget;
    private Combo mTagAbi;

    private Button mKeyboard;
    private Combo mSkinCombo;

    private Combo mFrontCamera;
    private Combo mBackCamera;

    private Button mSnapshot;
    private Button mGpuEmulation;

    private Text mRam;
    private Text mVmHeap;

    private Text mDataPartition;
    private Combo mDataPartitionSize;

    private Button mSdCardSizeRadio;
    private Text mSdCardSize;
    private Combo mSdCardSizeCombo;
    private Button mSdCardFileRadio;
    private Text mSdCardFile;
    private Button mBrowseSdCard;

    private Button mForceCreation;
    private Composite mStatusComposite;

    private Label mStatusIcon;
    private Label mStatusLabel;

    private Device mInitWithDevice;
    private AvdInfo mCreatedAvd;

    private static final AvdSkinChoice SKIN_DYNAMIC =
        new AvdSkinChoice(SkinType.DYNAMIC, "Skin with dynamic hardware controls");
    private static final AvdSkinChoice SKIN_NONE =
        new AvdSkinChoice(SkinType.NONE, "No skin");

    private final List<AvdSkinChoice> mCurrentSkinData = new ArrayList<AvdSkinChoice>();
    private final List<ISystemImage> mCurrentSystemImages = new ArrayList<ISystemImage>();

    /**
     * {@link VerifyListener} for {@link Text} widgets that should only contains
     * numbers.
     */
    private final VerifyListener mDigitVerifier = new VerifyListener() {
        @Override
        public void verifyText(VerifyEvent event) {
            int count = event.text.length();
            for (int i = 0; i < count; i++) {
                char c = event.text.charAt(i);
                if (c < '0' || c > '9') {
                    event.doit = false;
                    return;
                }
            }
        }
    };

    public AvdCreationDialog(Shell shell,
            AvdManager avdManager,
            ImageFactory imageFactory,
            ILogger log,
            AvdInfo editAvdInfo) {
        super(shell, 2, false);
        mAvdManager = avdManager;
        mImageFactory = imageFactory;
        mSdkLog = log;
        mAvdInfo = editAvdInfo;

        setShellStyle(getShellStyle() | SWT.RESIZE);
    }

    /** Returns the AVD Created, if successful. */
    public AvdInfo getCreatedAvd() {
        return mCreatedAvd;
    }

    @Override
    protected Control createContents(Composite parent) {
        Control control = super.createContents(parent);
        getShell().setMinimumSize(new Point(350, 600));
        getShell().setText(mAvdInfo == null ? "Create new Android Virtual Device (AVD)"
                                            : "Edit Android Virtual Device (AVD)");

        mOkButton = getButton(IDialogConstants.OK_ID);

        if (mAvdInfo != null) {
            fillExistingAvdInfo(mAvdInfo);
        } else if (mInitWithDevice != null) {
            fillInitialDeviceInfo(mInitWithDevice);
        }

        validatePage();
        return control;
    }

    @Override
    public void createDialogContent(Composite parent) {

        Label label;
        String tooltip;
        ValidateListener validateListener = new ValidateListener();

        // --- avd name
        label = new Label(parent, SWT.NONE);
        label.setText("AVD Name:");
        tooltip = "The name of the Android Virtual Device";
        label.setToolTipText(tooltip);
        mAvdName = new Text(parent, SWT.BORDER);
        mAvdName.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mAvdName.addModifyListener(new CreateNameModifyListener());

        // --- device selection
        label = new Label(parent, SWT.NONE);
        label.setText("Device:");
        tooltip = "The device this AVD will be based on";
        mDevice = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mDevice.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        initializeDevices();
        mDevice.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent arg0) {
                onDeviceSelection();
            }
        });

        // --- api target
        label = new Label(parent, SWT.NONE);
        label.setText("Target:");
        tooltip = "The target API of the AVD";
        label.setToolTipText(tooltip);
        mTarget = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mTarget.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mTarget.setToolTipText(tooltip);
        mTarget.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e) {
                reloadTagAbiCombo();
                validatePage();
            }
        });

        // --- avd ABIs
        label = new Label(parent, SWT.NONE);
        label.setText("CPU/ABI:");
        tooltip = "The CPU/ABI of the virtual device";
        label.setToolTipText(tooltip);
        mTagAbi = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mTagAbi.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mTagAbi.setToolTipText(tooltip);
        mTagAbi.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e) {
                reloadSkinCombo();
                validatePage();
            }
        });

        label = new Label(parent, SWT.NONE);
        label.setText("Keyboard:");
        mKeyboard = new Button(parent, SWT.CHECK);
        mKeyboard.setSelection(true); // default to having a keyboard irrespective of device
        mKeyboard.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mKeyboard.setText("Hardware keyboard present");

        // --- skins
        label = new Label(parent, SWT.NONE);
        label.setText("Skin:");
        mSkinCombo = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mSkinCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mSkinCombo.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e) {
                validatePage();
            }
        });

        // setup the 2 default choices (no skin, dynamic skin); do not select any right now.
        mCurrentSkinData.add(SKIN_DYNAMIC);
        mCurrentSkinData.add(SKIN_NONE);
        Collections.sort(mCurrentSkinData);
        mSkinCombo.add(mCurrentSkinData.get(0).getLabel());
        mSkinCombo.add(mCurrentSkinData.get(1).getLabel());

        // Preload target combo *after* ABI/Tag and Skin combos have been setup as
        // they will be setup depending on the selected target.
        preloadTargetCombo();

        // --- camera
        label = new Label(parent, SWT.NONE);
        label.setText("Front Camera:");
        tooltip = "";
        label.setToolTipText(tooltip);
        mFrontCamera = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mFrontCamera.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mFrontCamera.add("None");
        mFrontCamera.add("Emulated");
        mFrontCamera.add("Webcam0");
        mFrontCamera.select(0);

        label = new Label(parent, SWT.NONE);
        label.setText("Back Camera:");
        tooltip = "";
        label.setToolTipText(tooltip);
        mBackCamera = new Combo(parent, SWT.READ_ONLY | SWT.DROP_DOWN);
        mBackCamera.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mBackCamera.add("None");
        mBackCamera.add("Emulated");
        mBackCamera.add("Webcam0");
        mBackCamera.select(0);

        toggleCameras();

        // --- memory options group
        label = new Label(parent, SWT.NONE);
        label.setText("Memory Options:");

        Group memoryGroup = new Group(parent, SWT.NONE);
        memoryGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        memoryGroup.setLayout(new GridLayout(4, false));

        label = new Label(memoryGroup, SWT.NONE);
        label.setText("RAM:");
        tooltip = "The amount of RAM the emulated device should have in MiB";
        label.setToolTipText(tooltip);
        mRam = new Text(memoryGroup, SWT.BORDER);
        mRam.addVerifyListener(mDigitVerifier);
        mRam.addModifyListener(validateListener);
        mRam.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

        label = new Label(memoryGroup, SWT.NONE);
        label.setText("VM Heap:");
        tooltip = "The amount of memory, in MiB, available to typical Android applications";
        label.setToolTipText(tooltip);
        mVmHeap = new Text(memoryGroup, SWT.BORDER);
        mVmHeap.addVerifyListener(mDigitVerifier);
        mVmHeap.addModifyListener(validateListener);
        mVmHeap.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mVmHeap.setToolTipText(tooltip);

        // --- Data partition group
        label = new Label(parent, SWT.NONE);
        label.setText("Internal Storage:");
        tooltip = "The size of the data partition on the device.";
        Group storageGroup = new Group(parent, SWT.NONE);
        storageGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        storageGroup.setLayout(new GridLayout(2, false));
        mDataPartition = new Text(storageGroup, SWT.BORDER);
        mDataPartition.setText("200");
        mDataPartition.addVerifyListener(mDigitVerifier);
        mDataPartition.addModifyListener(validateListener);
        mDataPartition.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mDataPartitionSize = new Combo(storageGroup, SWT.READ_ONLY | SWT.DROP_DOWN);
        mDataPartitionSize.add("MiB");
        mDataPartitionSize.add("GiB");
        mDataPartitionSize.select(0);
        mDataPartitionSize.addModifyListener(validateListener);

        // --- sd card group
        label = new Label(parent, SWT.NONE);
        label.setText("SD Card:");
        label.setLayoutData(new GridData(GridData.BEGINNING, GridData.BEGINNING,
                false, false));

        final Group sdCardGroup = new Group(parent, SWT.NONE);
        sdCardGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        sdCardGroup.setLayout(new GridLayout(3, false));

        mSdCardSizeRadio = new Button(sdCardGroup, SWT.RADIO);
        mSdCardSizeRadio.setText("Size:");
        mSdCardSizeRadio.setToolTipText("Create a new SD Card file");
        mSdCardSizeRadio.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent arg0) {
                boolean sizeMode = mSdCardSizeRadio.getSelection();
                enableSdCardWidgets(sizeMode);
                validatePage();
            }
        });

        mSdCardSize = new Text(sdCardGroup, SWT.BORDER);
        mSdCardSize.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mSdCardSize.addVerifyListener(mDigitVerifier);
        mSdCardSize.addModifyListener(validateListener);
        mSdCardSize.setToolTipText("Size of the new SD Card file (must be at least 9 MiB)");

        mSdCardSizeCombo = new Combo(sdCardGroup, SWT.DROP_DOWN | SWT.READ_ONLY);
        mSdCardSizeCombo.add("KiB");
        mSdCardSizeCombo.add("MiB");
        mSdCardSizeCombo.add("GiB");
        mSdCardSizeCombo.select(1);
        mSdCardSizeCombo.addSelectionListener(validateListener);

        mSdCardFileRadio = new Button(sdCardGroup, SWT.RADIO);
        mSdCardFileRadio.setText("File:");
        mSdCardFileRadio.setToolTipText("Use an existing file for the SD Card");

        mSdCardFile = new Text(sdCardGroup, SWT.BORDER);
        mSdCardFile.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        mSdCardFile.addModifyListener(validateListener);
        mSdCardFile.setToolTipText("File to use for the SD Card");

        mBrowseSdCard = new Button(sdCardGroup, SWT.PUSH);
        mBrowseSdCard.setText("Browse...");
        mBrowseSdCard.setToolTipText("Select the file to use for the SD Card");
        mBrowseSdCard.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent arg0) {
                onBrowseSdCard();
                validatePage();
            }
        });

        mSdCardSizeRadio.setSelection(true);
        enableSdCardWidgets(true);

        // --- avd options group
        label = new Label(parent, SWT.NONE);
        label.setText("Emulation Options:");
        Group optionsGroup = new Group(parent, SWT.NONE);
        optionsGroup.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        optionsGroup.setLayout(new GridLayout(2, true));
        mSnapshot = new Button(optionsGroup, SWT.CHECK);
        mSnapshot.setText("Snapshot");
        mSnapshot.setToolTipText("Emulator's state will be persisted between emulator executions");
        mSnapshot.addSelectionListener(validateListener);
        mGpuEmulation = new Button(optionsGroup, SWT.CHECK);
        mGpuEmulation.setText("Use Host GPU");
        mGpuEmulation.setToolTipText("Enable hardware OpenGLES emulation");
        mGpuEmulation.addSelectionListener(validateListener);

        // --- force creation group
        mForceCreation = new Button(parent, SWT.CHECK);
        mForceCreation.setText("Override the existing AVD with the same name");
        mForceCreation
                .setToolTipText("There's already an AVD with the same name. Check this to delete it and replace it by the new AVD.");
        mForceCreation.setLayoutData(new GridData(GridData.BEGINNING, GridData.CENTER,
                true, false, 2, 1));
        mForceCreation.setEnabled(false);
        mForceCreation.addSelectionListener(validateListener);

        // add a separator to separate from the ok/cancel button
        label = new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL);
        label.setLayoutData(new GridData(GridData.FILL, GridData.CENTER, true, false, 3, 1));

        // add stuff for the error display
        mStatusComposite = new Composite(parent, SWT.NONE);
        mStatusComposite.setLayoutData(new GridData(GridData.FILL, GridData.CENTER,
                true, false, 3, 1));
        GridLayout gl;
        mStatusComposite.setLayout(gl = new GridLayout(2, false));
        gl.marginHeight = gl.marginWidth = 0;

        mStatusIcon = new Label(mStatusComposite, SWT.NONE);
        mStatusIcon.setLayoutData(new GridData(GridData.BEGINNING, GridData.BEGINNING,
                false, false));
        mStatusLabel = new Label(mStatusComposite, SWT.WRAP);
        GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, false, 1, 1);
        // allow for approx 3 lines of text corresponding to the number of lines in the longest
        // error or warning
        gridData.heightHint = 50;
        mStatusLabel.setLayoutData(gridData);
        mStatusLabel.setText(""); //$NON-NLS-1$
    }

    @Nullable
    private Device getSelectedDevice() {
        Device[] devices = (Device[]) mDevice.getData();
        if (devices != null) {
            int index = mDevice.getSelectionIndex();
            if (index != -1 && index < devices.length) {
                return devices[index];
            }
        }

        return null;
    }

    /** Called by fillExisting/InitialDeviceInfo to select the device in the combo list. */
    private void selectDevice(String manufacturer, String name) {
        Device[] devices = (Device[]) mDevice.getData();
        if (devices != null) {
            for (int i = 0, n = devices.length; i < n; i++) {
                Device device = devices[i];
                if (device.getManufacturer().equals(manufacturer)
                        && device.getName().equals(name)) {
                    mDevice.select(i);
                    break;
                }
            }
        }
    }

    /** Called by fillExisting/InitialDeviceInfo to select the device in the combo list. */
    private void selectDevice(Device device) {
        Device[] devices = (Device[]) mDevice.getData();
        if (devices != null) {
            for (int i = 0, n = devices.length; i < n; i++) {
                if (devices[i].equals(device)) {
                    mDevice.select(i);
                    break;
                }
            }
        }
    }

    private void initializeDevices() {
        assert mDevice != null;

        LocalSdk localSdk = mAvdManager.getLocalSdk();
        File location = localSdk.getLocation();
        if (location != null) {
            DeviceManager deviceManager = DeviceManager.createInstance(location, mSdkLog);
            List<Device>  deviceList    = deviceManager.getDevices(DeviceManager.ALL_DEVICES);

            // Sort
            List<Device> nexus = new ArrayList<Device>(deviceList.size());
            List<Device> other = new ArrayList<Device>(deviceList.size());
            for (Device device : deviceList) {
                if (isNexus(device) && !isGeneric(device)) {
                    nexus.add(device);
                } else {
                    other.add(device);
                }
            }
            Collections.reverse(other);
            Collections.sort(nexus, new Comparator<Device>() {
                @Override
                public int compare(Device device1, Device device2) {
                    // Descending order of age
                    return nexusRank(device2) - nexusRank(device1);
                }
            });
            List<Device> all = nexus;
            all.addAll(other);

            Device[] devices = all.toArray(new Device[all.size()]);
            String[] labels = new String[devices.length];
            for (int i = 0, n = devices.length; i < n; i++) {
                Device device = devices[i];
                if (isNexus(device) && !isGeneric(device)) {
                    labels[i] = getNexusLabel(device);
                } else {
                    labels[i] = getGenericLabel(device);
                }
            }
            mDevice.setData(devices);
            mDevice.setItems(labels);
        }
    }

    /**
     * Can be called after the constructor to set the default device for this AVD.
     * Useful especially for new AVDs.
     * @param device
     */
    public void selectInitialDevice(Device device) {
        mInitWithDevice = device;
    }

    /**
     * {@link ModifyListener} used for live-validation of the fields content.
     */
    private class ValidateListener extends SelectionAdapter implements ModifyListener {
        @Override
        public void modifyText(ModifyEvent e) {
            validatePage();
        }

        @Override
        public void widgetSelected(SelectionEvent e) {
            super.widgetSelected(e);
            validatePage();
        }
    }

    /**
     * Callback when the AVD name is changed. When creating a new AVD, enables
     * the force checkbox if the name is a duplicate. When editing an existing
     * AVD, it's OK for the name to match the existing AVD.
     */
    private class CreateNameModifyListener implements ModifyListener {
        @Override
        public void modifyText(ModifyEvent e) {
            String name = mAvdName.getText().trim();
            if (mAvdInfo == null || !name.equals(mAvdInfo.getName())) {
                // Case where we're creating a new AVD or editing an existing
                // one
                // and the AVD name has been changed... check for name
                // uniqueness.

                Pair<AvdConflict, String> conflict = mAvdManager.isAvdNameConflicting(name);
                if (conflict.getFirst() != AvdManager.AvdConflict.NO_CONFLICT) {
                    // If we're changing the state from disabled to enabled,
                    // make sure
                    // to uncheck the button, to force the user to voluntarily
                    // re-enforce it.
                    // This happens when editing an existing AVD and changing
                    // the name from
                    // the existing AVD to another different existing AVD.
                    if (!mForceCreation.isEnabled()) {
                        mForceCreation.setEnabled(true);
                        mForceCreation.setSelection(false);
                    }
                } else {
                    mForceCreation.setEnabled(false);
                    mForceCreation.setSelection(false);
                }
            } else {
                // Case where we're editing an existing AVD with the name
                // unchanged.

                mForceCreation.setEnabled(false);
                mForceCreation.setSelection(false);
            }
            validatePage();
        }
    }

    private void onDeviceSelection() {
        Device currentDevice = getSelectedDevice();
        if (currentDevice != null) {
            fillDeviceProperties(currentDevice);
        }

        toggleCameras();
        validatePage();
    }

    private void fillDeviceProperties(Device device) {
        Hardware hw = device.getDefaultHardware();
        Long ram = hw.getRam().getSizeAsUnit(Storage.Unit.MiB);
        mRam.setText(Long.toString(ram));

        // Set the default VM heap size. This is based on the Android CDD minimums for each
        // screen size and density.
        Screen s = hw.getScreen();
        ScreenSize size = s.getSize();
        Density density = s.getPixelDensity();
        int vmHeapSize = 32;
        if (size.equals(ScreenSize.XLARGE)) {
            switch (density) {
                case LOW:
                case MEDIUM:
                    vmHeapSize = 32;
                    break;
                case TV:
                case HIGH:
                    vmHeapSize = 64;
                    break;
                case XHIGH:
                case XXHIGH:
                case XXXHIGH:
                    vmHeapSize = 128;
                break;
                case NODPI:
                    break;
            }
        } else {
            switch (density) {
                case LOW:
                case MEDIUM:
                    vmHeapSize = 16;
                    break;
                case TV:
                case HIGH:
                    vmHeapSize = 32;
                    break;
                case XHIGH:
                case XXHIGH:
                case XXXHIGH:
                    vmHeapSize = 64;
                break;
                case NODPI:
                    break;
            }
        }
        mVmHeap.setText(Integer.toString(vmHeapSize));

        boolean reloadTabAbiCombo = false;

        List<Software> allSoftware = device.getAllSoftware();
        if (allSoftware != null && !allSoftware.isEmpty()) {
            Software first = allSoftware.get(0);
            int min = first.getMinSdkLevel();;
            int max = first.getMaxSdkLevel();;
            for (int i = 1; i < allSoftware.size(); i++) {
                min = Math.min(min, first.getMinSdkLevel());
                max = Math.max(max, first.getMaxSdkLevel());
            }
            if (mCurrentTargets != null) {
                int bestApiLevel = Integer.MAX_VALUE;
                IAndroidTarget bestTarget = null;
                for (IAndroidTarget target : mCurrentTargets.values()) {
                    if (!target.isPlatform()) {
                        continue;
                    }
                    int apiLevel = target.getVersion().getApiLevel();
                    if (apiLevel >= min && apiLevel <= max) {
                        if (bestTarget == null || apiLevel < bestApiLevel) {
                            bestTarget = target;
                            bestApiLevel = apiLevel;
                        }
                    }
                }

                if (bestTarget != null) {
                    selectTarget(bestTarget);
                    reloadTabAbiCombo = true;
                }
            }
        }

        if (!reloadTabAbiCombo) {
            String deviceTagId = device.getTagId();
            Pair<IdDisplay, String> currTagAbi = getSelectedTagAbi();
            if (deviceTagId != null &&
                    (currTagAbi == null || !deviceTagId.equals(currTagAbi.getFirst().getId()))) {
                reloadTabAbiCombo = true;
            }
        }

        if (reloadTabAbiCombo) {
            reloadTagAbiCombo();
        }
    }

    private void toggleCameras() {
        mFrontCamera.setEnabled(false);
        mBackCamera.setEnabled(false);
        Device d = getSelectedDevice();
        if (d != null) {
            for (Camera c : d.getDefaultHardware().getCameras()) {
                if (CameraLocation.FRONT.equals(c.getLocation())) {
                    mFrontCamera.setEnabled(true);
                }
                if (CameraLocation.BACK.equals(c.getLocation())) {
                    mBackCamera.setEnabled(true);
                }
            }
        }
    }

    private void preloadTargetCombo() {
        String selected = null;
        int index = mTarget.getSelectionIndex();
        if (index >= 0) {
            selected = mTarget.getItem(index);
        }

        mCurrentTargets.clear();
        mTarget.removeAll();

        boolean found = false;
        index = -1;

        List<IAndroidTarget> targetData = new ArrayList<IAndroidTarget>();
        LocalSdk localSdk = mAvdManager.getLocalSdk();
        if (localSdk != null) {
            for (IAndroidTarget target : localSdk.getTargets()) {
                String name;
                if (target.isPlatform()) {
                    name = String.format("%s - API Level %s",
                            target.getName(),
                            target.getVersion().getApiString());
                } else {
                    name = String.format("%s (%s) - API Level %s",
                            target.getName(),
                            target.getVendor(),
                            target.getVersion().getApiString());
                }
                mCurrentTargets.put(name, target);
                mTarget.add(name);
                targetData.add(target);
                if (!found) {
                    index++;
                    found = name.equals(selected);
                }
            }
        }

        mTarget.setEnabled(mCurrentTargets.size() > 0);
        mTarget.setData(targetData.toArray(new IAndroidTarget[targetData.size()]));

        if (found) {
            mTarget.select(index);
        }

        reloadTagAbiCombo();
    }

    private void selectTarget(IAndroidTarget target) {
        IAndroidTarget[] targets = (IAndroidTarget[]) mTarget.getData();
        if (targets != null) {
            for (int i = 0; i < targets.length; i++) {
                if (target == targets[i]) {
                    mTarget.select(i);
                    break;
                }
            }
        }
    }

    private IAndroidTarget getSelectedTarget() {
        IAndroidTarget[] targets = (IAndroidTarget[]) mTarget.getData();
        int index = mTarget.getSelectionIndex();
        if (targets != null && index != -1 && index < targets.length) {
            return targets[index];
        }

        return null;
    }

    /**
     * Reload all the abi types in the selection list.
     * Also adds/remove the skin choices embedded in a tag/abi, if any.
     */
    private void reloadTagAbiCombo() {

        int index = mTarget.getSelectionIndex();
        if (index >= 0) {
            String targetName = mTarget.getItem(index);
            IAndroidTarget target = mCurrentTargets.get(targetName);

            ISystemImage[] systemImages = getSystemImages(target);

            // If user explicitly selected an ABI before, preserve that option
            // If user did not explicitly select before (only one option before)
            // force them to select
            String selected = null;
            index = mTagAbi.getSelectionIndex();
            if (index >= 0 && mTagAbi.getItemCount() > 1) {
                selected = mTagAbi.getItem(index);
            }

            // if there's a selected device that requires a specific non-default tag-id,
            // filter the list to only match this tag.
            Device currDevice = getSelectedDevice();
            String deviceTagId = currDevice == null ? null : currDevice.getTagId();
            if (deviceTagId != null &&
                    (deviceTagId.isEmpty() || SystemImage.DEFAULT_TAG.equals(deviceTagId))) {
                deviceTagId = null;
            }

            // filter and create the list
            mTagAbi.removeAll();
            mCurrentSystemImages.clear();

            int i;
            boolean found = false;
            for (i = 0; i < systemImages.length; i++) {
                ISystemImage systemImage = systemImages[i];
                if (deviceTagId != null && !deviceTagId.equals(systemImage.getTag().getId())) {
                    continue;
                }
                mCurrentSystemImages.add(systemImage);
                String prettyAbiType = AvdInfo.getPrettyAbiType(systemImage);
                mTagAbi.add(prettyAbiType);
                if (!found) {
                    found = prettyAbiType.equals(selected);
                    if (found) {
                        mTagAbi.select(i);
                    }
                }
            }

            mTagAbi.setEnabled(!mCurrentSystemImages.isEmpty());

            if (mCurrentSystemImages.isEmpty()) {
                mTagAbi.add("No system images installed for this target.");
                mTagAbi.select(0);
            } else if (mCurrentSystemImages.size() == 1) {
                mTagAbi.select(0);
            }
        }

        reloadSkinCombo();
    }

    private void reloadSkinCombo() {
        AvdSkinChoice selected = getSelectedSkinChoice();

        // Remove existing target & tag skins
        for (Iterator<AvdSkinChoice> it = mCurrentSkinData.iterator(); it.hasNext(); ) {
            AvdSkinChoice choice = it.next();
            if (choice.hasPath()) {
                it.remove();
            }
        }

        IAndroidTarget target = getSelectedTarget();
        if (target != null) {
            ISystemImage   sysImg = getSelectedSysImg();
            Set<File> sysImgSkins = new HashSet<File>();
            if (sysImg != null) {
                sysImgSkins.addAll(Arrays.asList(sysImg.getSkins()));
            }

            // path of sdk/system-images
            String sdkSysImgPath = new File(mAvdManager.getLocalSdk().getLocation(),
                                            SdkConstants.FD_SYSTEM_IMAGES).getAbsolutePath();

            for (File skin : target.getSkins()) {
                String label = skin.getName();
                String skinPath = skin.getAbsolutePath();
                if (skinPath.startsWith(sdkSysImgPath)) {
                    if (sysImg == null) {
                        // Reject a sys-img based skin if no sys img is selected
                        continue;
                    }
                    if (!sysImgSkins.contains(skin)) {
                        // If a skin comes from a tagged system-image, only display
                        // those matching the current system image.
                        continue;
                    }
                    if (!SystemImage.DEFAULT_TAG.equals(sysImg.getTag().getId())) {
                        // Append the tag name if it's not the similar to the label.
                        String display = sysImg.getTag().getDisplay();
                        String azDisplay = display.toLowerCase(Locale.US).replaceAll("[^a-z]", "");
                        String azLabel   = label  .toLowerCase(Locale.US).replaceAll("[^a-z]", "");
                        if (!azLabel.contains(azDisplay)) {
                            label = String.format("%s (%s)", label, display);
                        }
                    }
                }
                AvdSkinChoice sc = new AvdSkinChoice(SkinType.FROM_TARGET, label, skin);
                mCurrentSkinData.add(sc);
            }
        }

        Collections.sort(mCurrentSkinData);

        mSkinCombo.removeAll();
        for (int i = 0; i < mCurrentSkinData.size(); i++) {
            AvdSkinChoice choice = mCurrentSkinData.get(i);
            mSkinCombo.add(choice.getLabel());
            if (choice == selected) {
                mSkinCombo.select(i);
            }
        }

    }

    /**
     * Enable or disable the sd card widgets.
     *
     * @param sizeMode if true the size-based widgets are to be enabled, and the
     *            file-based ones disabled.
     */
    private void enableSdCardWidgets(boolean sizeMode) {
        mSdCardSize.setEnabled(sizeMode);
        mSdCardSizeCombo.setEnabled(sizeMode);

        mSdCardFile.setEnabled(!sizeMode);
        mBrowseSdCard.setEnabled(!sizeMode);
    }

    private void onBrowseSdCard() {
        FileDialog dlg = new FileDialog(getContents().getShell(), SWT.OPEN);
        dlg.setText("Choose SD Card image file.");

        String fileName = dlg.open();
        if (fileName != null) {
            mSdCardFile.setText(fileName);
        }
    }

    @Override
    public void okPressed() {
        if (createAvd()) {
            super.okPressed();
        }
    }

    private void validatePage() {
        String error = null;
        ArrayList<String> warnings = new ArrayList<String>();
        boolean valid = true;

        if (mAvdName.getText().isEmpty()) {
            error = "AVD Name cannot be empty";
            setPageValid(false, error, null);
            return;
        }

        String avdName = mAvdName.getText();
        if (!AvdManager.RE_AVD_NAME.matcher(avdName).matches()) {
            error = String.format(
                    "AVD name '%1$s' contains invalid characters.\nAllowed characters are: %2$s",
                    avdName, AvdManager.CHARS_AVD_NAME);
            setPageValid(false, error, null);
            return;
        }

        if (mDevice.getSelectionIndex() < 0) {
            error = "No device selected";
            setPageValid(false, error, null);
            return;
        }

        if (mTarget.getSelectionIndex() < 0) {
            error = "No target selected";
            setPageValid(false, error, null);
            return;
        }

        if (mCurrentSystemImages.isEmpty()) {
            error = "No CPU/ABI system image available for this target";
            setPageValid(false, error, null);
            return;
        } else if (getSelectedTagAbi() == null) {
            error = "No CPU/ABI system image selected";
            setPageValid(false, error, null);
            return;
        }

        // If the target is an addon, check its base platform requirement is satisfied.
        String targetName = mTarget.getItem(mTarget.getSelectionIndex());
        IAndroidTarget target = mCurrentTargets.get(targetName);
        if (target != null && !target.isPlatform()) {

            ISystemImage[] sis = target.getSystemImages();
            if (sis != null && sis.length > 0) {
                // Note: if an addon has no system-images of its own, it depends on its parent
                // platform and it wouldn't have been loaded properly if the platform were
                // missing so we don't need to double-check that part here.

                Pair<IdDisplay, String> tagAbi = getSelectedTagAbi();
                IdDisplay tag = tagAbi.getFirst();
                String abiType = tagAbi.getSecond();
                if (abiType != null &&
                        !abiType.isEmpty() &&
                        target.getParent().getSystemImage(tag, abiType) == null) {
                    // We have a system-image requirement but there is no such system image
                    // loaded in the parent platform. This AVD won't run properly.
                    warnings.add(
                            String.format(
                                "This AVD may not work unless you install the %1$s system image " +
                                "for %2$s (%3$s) first.",
                                AvdInfo.getPrettyAbiType(tag, abiType),
                                target.getParent().getName(),
                                target.getParent().getVersion().toString()));
                }
            }
        }

        AvdSkinChoice skinChoice = getSelectedSkinChoice();
        if (skinChoice == null) {
            error = "No skin selected";
            setPageValid(false, error, null);
            return;
        }

        if (mRam.getText().isEmpty()) {
            error = "Mising RAM value";
            setPageValid(false, error, null);
            return;
        }

        if (mVmHeap.getText().isEmpty()) {
            error = "Mising VM Heap value";
            setPageValid(false, error, null);
            return;
        }

        if (mDataPartition.getText().isEmpty() || mDataPartitionSize.getSelectionIndex() < 0) {
            error = "Invalid Data partition size.";
            setPageValid(false, error, null);
            return;
        }

        // validate sdcard size or file
        if (mSdCardSizeRadio.getSelection()) {
            if (!mSdCardSize.getText().isEmpty() && mSdCardSizeCombo.getSelectionIndex() >= 0) {
                try {
                    long sdSize = Long.parseLong(mSdCardSize.getText());

                    int sizeIndex = mSdCardSizeCombo.getSelectionIndex();
                    if (sizeIndex >= 0) {
                        // index 0 shifts by 10 (1024=K), index 1 by 20, etc.
                        sdSize <<= 10 * (1 + sizeIndex);
                    }

                    if (sdSize < AvdManager.SDCARD_MIN_BYTE_SIZE ||
                            sdSize > AvdManager.SDCARD_MAX_BYTE_SIZE) {
                        valid = false;
                        error = "SD Card size is invalid. Range is 9 MiB..1023 GiB.";
                    }
                } catch (NumberFormatException e) {
                    valid = false;
                    error = " SD Card size must be a valid integer between 9 MiB and 1023 GiB";
                }
            }
        } else {
            if (mSdCardFile.getText().isEmpty() || !new File(mSdCardFile.getText()).isFile()) {
                valid = false;
                error = "SD Card path isn't valid.";
            }
        }
        if (!valid) {
            setPageValid(valid, error, null);
            return;
        }

        if (mForceCreation.isEnabled() && !mForceCreation.getSelection()) {
            valid = false;
            error = String.format(
                    "The AVD name '%s' is already used.\n" +
                            "Check \"Override the existing AVD\" to delete the existing one.",
                    mAvdName.getText());
        }

        if (mAvdInfo != null && !mAvdInfo.getName().equals(mAvdName.getText())) {
            warnings.add(
                    String.format("The AVD '%1$s' will be duplicated into '%2$s'.",
                        mAvdInfo.getName(),
                        mAvdName.getText()));
        }

        // On Windows, display a warning if attempting to create AVD's with RAM > 512 MB.
        // This restriction should go away when we switch to using a 64 bit emulator.
        if (SdkConstants.CURRENT_PLATFORM == SdkConstants.PLATFORM_WINDOWS) {
            long ramSize = 0;
            try {
                ramSize = Long.parseLong(mRam.getText());
            } catch (NumberFormatException e) {
                // ignore
            }

            if (ramSize > 768) {
                warnings.add(
                    "On Windows, emulating RAM greater than 768M may fail depending on the"
                    + " system load. Try progressively smaller values of RAM if the emulator"
                    + " fails to launch.");
            }
        }

        if (mGpuEmulation.getSelection() && mSnapshot.getSelection()) {
            valid = false;
            error = "GPU Emulation and Snapshot cannot be used simultaneously";
        }

        String warning = Joiner.on('\n').join(warnings);
        setPageValid(valid, error, warning);
        return;
    }

    private void setPageValid(boolean valid, String error, String warning) {
        mOkButton.setEnabled(valid);
        if (error != null && !error.isEmpty()) {
            mStatusIcon.setImage(mImageFactory.getImageByName("reject_icon16.png"));  //$NON-NLS-1$
            mStatusLabel.setText(error);
        } else if (warning != null && !warning.isEmpty()) {
            mStatusIcon.setImage(mImageFactory.getImageByName("warning_icon16.png")); //$NON-NLS-1$
            mStatusLabel.setText(warning);
        } else {
            mStatusIcon.setImage(null);
            mStatusLabel.setText(" \n "); //$NON-NLS-1$
        }

        mStatusComposite.pack(true);
        getShell().layout(true, true);
    }

    private boolean createAvd() {
        String avdName = mAvdName.getText();
        if (avdName == null || avdName.isEmpty()) {
            return false;
        }

        String targetName = mTarget.getItem(mTarget.getSelectionIndex());
        IAndroidTarget target = mCurrentTargets.get(targetName);
        if (target == null) {
            return false;
        }

        // get the tag & abi type
        Pair<IdDisplay, String> tagAbi = getSelectedTagAbi();
        if (tagAbi == null) {
            return false;
        }
        IdDisplay tag = tagAbi.getFirst();
        String abiType = tagAbi.getSecond();

        // get the SD card data from the UI.
        String sdName = null;
        if (mSdCardSizeRadio.getSelection()) {
            // size mode
            String value = mSdCardSize.getText().trim();
            if (value.length() > 0) {
                sdName = value;
                // add the unit
                switch (mSdCardSizeCombo.getSelectionIndex()) {
                    case 0:
                        sdName += "K"; //$NON-NLS-1$
                        break;
                    case 1:
                        sdName += "M"; //$NON-NLS-1$
                        break;
                    case 2:
                        sdName += "G"; //$NON-NLS-1$
                        break;
                    default:
                        // shouldn't be here
                        assert false;
                }
            }
        } else {
            // file mode.
            sdName = mSdCardFile.getText().trim();
        }

        // Get the device
        Device device = getSelectedDevice();
        if (device == null) {
            return false;
        }

        File skinFolder = null;
        String skinName = null;
        AvdSkinChoice skinChoice = getSelectedSkinChoice();
        if (skinChoice == null) {
            return false;
        }
        if (skinChoice.hasPath()) {
            skinFolder = skinChoice.getPath();
        } else {
            Screen s = device.getDefaultHardware().getScreen();
            skinName = s.getXDimension() + "x" + s.getYDimension();
        }

        ILogger log = mSdkLog;
        if (log == null || log instanceof MessageBoxLog) {
            // If the current logger is a message box, we use our own (to make sure
            // to display errors right away and customize the title).
            log = new MessageBoxLog(
                    String.format("Result of creating AVD '%s':", avdName),
                    getContents().getDisplay(),
                    false /* logErrorsOnly */);
        }

        Map<String, String> hwProps = DeviceManager.getHardwareProperties(device);
        if (mGpuEmulation.getSelection()) {
            hwProps.put(AvdManager.AVD_INI_GPU_EMULATION, HardwareProperties.BOOLEAN_YES);
        }

        File avdFolder = null;
        try {
            avdFolder = AvdInfo.getDefaultAvdFolder(mAvdManager, avdName);
        } catch (AndroidLocationException e) {
            return false;
        }

        // Although the device has this information, some devices have more RAM than we'd want to
        // allocate to an emulator.
        hwProps.put(AvdManager.AVD_INI_RAM_SIZE, mRam.getText());
        hwProps.put(AvdManager.AVD_INI_VM_HEAP_SIZE, mVmHeap.getText());

        String suffix;
        switch (mDataPartitionSize.getSelectionIndex()) {
            case 0:
                suffix = "M";
                break;
            case 1:
                suffix = "G";
                break;
            default:
                suffix = "K";
        }
        hwProps.put(AvdManager.AVD_INI_DATA_PARTITION_SIZE, mDataPartition.getText()+suffix);

        hwProps.put(HardwareProperties.HW_KEYBOARD,
                mKeyboard.getSelection() ?
                        HardwareProperties.BOOLEAN_YES : HardwareProperties.BOOLEAN_NO);

        hwProps.put(AvdManager.AVD_INI_SKIN_DYNAMIC,
                skinChoice.getType() == SkinType.DYNAMIC ?
                        HardwareProperties.BOOLEAN_YES : HardwareProperties.BOOLEAN_NO);

        if (mFrontCamera.isEnabled()) {
            hwProps.put(AvdManager.AVD_INI_CAMERA_FRONT,
                    mFrontCamera.getText().toLowerCase());
        }

        if (mBackCamera.isEnabled()) {
            hwProps.put(AvdManager.AVD_INI_CAMERA_BACK,
                    mBackCamera.getText().toLowerCase());
        }

        if (sdName != null) {
            hwProps.put(HardwareProperties.HW_SDCARD, HardwareProperties.BOOLEAN_YES);
        }

        AvdInfo avdInfo = mAvdManager.createAvd(avdFolder,
                avdName,
                target,
                tag,
                abiType,
                skinFolder,
                skinName,
                sdName,
                hwProps,
                device.getBootProps(),
                mSnapshot.getSelection(),
                mForceCreation.getSelection(),
                mAvdInfo != null, // edit existing
                log);

        mCreatedAvd = avdInfo;
        boolean success = avdInfo != null;

        if (log instanceof MessageBoxLog) {
            ((MessageBoxLog) log).displayResult(success);
        }
        return success;
    }

    @Nullable
    private AvdSkinChoice getSelectedSkinChoice() {
        int choiceIndex = mSkinCombo.getSelectionIndex();
        if (choiceIndex >= 0 && choiceIndex < mCurrentSkinData.size()) {
            return mCurrentSkinData.get(choiceIndex);
        }
        return null;
    }

    @Nullable
    private Pair<IdDisplay, String> getSelectedTagAbi() {
        ISystemImage selected = getSelectedSysImg();
        if (selected != null) {
            return Pair.of(selected.getTag(), selected.getAbiType());
        }
        return null;
    }

    @Nullable
    private ISystemImage getSelectedSysImg() {
        if (!mCurrentSystemImages.isEmpty()) {
            int abiIndex = mTagAbi.getSelectionIndex();
            if (abiIndex >= 0 && abiIndex < mCurrentSystemImages.size()) {
                return mCurrentSystemImages.get(abiIndex);
            }
        }
        return null;
    }

    private void fillExistingAvdInfo(AvdInfo avd) {
        mAvdName.setText(avd.getName());
        selectDevice(avd.getDeviceManufacturer(), avd.getDeviceName());
        toggleCameras();

        IAndroidTarget target = avd.getTarget();

        if (target != null && !mCurrentTargets.isEmpty()) {
            // Try to select the target in the target combo.
            // This will fail if the AVD needs to be repaired.
            //
            // This is a linear search but the list is always
            // small enough and we only do this once.
            int n = mTarget.getItemCount();
            for (int i = 0; i < n; i++) {
                if (target.equals(mCurrentTargets.get(mTarget.getItem(i)))) {
                    // Note: combo.select does not trigger the combo's widgetSelected callback.
                    mTarget.select(i);
                    reloadTagAbiCombo();
                    break;
                }
            }
        }

        ISystemImage[] systemImages = getSystemImages(target);
        if (target != null && systemImages.length > 0) {
            mTagAbi.setEnabled(systemImages.length > 1);
            String abiType = AvdInfo.getPrettyAbiType(avd.getTag(), avd.getAbiType());
            int n = mTagAbi.getItemCount();
            for (int i = 0; i < n; i++) {
                if (abiType.equals(mTagAbi.getItem(i))) {
                    mTagAbi.select(i);
                    reloadSkinCombo();
                    break;
                }
            }
        }

        Map<String, String> props = avd.getProperties();

        if (props != null) {
            String snapshots = props.get(AvdManager.AVD_INI_SNAPSHOT_PRESENT);
            if (snapshots != null && snapshots.length() > 0) {
                mSnapshot.setSelection(snapshots.equals("true"));
            }

            String gpuEmulation = props.get(AvdManager.AVD_INI_GPU_EMULATION);
            mGpuEmulation.setSelection(gpuEmulation != null &&
                    gpuEmulation.equals(HardwareProperties.BOOLEAN_YES));

            String sdcard = props.get(AvdManager.AVD_INI_SDCARD_PATH);
            if (sdcard != null && sdcard.length() > 0) {
                enableSdCardWidgets(false);
                mSdCardSizeRadio.setSelection(false);
                mSdCardFileRadio.setSelection(true);
                mSdCardFile.setText(sdcard);
            }

            String ramSize = props.get(AvdManager.AVD_INI_RAM_SIZE);
            if (ramSize != null) {
                mRam.setText(ramSize);
            }

            String vmHeapSize = props.get(AvdManager.AVD_INI_VM_HEAP_SIZE);
            if (vmHeapSize != null) {
                mVmHeap.setText(vmHeapSize);
            }

            String dataPartitionSize = props.get(AvdManager.AVD_INI_DATA_PARTITION_SIZE);
            if (dataPartitionSize != null) {
                mDataPartition.setText(
                        dataPartitionSize.substring(0, dataPartitionSize.length() - 1));
                switch (dataPartitionSize.charAt(dataPartitionSize.length() - 1)) {
                    case 'M':
                        mDataPartitionSize.select(0);
                        break;
                    case 'G':
                        mDataPartitionSize.select(1);
                        break;
                    default:
                        mDataPartitionSize.select(-1);
                }
            }

            mKeyboard.setSelection(
                    HardwareProperties.BOOLEAN_YES.equalsIgnoreCase(
                            props.get(HardwareProperties.HW_KEYBOARD)));

            SkinType defaultSkinType = SkinType.NONE;
            // the AVD .ini skin path is relative to the SDK folder *or* is a numeric size.
            String skinIniPath = props.get(AvdManager.AVD_INI_SKIN_PATH);
            if (skinIniPath != null) {
                File skinFolder = new File(mAvdManager.getLocalSdk().getLocation(), skinIniPath);

                for (int i = 0; i < mCurrentSkinData.size(); i++) {
                    if (mCurrentSkinData.get(i).hasPath() &&
                            skinFolder.equals(mCurrentSkinData.get(i).getPath())) {
                        mSkinCombo.select(i);
                        defaultSkinType = null;
                        break;
                    }
                }
            }

            if (defaultSkinType != null) {
                if (HardwareProperties.BOOLEAN_YES.equalsIgnoreCase(
                        props.get(AvdManager.AVD_INI_SKIN_DYNAMIC))) {
                    defaultSkinType = SkinType.DYNAMIC;
                }

                for (int i = 0; i < mCurrentSkinData.size(); i++) {
                    if (mCurrentSkinData.get(i).getType() == defaultSkinType) {
                        mSkinCombo.select(i);
                        break;
                    }
                }

            }

            String cameraFront = props.get(AvdManager.AVD_INI_CAMERA_FRONT);
            if (cameraFront != null) {
                String[] items = mFrontCamera.getItems();
                for (int i = 0; i < items.length; i++) {
                    if (items[i].toLowerCase().equals(cameraFront)) {
                        mFrontCamera.select(i);
                        break;
                    }
                }
            }

            String cameraBack = props.get(AvdManager.AVD_INI_CAMERA_BACK);
            if (cameraBack != null) {
                String[] items = mBackCamera.getItems();
                for (int i = 0; i < items.length; i++) {
                    if (items[i].toLowerCase().equals(cameraBack)) {
                        mBackCamera.select(i);
                        break;
                    }
                }
            }

            sdcard = props.get(AvdManager.AVD_INI_SDCARD_SIZE);
            if (sdcard != null && sdcard.length() > 0) {
                String[] values = new String[2];
                long sdcardSize = AvdManager.parseSdcardSize(sdcard, values);

                if (sdcardSize != AvdManager.SDCARD_NOT_SIZE_PATTERN) {
                    enableSdCardWidgets(true);
                    mSdCardFileRadio.setSelection(false);
                    mSdCardSizeRadio.setSelection(true);

                    mSdCardSize.setText(values[0]);

                    String suffix = values[1];
                    int n = mSdCardSizeCombo.getItemCount();
                    for (int i = 0; i < n; i++) {
                        if (mSdCardSizeCombo.getItem(i).startsWith(suffix)) {
                            mSdCardSizeCombo.select(i);
                        }
                    }
                }
            }
        }
    }

    private void fillInitialDeviceInfo(Device device) {
        String name = device.getManufacturer();
        if (!name.equals("Generic") &&      // TODO define & use constants
                !name.equals("User") &&
                device.getName().indexOf(name) == -1) {
            name = " by " + name;
        } else {
            name = "";
        }
        name = "AVD for " + device.getName() + name;
        // sanitize the name
        name = name.replaceAll("[^0-9a-zA-Z_-]+", " ").trim().replaceAll("[ _]+", "_");
        mAvdName.setText(name);

        // Select the device
        selectDevice(device);
        toggleCameras();

        // If there's only one target, select it by default.
        // TODO: if there are more than 1 target, select the higher platform target as
        // a likely default.
        if (mTarget.getItemCount() == 1) {
            mTarget.select(0);
            reloadTagAbiCombo();
        }

        fillDeviceProperties(device);
    }

    /**
     * Returns the list of system images of a target.
     * <p/>
     * If target is null, returns an empty list. If target is an add-on with no
     * system images, return the list from its parent platform.
     *
     * @param target An IAndroidTarget. Can be null.
     * @return A non-null ISystemImage array. Can be empty.
     */
    @NonNull
    private ISystemImage[] getSystemImages(IAndroidTarget target) {
        if (target != null) {
            ISystemImage[] images = target.getSystemImages();

            if ((images == null || images.length == 0) && !target.isPlatform()) {
                // If an add-on does not provide any system images, use the ones
                // from the parent.
                images = target.getParent().getSystemImages();
            }

            if (images != null) {
                return images;
            }
        }

        return new ISystemImage[0];
    }

    // Code copied from DeviceMenuListener in ADT; unify post release

    private static final String NEXUS = "Nexus";       //$NON-NLS-1$
    private static final String GENERIC = "Generic";   //$NON-NLS-1$
    private static Pattern PATTERN = Pattern.compile(
            "(\\d+\\.?\\d*)in (.+?)( \\(.*Nexus.*\\))?"); //$NON-NLS-1$

    private static int nexusRank(Device device) {
        String name = device.getName();
        if (name.endsWith(" One")) {     //$NON-NLS-1$
            return 1;
        }
        if (name.endsWith(" S")) {       //$NON-NLS-1$
            return 2;
        }
        if (name.startsWith("Galaxy")) { //$NON-NLS-1$
            return 3;
        }
        if (name.endsWith(" 7")) {       //$NON-NLS-1$
            return 4;
        }
        if (name.endsWith(" 10")) {       //$NON-NLS-1$
            return 5;
        }
        if (name.endsWith(" 4")) {       //$NON-NLS-1$
            return 6;
        }

        return 7;
    }

    private static boolean isGeneric(Device device) {
        return device.getManufacturer().equals(GENERIC);
    }

    private static boolean isNexus(Device device) {
        return device.getName().contains(NEXUS);
    }

    private static String getGenericLabel(Device d) {
        // * Replace "'in'" with '"' (e.g. 2.7" QVGA instead of 2.7in QVGA)
        // * Use the same precision for all devices (all but one specify decimals)
        // * Add some leading space such that the dot ends up roughly in the
        //   same space
        // * Add in screen resolution and density
        String name = d.getName();
        if (name.equals("3.7 FWVGA slider")) {                        //$NON-NLS-1$
            // Fix metadata: this one entry doesn't have "in" like the rest of them
            name = "3.7in FWVGA slider";                              //$NON-NLS-1$
        }

        Matcher matcher = PATTERN.matcher(name);
        if (matcher.matches()) {
            String size = matcher.group(1);
            String n = matcher.group(2);
            int dot = size.indexOf('.');
            if (dot == -1) {
                size = size + ".0";
                dot = size.length() - 2;
            }
            for (int i = 0; i < 2 - dot; i++) {
                size = ' ' + size;
            }
            name = size + "\" " + n;
        }

        return String.format(java.util.Locale.US, "%1$s (%2$s)", name,
                getResolutionString(d));
    }

    private static String getNexusLabel(Device d) {
        String name = d.getName();
        Screen screen = d.getDefaultHardware().getScreen();
        float length = (float) screen.getDiagonalLength();
        return String.format(java.util.Locale.US, "%1$s (%3$s\", %2$s)",
                name, getResolutionString(d), Float.toString(length));
    }

    @Nullable
    private static String getResolutionString(Device device) {
        Screen screen = device.getDefaultHardware().getScreen();
        return String.format(java.util.Locale.US,
                "%1$d \u00D7 %2$d: %3$s", // U+00D7: Unicode multiplication sign
                screen.getXDimension(),
                screen.getYDimension(),
                screen.getPixelDensity().getResourceValue());
    }

    /**
     * AVD skin type. Order defines the order of the skin combo list.
     */
    private enum SkinType {
        DYNAMIC,
        NONE,
        FROM_TARGET,
    }

    /*
     * Choice of AVD skin: dynamic, no skin, or one from the target.
     * The 2 "internals" skins (dynamic and no skin) have no path.
     * The target-based skins have a path.
     */
    private static class AvdSkinChoice implements Comparable<AvdSkinChoice> {

        private final SkinType mType;
        private final String mLabel;
        private final File mPath;

        AvdSkinChoice(@NonNull SkinType type, @NonNull String label) {
            this(type, label, null);
        }

        AvdSkinChoice(@NonNull SkinType type, @NonNull String label, @NonNull File path) {
            mType = type;
            mLabel = label;
            mPath = path;
        }

        @NonNull
        public SkinType getType() {
            return mType;
        }

        @NonNull
        public String getLabel() {
            return mLabel;
        }

        @Nullable
        public File getPath() {
            return mPath;
        }

        public boolean hasPath() {
            return mType == SkinType.FROM_TARGET;
        }

        @Override
        public int compareTo(AvdSkinChoice o) {
            int t = mType.compareTo(o.mType);
            if (t == 0) {
                t = mLabel.compareTo(o.mLabel);
            }
            if (t == 0 && mPath != null && o.mPath != null) {
                t = mPath.compareTo(o.mPath);
            }
            return t;
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((mType == null) ? 0 : mType.hashCode());
            result = prime * result + ((mLabel == null) ? 0 : mLabel.hashCode());
            result = prime * result + ((mPath == null) ? 0 : mPath.hashCode());
            return result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj == null) {
                return false;
            }
            if (!(obj instanceof AvdSkinChoice)) {
                return false;
            }
            AvdSkinChoice other = (AvdSkinChoice) obj;
            if (mType != other.mType) {
                return false;
            }
            if (mLabel == null) {
                if (other.mLabel != null) {
                    return false;
                }
            } else if (!mLabel.equals(other.mLabel)) {
                return false;
            }
            if (mPath == null) {
                if (other.mPath != null) {
                    return false;
                }
            } else if (!mPath.equals(other.mPath)) {
                return false;
            }
            return true;
        }


    }

}
