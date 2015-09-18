#!/bin/sh

isu_exec=${1}
priv_key=${2}
pub_key=${3}

bootimage=`tempfile`
manifest=`tempfile`
tput cup 0 0 >/dev/null 2>&1
cat - > ${bootimage}
${isu_exec} -h 0 -i ${bootimage} -o ${manifest} -l \
${priv_key} -k \
${pub_key} -t 4 -p 2 -v 1 >/dev/null 2>&1
cat ${manifest}_OS_manifest.bin
rm -f ${bootimage}
rm -f ${manifest}*

exit 0
