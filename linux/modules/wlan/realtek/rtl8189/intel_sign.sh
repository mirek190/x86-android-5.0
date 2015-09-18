#!/bin/bash
export SIGN_FILE=/home/android_sdk/intel/lp50/lp50r2/linux/kernel/scripts/sign-file 
export SIGNING_KEY_PRIV=/home/android_sdk/intel/lp50/lp50r2/out/target/product/byt_t_crv2/linux/kernel/signing_key.priv 
export SIGNING_KEY_x509=/home/android_sdk/intel/lp50/lp50r2/out/target/product/byt_t_crv2/linux/kernel/signing_key.x509 
perl $SIGN_FILE sha1 $SIGNING_KEY_PRIV $SIGNING_KEY_x509 8189es.ko

