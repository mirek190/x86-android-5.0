#!/bin/bash

#####################################################################################
# Hook script used to apply some google_diff patches before running FOTAGen process #
#####################################################################################

GOOGLE_DIFFS=vendor/intel/google_diffs/l_pdk/patches

PATCH="0003-OTA-cleanup-OSIP-specifics.patch"
FOLDER="build"
BACK_FOLDER=".." # must contain as many "../" than FOLDER has depth
if [ -f ${GOOGLE_DIFFS}/${FOLDER}/${PATCH} ]
then
  echo "Applying patch ${PATCH} from ${GOOGLE_DIFFS}/${FOLDER}"
  cd $FOLDER && git am ${BACK_FOLDER}/${GOOGLE_DIFFS}/${FOLDER}/${PATCH} && cd -
else
  echo "Could not find patch ${PATCH} from ${GOOGLE_DIFFS}/${FOLDER} : skipping"
fi
