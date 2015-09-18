#!/usr/bin/env python

# Copyright (C) 2011 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import struct
import getopt
import sys

# Bootstub only reserves this much space for the cmdline
max_cmdline_size = 1023
kernel_cmdline_padding = 128


def write_padded(outfile, data, padding):
    padding = padding - len(data)
    assert padding >= 0
    outfile.write(data)
    outfile.write('\0' * padding)


def make_osimage(bootstub, xen, xen_cmdline, kernel, kernel_cmdline, ramdisk, domU_kernel, domU_cmdline, domU_ramdisk, vxe_FW_image, sps_image, filename):
    """Create a medfield-compatible OS image from component parts. This image
    will need to be stitched by FSTK before a medfield device will boot it"""

# OPEN AND SIZE OPTIONAL COMPONENTS
    if xen:
        xen_sz = os.stat(xen).st_size
        if xen_sz % 4096 != 0:
            xen_sz += 4096 - xen_sz % 4096
    if vxe_FW_image:
        vxe_size = os.stat(vxe_FW_image).st_size
        if vxe_size % 4096 != 0:
            vxe_size += 4096 - vxe_size % 4096
    if sps_image:
        sps_size = os.stat(sps_image).st_size
        if sps_size % 4096 != 0:
            sps_size += 4096 - sps_size % 4096
    if domU_kernel:
        domU_kernel_sz = os.stat(domU_kernel).st_size
        if domU_kernel_sz % 4096 != 0:
            domU_kernel_sz += 4096 - domU_kernel_sz % 4096

# OPEN AND SIZE KERNEL , CMDLINE
    kernel_sz = os.stat(kernel).st_size
    if kernel_sz % 4096 != 0:
        kernel_sz += 4096 - kernel_sz % 4096

    cmdline_sz = os.stat(kernel_cmdline).st_size + kernel_cmdline_padding

# Append cmdline sizes to kernel cmdline
    if xen:
        cmdline_sz += 1 + os.stat(xen_cmdline).st_size

    if domU_kernel:
        cmdline_sz += 1 + os.stat(domU_cmdline).st_size

    ramdisk_sz = os.stat(ramdisk).st_size
    if ramdisk_sz % 4096 != 0:
        ramdisk_sz += 4096 - ramdisk_sz % 4096

    if domU_kernel:
        domU_ramdisk_sz = os.stat(domU_ramdisk).st_size
    console_suppression = 0
    console_dev_type = 0xff
    reserved_flag_0 = 0x02BD02BD
    reserved_flag_1 = 0x12BD12BD

    if vxe_FW_image:
        vxe_f = open(vxe_FW_image, "rw")

    if sps_image:
        sps_f = open(sps_image, "rw")

    if xen:
        xen_f = open(xen, "rw")
        xen_cmdline_f = open(xen_cmdline, "rw")

    kernel_f = open(kernel, "rb")
    kernel_cmdline_f = open(kernel_cmdline, "rb")
    ramdisk_f = open(ramdisk, "rb")
    if domU_kernel:
        domU_kernel_f = open(domU_kernel, "rb")
        domU_cmdline_f = open(domU_cmdline, "rb")
        domU_ramdisk_f = open(domU_ramdisk, "rb")
    bootstub_f = open(bootstub, "rb")
    outfile_f = open(filename, "wb")

    assert cmdline_sz <= max_cmdline_size, "Command line too long, max %d bytes" % (max_cmdline_size,)

    cmdline = kernel_cmdline_f.read()
    cmdline += '\0' * kernel_cmdline_padding
    if xen:
        cmdline += '\0$' + xen_cmdline_f.read()
    if domU_kernel:
        cmdline += '\0$' + domU_cmdline_f.read()
    write_padded(outfile_f, cmdline, 1024)

# Write Size fields
    data = struct.pack("<IIIIII", kernel_sz, ramdisk_sz,
                                    console_suppression, console_dev_type,
                                    reserved_flag_0, reserved_flag_1)
    if vxe_FW_image:
        data += struct.pack("<I", vxe_size)
    if sps_image:
        data += struct.pack("<I", sps_size)
    if xen:
        data += struct.pack("<I", xen_sz)
    if domU_kernel:
        data += struct.pack("<II", domU_kernel_sz, domU_ramdisk_sz)
    write_padded(outfile_f, data, 3072)

