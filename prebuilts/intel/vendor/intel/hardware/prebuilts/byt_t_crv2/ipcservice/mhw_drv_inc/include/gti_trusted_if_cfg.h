/*  -------------------------------------------------------------------------
    Copyright (C) 2011-2013 Intel Mobile Communications GmbH
    
         Sec Class: Intel Confidential (IC)
    
     ----------------------------------------------------------------------
     Revision Information:
       $$File name:  /mhw_drv_inc/inc/target_test/gti_trusted_if_cfg.h $
       $$CC-Version: .../oint_drv_target_interface/6 $
       $$Date:       2013-02-20    10:08:47 UTC $
     ---------------------------------------------------------------------- */


/*Use GTI_TRUSTED_IF_DECLARATION(module_name) to add trusted interface. Where
module_name is the name of the interface as declared in the corresponding structure definition
type. This feature enables to list an interface/interfaces as trusted interface directly through
interface name instead of pointer to interface definition.*/

#ifdef XL1_TEST_IF_ENABLED
    GTI_TRUSTED_IF_DECLARATION(xl1)
#endif

GTI_TRUSTED_IF_DECLARATION(sec)

GTI_TRUSTED_IF_DECLARATION(ver)

GTI_TRUSTED_IF_DECLARATION(vers)

GTI_TRUSTED_IF_DECLARATION(gticom)


