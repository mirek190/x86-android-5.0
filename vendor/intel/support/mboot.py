#!/usr/bin/env python

# mboot.py : script to unpack and re-pack boot.img for Android
# Copyright (c) 2014, Intel Corporation.
# Author: Falempe Jocelyn <jocelyn.falempe@intel.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

import os
import subprocess
from optparse import OptionParser
import struct
import re
import shutil

# call an external command
# optional parameter edir is the directory where it should be executed.
def call(cmd, edir=''):
    if options.verbose:
        print '[', edir, '] Calling', cmd

    if edir:
        origdir = os.getcwd()
        os.chdir(os.path.abspath(edir))

    P = subprocess.Popen(args=cmd, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE, shell=True)

    if edir:
        os.chdir(origdir)

    stdout, stderr = P.communicate()
    if P.returncode:
        print cmd
        print "Failed " + stderr
        raise Exception('Error, stopping')
    return stdout

def write_file(fname, data, odir=True):
    if odir and options.dir:
        fname = os.path.join(options.dir, fname)
    print 'Write  ', fname
    out = open(fname, 'w')
    out.write(data)
    out.close()

def read_file(fname, odir=True):
    if odir and options.dir:
        fname = os.path.join(options.dir, fname)
    print 'Read   ', fname
    f = open(fname, 'r')
    data = f.read()
    f.close()
    return data

# unpack the ramdisk to outdir
# caution, outdir is removed with rmtree() before unpacking
def unpack_ramdisk(fname, outdir):
    print 'Unpacking ramdisk to', outdir

    call('gunzip -f ' + fname, options.dir)
    fname = re.sub(r'\.gz$', '', fname)

    if os.path.exists(outdir):
        shutil.rmtree(outdir)

    os.mkdir(outdir)
    call('cpio -i < ../' + fname, edir=outdir)

# Return next few bytes from file f
def check_byte(f, size):
    off = f.tell()
    byte = f.read(size)
    f.seek(-size, os.SEEK_CUR)
    return byte

# Intel legacy format
def unpack_bootimg_intel(fname):
    f = open(fname, 'r')

    sig = f.read(512)

# it may have 480 bytes more of signature on some products
    if not check_byte(f, 1).isalnum():
        sig += f.read(480)

    cmdline_block = f.read(4096)
    bootstub = f.read(4096)
# bootstub is 4k, but can be 8k on some products
    if check_byte(f, 1).isalnum():
        bootstub += f.read(4096)

    kernelsize, ramdisksize = struct.unpack('II', cmdline_block[1024:1032])

    print 'kernel size  ', kernelsize
    print 'ramdisk size ', ramdisksize

    if kernelsize < 500000 or kernelsize > 15000000:
        print 'Error kernel size likely wrong'
        return

    if ramdisksize < 10000 or ramdisksize > 300000000:
        print 'Error ramdisk size likely wrong'
        return

    kernel = f.read(kernelsize)
    ramdisk = f.read(ramdisksize)

    cmdline = cmdline_block[0:1024]
    cmdline = cmdline.rstrip('\x00')
    parameters = cmdline_block[1032:1040]

    write_file('sig', sig)
    write_file('cmdline.txt', cmdline)
    write_file('parameter', parameters)
    write_file('bootstub', bootstub)
    write_file('kernel', kernel)
    write_file('ramdisk.cpio.gz', ramdisk)

    f.close()
    unpack_ramdisk('ramdisk.cpio.gz', os.path.join(options.dir, 'extracted_ramdisk'))

def skip_pad(f, pgsz):
    npg = ((f.tell() / pgsz) + 1)
    f.seek(npg * pgsz)

