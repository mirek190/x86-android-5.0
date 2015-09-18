/*
 * Copyright (C) 2013 Intel Corporation
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

package com.android.server.pm;

import android.content.Context;
import java.io.File;

/** {@hide} */
public class ExtendUserManagerService extends UserManagerService {
    /**
     * Available for testing purposes.
     */
    ExtendUserManagerService(File dataDir, File baseUserPath) {
        super(dataDir, baseUserPath);
    }

    /**
     * Called by package manager to create the service.  This is closely
     * associated with the package manager, and the given lock is the
     * package manager's own lock.
     */
    ExtendUserManagerService(Context context, PackageManagerService pm,
            Object installLock, Object packagesLock) {
        super(context, pm, installLock, packagesLock);
    }
}
