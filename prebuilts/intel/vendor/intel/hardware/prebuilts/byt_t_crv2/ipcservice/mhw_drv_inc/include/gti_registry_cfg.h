/*  -------------------------------------------------------------------------
    Copyright (C) 2011-2014 Intel Mobile Communications GmbH
    
         Sec Class: Intel Confidential (IC)
    
     ----------------------------------------------------------------------
     Revision Information:
       $$File name:  /mhw_drv_inc/inc/target_test/gti_registry_cfg.h $
       $$CC-Version: .../oint_drv_target_interface/4 $
       $$Date:       2014-02-21    11:23:11 UTC $
     ---------------------------------------------------------------------- */


/********************************************************************************
*
* Description:
*   Declaration of GTI interfaces which shall be enabled from boot up.
*   Note: Only declare interfaces as shown, -no additional stuff can be put in this file.
*
*********************************************************************************/

// Ongoing discussion - removed for now
/* Definitions for dynamic groups created at boot-time
    GTI_REGISTRY_ITEM(parent, item)
    Enumeration is created by the convention:
    UTA_GTI_REG_[parent]_[item] 
    used to access registry-groups 
    Note:
    REGISTRY_ROOT is expanded to ROOT. 
    I.e.  GTI_REGISTRY_ITEM (REGISTRY_ROOT, HW) expands into UTA_GTI_REG_ROOT_HW
*/
#if 0 // Not used at the moment - Usage TBD
GTI_REGISTRY_GROUP (REGISTRY_ROOT, HW)

GTI_REGISTRY_GROUP (REGISTRY_ROOT, SW)
GTI_REGISTRY_GROUP (SW, MODULES)

GTI_REGISTRY_GROUP (REGISTRY_ROOT, PRODUCTION)
GTI_REGISTRY_GROUP (REGISTRY_ROOT, MODEM)
GTI_REGISTRY_GROUP (REGISTRY_ROOT, RF)
#endif
