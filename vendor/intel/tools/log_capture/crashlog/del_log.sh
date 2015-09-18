#!/sytem/xbin/ash
#
#
# Copyright (C) Intel 2010
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

if [ $# -eq 1 ] ; then

  diremmc=$(getprop persist.crashlogd.root)
  if [ -z $diremmc ]; then
    diremmc="/logs"
  fi
  dirsd="/mnt/sdcard/logs"
  num=$1

  tmp=$diremmc"/crashlog"$num
  if [ -d $tmp ]; then
    cd $diremmc
    rm -r $tmp
  fi

  tmp=$dirsd"/crashlog"$num
  if [ -d $tmp ]; then
    cd $dirsd
    rm -r $tmp
  fi

else

  echo "USAGE: del_log number"

fi

