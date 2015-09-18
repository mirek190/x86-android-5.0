#
# Copyright (c) 2011, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
#      copyright notice, this list of conditions and the following
#      disclaimer in the documentation and/or other materials provided
#      with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

%.efi: %.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
		-j .rela -j .reloc -S --target=$(FORMAT) $*.so $@

OBJCOPY=objcopy

HOST  = $(shell $(CC) -dumpmachine | sed "s/\(-\).*$$//")
ARCH := $(shell $(CC) -dumpmachine | sed "s/\(-\).*$$//")
ARCH=ia32
ifeq ($(ARCH),x86_64)
	LIBDIR=/usr/lib64
	FORMAT=efi-app-x86-64
else
	ARCH=ia32
	LIBDIR=/usr/lib
	FORMAT=efi-app-$(ARCH)
endif

INCDIR := ../gnu-efi/inc
LIBDIR := ../gnu-efi

# gnuefi sometimes installs these under a gnuefi/ directory, and sometimes not
CRT0 := $(shell find $(LIBDIR) -name crt0-efi-$(ARCH).o 2>/dev/null | tail -n1)
LDSCRIPT := $(shell find $(LIBDIR) -name elf_$(ARCH)_efi.lds 2>/dev/null | tail -n1)

OSLOADER_FILE_PATH := EFI/BOOT/boot$(ARCH).efi
CFLAGS=-I. -I$(INCDIR) -I$(INCDIR)/$(ARCH) -Iandroid \
		-DEFI_FUNCTION_WRAPPER -fPIC -fshort-wchar -ffreestanding \
		-Wall -Ifs/ -Iloaders/ -D$(ARCH) -Werror -DOSLOADER_FILE_PATH=L'"$(OSLOADER_FILE_PATH)"'

ifeq ($(ARCH),ia32)
	ifeq ($(HOST),x86_64)
		CFLAGS += -m32
	endif
endif
ifeq ($(ARCH),x86_64)
	CFLAGS += -mno-red-zone
endif

LDFLAGS=-T $(LDSCRIPT) -Bsymbolic -shared -nostdlib -znocombreloc \
		-L$(LIBDIR)/gnuefi -L$(LIBDIR)/lib $(CRT0)

IMAGE=efilinux.efi
OBJS = entry.o malloc.o android/boot.o utils.o acpi.o bootlogic.o	\
	intel_partitions.o uefi_osnib.o uefi_keys.o uefi_boot.o \
	uefi_utils.o commands.o uefi_em.o
FS = fs/fs.o
PLATFORM = platform/platform.o platform/cherrytrail.o platform/x86.o
SPLASH_BMP = splash.bmp
SPLASH_OBJ = splash_bmp.o
all: $(IMAGE)

efilinux.efi: efilinux.so

$(SPLASH_OBJ): $(SPLASH_BMP)
	../../../out/host/linux-x86/bin/bin-to-hex splash_bmp < $^ | $(CC) -x c - -c $(CFLAGS) -o $@

efilinux.so: $(OBJS) $(FS) $(LOADERS) $(PLATFORM) $(SPLASH_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^  -lgnuefi -lefi $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

clean:
	rm -f $(IMAGE) efilinux.so $(OBJS) $(FS) $(LOADERS) $(PLATFORM) $(SPLASH_OBJ)