# Write image data
    write_padded(outfile_f, bootstub_f.read(), 8192)
    write_padded(outfile_f, kernel_f.read(), kernel_sz)
    write_padded(outfile_f, ramdisk_f.read(), ramdisk_sz)
    if vxe_FW_image:
        write_padded(outfile_f, vxe_f.read(), vxe_size)
    if sps_image:
        write_padded(outfile_f, sps_f.read(), sps_size)
    if xen:
        write_padded(outfile_f, xen_f.read(), xen_sz)
    if domU_kernel:
        write_padded(outfile_f, domU_kernel_f.read(), domU_kernel_sz)
        outfile_f.write(domU_ramdisk_f.read())

# Close opened files
    if domU_kernel:
        domU_kernel_f.close()
        domU_cmdline_f.close()
        domU_ramdisk_f.close()
    if xen:
        xen_f.close()
        xen_cmdline_f.close()
    if sps_image:
        sps_f.close()
    if vxe_FW_image:
        vxe_f.close()

    kernel_f.close()
    kernel_cmdline_f.close()
    ramdisk_f.close()
    bootstub_f.close()
    outfile_f.close()


def usage():
    print "-b | --bootstub       Bootstub binary"
    print "-x | --xen            xen binary"
    print "-X | --xen-cmdline    xen command line file (max " + str(max_cmdline_size) + " bytes)"
    print "-k | --kernel         kernel bzImage binary"
    print "-K | --kernel-cmdline kernel command line file (max " + str(max_cmdline_size) + " bytes)"
    print "-r | --ramdisk        Ramdisk"
    print "-u | --domU-kernel    domU kernel bzImage binary"
    print "-U | --domU-cmdline   domU kernel command line file (max " + str(max_cmdline_size) + " bytes)"
    print "-R | --domU-ramdisk   domU ramdisk"
    print "-V | --vxe-fw         VXE FW image"
    print "-S | --sps-image      Secure Platform Services images"
    print "-o | --output         Output OS image"
    print "-h | --help           Show this message"
    print
    print "-b, -k, -K, -r, and -o are required"


def main():
    bootstub = None
    xen = None
    xen_cmdline = None
    kernel = None
    kernel_cmdline = None
    ramdisk = None
    vxe_FW_image = None
    sps_image = None
    domU_kernel = None
    domU_cmdline = None
    domU_ramdisk = None
    outfile = None
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hb:k:K:r:o:V:S:", ["help",
            "bootstub=", "kernel=", "kernel-cmdline=", "ramdisk=", "output=", "xen=", "xen-cmdline=", "domU-kernel=", "domU-cmdline=", "domU-ramdisk=",
            "vxe-fw=", "sps-image="])
    except getopt.GetoptError, err:
        usage()
        print err
        sys.exit(2)
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-b", "--bootstub"):
            bootstub = a
        elif o in ("-k", "--kernel"):
            kernel = a
        elif o in ("-K", "--kernel-cmdline"):
            kernel_cmdline = a
        elif o in ("-r", "--ramdisk"):
            ramdisk = a
        elif o in ("-o", "--output"):
            outfile = a
        elif o in ("-x", "--xen"):
            xen = a
        elif o in ("-X", "--xen-cmdline"):
            xen_cmdline = a
        elif o in ("-u", "--domU-kernel"):
            domU_kernel = a
        elif o in ("-U", "--domU-cmdline"):
            domU_cmdline = a
        elif o in ("-R", "--domU-ramdisk"):
            domU_ramdisk = a
        elif o in ("-V", "--vxe-fw"):
            vxe_FW_image = a
        elif o in ("-S", "--sps-image"):
            sps_image = a
        else:
            usage()
            sys.exit(2)
    if (not bootstub or not kernel or not kernel_cmdline or not ramdisk or not outfile):
        usage()
        print "Missing required option!"
        sys.exit(2)
    make_osimage(bootstub, xen, xen_cmdline, kernel, kernel_cmdline, ramdisk, domU_kernel, domU_cmdline, domU_ramdisk, vxe_FW_image, sps_image, outfile)

if __name__ == "__main__":
    main()
