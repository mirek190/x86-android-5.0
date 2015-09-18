#!/system/xbin/ash
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

if [ $# -le 1 ] ; then
  if [ $# -eq 0 ]; then
  filename="history_event"
  else
  filename=$1
  fi
  tmp=$filename".tmp"

  logs_root=$(getprop persist.crashlogd.root)
  if [ -z $logs_root ]; then
    logs_root="/logs"
  fi
  cd $logs_root
  if [ -f $filename ]; then
          read line < $filename
          echo $line > $tmp
          mv $tmp $filename
  fi
else

  echo "USAGE: del_hist [filename]"

fi

