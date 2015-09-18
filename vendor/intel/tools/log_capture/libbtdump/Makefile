#
# Copyright (C) Intel 2014
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

EXENAME = btdump
CFLAGS = -c -Werror -g -DUSE_LIBUNWIND -Iinc
LFLAGS = -g
LLIBS = -lunwind-x86_64 -lunwind-ptrace -lrt

all: CProcInfo.cpp btdump.cpp libbtdump.cpp
	g++ $(CFLAGS) CProcInfo.cpp
	g++ $(CFLAGS) CThreadInfo.cpp
	g++ $(CFLAGS) CFrameInfo.cpp
	g++ $(CFLAGS) btdump.cpp
	g++ $(CFLAGS) libbtdump.cpp
	g++ $(LFLAGS) -o $(EXENAME) *.o $(LLIBS)

clean:
	rm *.o $(EXENAME)