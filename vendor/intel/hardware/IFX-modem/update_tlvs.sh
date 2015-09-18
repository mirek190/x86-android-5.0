# -----------------------------------------------------------------------------
# Description:
# The goal of this script is to update all TLV files
# -----------------------------------------------------------------------------
#
# Copyright (C) Intel 2014
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash

usage()
{
    echo -e "This script will update all TLV files\nUsage:\n"\
    "\t-h help\n"\
    "\t-b specify location of TLVgenerator binary\n"\
    "\t-f specify location of WPRD keys. The keys must be present in the same folder and be called NewSprk1024dev.key and NewSprk2048dev.key\n"
}

TLVgenerator=""
key_folder=""
input=$PWD/../../fw/modem/IMC/

while getopts "hb:f:" arg; do
    case $arg in
        h)
            usage
            exit
            ;;
        b)
            TLVgenerator=$OPTARG
            ;;
        f)
            key_folder=$OPTARG
            ;;
    esac
done

if [ "$TLVgenerator" == "" ] || [ "$key_folder" == "" ] ; then
    usage
    exit
fi

for key in {1024,2048}; do
    echo -e "\nUpdating TLV files with $key..."

    for f in $(find $input -name "*.txt"); do
        output=$(echo $f | sed "s#\($(basename $f)\)#${key}_\1# ; s:txt:tlv:g")
        $TLVgenerator -s pkcs1 -k $key_folder/NewSprk${key}dev.key -i $f -o $output;
        echo "+++ $output updated +++"
    done

done

echo -e "\nDone."