# Google mkbootimg standard format
def unpack_bootimg_google(fname):
    f = open(fname, 'r')

    header = f.read(64)
    kernelsize = struct.unpack('I',header[8:12])[0]
    ramdisksize = struct.unpack('I',header[16:20])[0]
    pagesize = struct.unpack('I',header[36:40])[0]
    sigsize = struct.unpack('I',header[40:44])[0]

    print 'kernel size  ', kernelsize
    print 'ramdisk size ', ramdisksize
    print 'page size    ', pagesize
    print 'sig size     ', sigsize

    cmdline = f.read(512)
    checksum = f.read(32)
    cmdline += f.read(1024)
    skip_pad(f, pagesize)
    kernel = f.read(kernelsize)
    skip_pad(f, pagesize)
    ramdisk = f.read(ramdisksize)
    skip_pad(f, pagesize)

    cmdline = cmdline.rstrip('\x00')

    write_file('cmdline.txt', cmdline)
    write_file('kernel', kernel)
    write_file('ramdisk.cpio.gz', ramdisk)

    f.close()
    unpack_ramdisk('ramdisk.cpio.gz', os.path.join(options.dir, 'extracted_ramdisk'))

def unpack_bootimg(fname):
    if options.dir == 'tmp_boot_unpack' and os.path.exists(options.dir):
        print 'Removing ', options.dir
        shutil.rmtree(options.dir)

    print 'Unpacking', fname, 'into', options.dir
    if options.dir:
        if not os.path.exists(options.dir):
            os.mkdir(options.dir)

    f = open(fname, 'r')
    magic = f.read(8)
    f.close()
    if magic == 'ANDROID!':
        unpack_bootimg_google(fname)
    else:
        unpack_bootimg_intel(fname)

def pack_ramdisk(dname):
    dname = os.path.join(options.dir, dname)
    print 'Packing directory [', dname, '] => ramdisk.cpio.gz'
    call('find . | cpio -o -H newc > ../ramdisk.cpio', dname)
    call('gzip -f ramdisk.cpio', options.dir)

def pack_bootimg_intel(fname):
    pack_ramdisk('extracted_ramdisk')
    kernel = read_file('kernel')
    ramdisk = read_file('ramdisk.cpio.gz')

    cmdline = read_file('cmdline.txt')
    cmdline_block = cmdline + (1024 - len(cmdline)) * '\0'
    cmdline_block += struct.pack('II', len(kernel), len(ramdisk))
    cmdline_block += read_file('parameter')
    cmdline_block += '\0' * (4096 - len(cmdline_block))

    sig = read_file('sig')

    data = cmdline_block
    data += read_file('bootstub')
    data += kernel
    data += ramdisk

    topad = 512 - (len(data) % 512)
    data += '\xFF' * topad

    #update signature
    n_block = (len(data) / 512)
    new_sig = sig[0:48] + struct.pack('I', n_block) + sig[52:]

    data = new_sig + data

    write_file(fname, data, odir=False)

def main():
    global options
    usage = 'usage: %prog [options] boot.img\n\n' \
            '    unpack boot.img into separate files,\n' \
            '    OR pack a directory with kernel/ramdisk/bootstub  into a boot.img\n' \
            '    Default is to (unpack to / pack from) tmp_boot_unpack\n\n' \
            'Example : \n' \
            ' To unpack a boot.img image\n' \
            '    mboot.py -u boot.img\n' \
            ' modify tmp_boot_unpack/extracted_ramdisk/init.rc and run\n' \
            '    mboot.py boot-new.img'

    parser = OptionParser(usage, version='%prog 0.1')
    parser.add_option("-v", "--verbose",
                      action="store_true", dest="verbose")
    parser.add_option("-u", "--unpack",
                      action="store_true", dest="unpack", help='split boot image into kernel, ramdisk, bootstub ...')

    parser.add_option("-d", "--directory", dest="dir", default='tmp_boot_unpack',
                      help="extract boot.img to DIR, or create boot.img from DIR")

    (options, args) = parser.parse_args()


    if len(args) != 1:
        parser.error("takes exactly 1 argument")

    bootimg = args[0]

    if options.unpack:
        unpack_bootimg(bootimg)
        return

    if options.dir and not os.path.isdir(options.dir):
        print 'error ', options.dir, 'is not a valid directory'
        return

    pack_bootimg_intel(bootimg)

if __name__ == "__main__":
    main()
