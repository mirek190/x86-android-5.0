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

package com.intel.arkham;

/**
 * Utility class containing constants used by container administrator applications.
 */
public interface ContainerConstants {
    /** @hide */
    public static final String ACCOUNT_TYPE_CONTAINER = "account_type_container";
    /**
     * Account type used for syncing container contacts outside the container,
     * if the administrator's policy allows it.
     * @hide
     */
    public static final String SYNC_ACCOUNT_TYPE = "com.intel.arkham.accounts";
    /** @hide */
    public static final String EXTRA_CONTAINER_INFO = "containerInfo";
    public static final String CONTAINER_MANAGER_SERVICE = "container_manager";
}
