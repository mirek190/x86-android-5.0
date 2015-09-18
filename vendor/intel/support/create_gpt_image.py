#!/usr/bin/env python
#
# -*- coding: utf-8 -*-
# -*- tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
#
# ex:enc=utf-8
# ex:ts=4:sw=4:sts=4:et
#

# create_gpt_image.py : script to create and show GPT image informations
# Copyright (c) 2014, Intel Corporation.
# Author: Perrot Thomas <thomasx.perrot@intel.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.

"""
Create and show GPT image informations
"""

from sys import exit as sys_exit, version_info

if version_info < (2, 7, 3):
    exit('Python version must be 2.7.3 or higther')

from logging import debug, info, error, DEBUG, INFO, getLogger, \
    basicConfig
from argparse import ArgumentParser
from os import environ, remove, stat
from os.path import isdir, isfile, normcase, normpath, realpath, join, basename
from struct import unpack, pack
from uuid import UUID, uuid4
from binascii import crc32
from re import compile as re_compile
from collections import namedtuple
from subprocess import call


class MBRInfos(object):
    """
    Named tuple of MBR informations

    # Raw MBR is a little-endian struct with:
    # ---------------------------------------------
    # name           type       size        format
    # ---------------------------------------------
    # boot            int       4           I
    # OS type         int       4           I
    # starting LBA    uint      4           I
    # size in LBA     uint      4           I
    # dummy 1         char[]    430 char    430s
    # dummy 2         char[]    16 char     16s
    # dummy 3         char[]    48 char     48s
    # sign            char[]    2 char      2s
    # ---------------------------------------------
    """
    __slots__ = ('block_size', 'raw', 'boot', 'os_type', 'lba_start', \
                     'lba_size', 'dummy_1', 'dummy_2', 'dummy_3', 'sign')

    _FMT = '<IIII430s16s48s2s'
    _PART_ENTRY = '\x00\x00\x02\x00\xee\x9b\xf7\xcc\x01\x00\x00\x00\x27\x6b' \
        '\xee\x00'

    def __init__(self, block_size=512):
        self.block_size = block_size
        self.raw = ''

        # TODO use decorator to remove many attributs and replace those by r/w
        # in raw attribut with pack and unpack function
        self.boot = 0
        self.os_type = 0
        self.os_type = 0
        self.lba_start = 0
        self.lba_size = 0
        self.dummy_1 = ''
        self.dummy_2 = ''
        self.dummy_3 = ''
        self.sign = '\x55\xaa'

    def __repr__(self):
        # compute size in GBytes
        human_size = (self.lba_size / self.block_size) / (1024 * 1024 * 1024)
        chs_starting = self.boot & 0x00FFFFFF
        chs_ending = self.os_type & 0x00FFFFFF

        result = 'MBR:\n'
        result = '{0}\tboot: 0x{1:02x}\n'.format(result, self.boot)
        result = '{0}\tOS type: 0x{1:02x}\n'.format(result, self.os_type)
        result = '{0}\tCHS : starting {1:d}, ending {2:d}\n' \
            .format(result, chs_starting, chs_ending)
        result = '{0}\tLBA: start {1:d}, size {2:d}\n' \
            .format(result, self.lba_start, self.lba_size)
        result = '{0}\tsize: {1:d} GBytes\n'.format(result, human_size)

        return result

    def read(self, img_file, offset=0):
        """
        Used to extract informations of raw gpt obtains after reading a image
        """
        # read image file
        img_file.seek(offset)
        self.raw = img_file.read(self.block_size)

        # unpack raw MBR to a named tuple
        self.boot, self.os_type, self.lba_start, self.lba_size, self.dummy_1, \
            self.dummy_2, self.dummy_3, self.sign \
            = unpack(MBRInfos._FMT, self.raw)

    def write(self, img_file, offset=0):
        """
        Used to write MBR in a image file
        """
        self.raw = pack(MBRInfos._FMT, self.boot, self.os_type, \
                            self.lba_start, self.lba_size, '', \
                            MBRInfos._PART_ENTRY, '', self.sign)
        img_file.seek(offset)
        img_file.write(self.raw)


class GPTHeaderInfos(object):
    """
    Named tuple of GPT header informations

    # GPT header format
    # -------------------------------------------------------------------------
    # offset	   Length	Contents
    # -------------------------------------------------------------------------
    # 0 (0x00)	   8 bytes	Signature ('EFI PART',
    #                           45h 46h 49h 20h 50h 41h 52h 54h)
    # 8 (0x08)	   4 bytes	Revision (for GPT version 1.0 (through at least
    #                           UEFI version 2.3.1), the value is
    #                           00h 00h 01h 00h)
    # 12 (0x0C)	   4 bytes	Header size in little endian (in bytes,
    #                           usually 5Ch 00h 00h 00h meaning 92 bytes)
    # 16 (0x10)	   4 bytes	CRC32 of header (offset +0 up to header size),
    #                           with this field zeroed during calculation
    # 20 (0x14)	   4 bytes	Reserved; must be zero
    # 24 (0x18)	   8 bytes	Current LBA (location of this header copy)
    # 32 (0x20)	   8 bytes	Backup LBA (location of the other header copy)
    # 40 (0x28)	   8 bytes	First usable LBA for partitions (primary
    #                           partition table last LBA + 1)
    # 48 (0x30)    8 bytes	Last usable LBA (secondary partition table first
    #                           LBA - 1)
    # 56 (0x38)	   16bytes	Disk GUID (also referred as UUID on UNIXes)
    # 72 (0x48)	   8 bytes	Starting LBA of array of partition entries
    #                           (always 2 in primary copy)
    # 80 (0x50)	    4 bytes	Number of partition entries in array
    # 84 (0x54)	    4 bytes	Size of a single partition entry (usually 128)
    # 88 (0x58)	    4 bytes	CRC32 of partition array
    # 92 (0x5C)	    *	        Reserved; must be zeroes for the rest of the
    #                           block (420 bytes for a sector size of 512 bytes
    #                           but can be more with larger sector sizes)
    # ------------------------------------------------------------------------
    # LBA size	    Total
    # ------------------------------------------------------------------------
    """
    __slots__ = ('raw', 'sign', 'rev', 'size', 'crc', 'lba_current', \
                     'lba_backup', 'lba_first', 'lba_last', 'uuid', \
                     'lba_start', 'table_length', 'entry_size', \
                     'table_crc')

    _FMT = '<8s4sII4xQQQQ16sQIII'

    def __init__(self, img_size=2, block_size=512, size=92):
        self.raw = ''

        # TODO use decorator to remove many attributs and replace those by r/w
        # in raw attribut with pack and unpack function
        self.sign = 'EFI PART'
        self.rev = '\x00\x00\x01\x00'
        self.size = size

        # init GPT partition table length and entry size with default value
        self.table_length = 128
        self.entry_size = 128

        # calculate the size of image in block
        size_in_block = (img_size * 1024 * 1024 * 1024) / block_size

        # init lba backup at the value of first lba used by GPT backup
        self.lba_backup = size_in_block - 1

        # calculate the size of partition table in block
        table_size = (self.table_length * self.entry_size) / block_size

        # init lba first at first usable lba for the partitions
        self.lba_first = table_size + 2

        # init last lba at last usable lba for the partitions
        self.lba_last = size_in_block - 2 - table_size

        # Generate a unique disk GUID
        self.uuid = uuid4().bytes_le

        # init lba start at the value of first lba used by GPT header
        self.lba_start = size_in_block - 1 - table_size

        self.crc = 0
        self.lba_current = 0
        self.table_crc = 0

    def __repr__(self):
        result = 'GPT Header:\n'
        result = '{0}\tsignature: {1}\n'.format(result, self.sign)
        result = '{0}\trevision: {1}\n'.format(result, self.rev)
        result = '{0}\tsize: {1} bytes\n'.format(result, self.size)
        result = '{0}\tCRC32: {1}\n'.format(result, self.crc)

        result = '{0}\tLBAs:\n'.format(result)
        result = '{0}\t\t current: {1}\n'.format(result, self.lba_current)
        result = '{0}\t\t backup: {1}\n'.format(result, self.lba_backup)
        result = '{0}\t\t first usable: {1}\n'.format(result, self.lba_first)
        result = '{0}\t\t last usable: {1} - {2}\n' \
            .format(result, self.lba_last, self.lba_start)

        result = '{0}Disk UUID: {1}\n' \
            .format(result, UUID(bytes_le=self.uuid))

        result = '{0}partition entries:\n'.format(result)
        result = '{0}\tstarting LBA: {1}\n'.format(result, self.lba_start)
        result = '{0}\tnumber of partition entries: {1}\n' \
            .format(result, self.table_length)
        result = '{0}\tsize of a single partition: {1}\n' \
            .format(result, self.entry_size)
        result = '{0}\tCRC32 of partition array: {1}\n' \
            .format(result, self.table_crc)

        return result

    def read(self, img_file, offset):
        """
        Used to extract informations of raw gpt obtains after reading a image
        """

        # read image file
        img_file.seek(offset)
        self.raw = img_file.read(self.size)

        # unpack raw GPT header to a named tuple
        self.sign, self.rev, self.size, self.crc, self.lba_current, \
            self.lba_backup, self.lba_first, self.lba_last, self.uuid, \
            self.lba_start, self.table_length, self.entry_size, \
            self.table_crc = unpack(GPTHeaderInfos._FMT, self.raw)

    def write(self, img_file, offset, block_size):
        """
        Used to write GPT header and backup in a image file
        """
        self.raw = pack(GPTHeaderInfos._FMT, self.sign, self.rev, \
                            self.size, 0, 1, self.lba_backup, \
                            self.lba_first, self.lba_last, self.uuid, \
                            2, self.table_length, self.entry_size, 0)

        backup_raw = pack(GPTHeaderInfos._FMT, self.sign, self.rev, \
                              self.size, 0, self.lba_backup, 1, \
                              self.lba_first, self.lba_last, self.uuid, \
                              self.lba_start, self.table_length, \
                              self.entry_size, 0)

        # write a new GPT header
        img_file.seek(offset)
        img_file.write(self.raw)

        # write zero on GPT header not used blocks
        raw_stuffing = '\x00' * (block_size - len(self.raw))
        img_file.write(raw_stuffing)

        # save the end of GPT header
        gpt_header_end = img_file.tell()

        # write a new GPT backup
        backup_position = self.lba_backup * block_size
        img_file.seek(backup_position)
        img_file.write(backup_raw)

        # write zero on GPT backup not used blocks
        img_file.write(raw_stuffing)

        # position image file pointer to the end of GPT header wrote
        img_file.seek(gpt_header_end)


class PartTableInfos(list):
    """
    The list of partition table entries
    """
    __slots__ = ('raw')

    def __init__(self):
        super(PartTableInfos, self).__init__()
        self.raw = ''

    def __repr__(self):
        result = 'Partitions table:\n'
        for entry in self:
            if UUID(bytes_le=entry.type) == \
                    UUID('00000000-0000-0000-0000-000000000000'):
                continue
            result = '{0}{1}'.format(result, entry)

        return result

    def read(self, img_file, offset, length, entry_size):
        """
        Read the partition table from a GPT image
        """
        img_file.seek(offset)
        self.raw = img_file.read(length * entry_size)

        # read each entry of partition table
        for i in xrange(length):
            entry = TableEntryInfos(i, entry_size)
            entry.read(self.raw)
            self.append(entry)

    def write(self, img_file, offset, entry_size, tlb_infos, last_usable):
        """
        Used to write GPT partitions tables in a image file
        """
        # Erase the partition table entries
        self = []

        # write all new partition entries in primary GPT
        current_offset = offset
        for pos, part_info in enumerate(tlb_infos):
            entry = TableEntryInfos(pos, entry_size)
            entry.write(img_file, current_offset, part_info)
            current_offset += entry_size
            self.append(entry)

        # copy all partition entries wrote from primary GPT to
        # the secondary GPT
        img_file.seek(offset)
        raw_entries_size = current_offset - offset
        raw_entries = img_file.read(raw_entries_size)
        img_file.seek(last_usable + 1)
        img_file.write(raw_entries)

        img_file.seek(current_offset)


class TableEntryInfos(object):
    """
    A entry of the partition table

    # UUID partition entry format
    # -------------------------------------------------------------------------
    # Offset	   Length      Contents
    # -------------------------------------------------------------------------
    # 0 (0x00)	   16 bytes    Partition type GUID
    # 16 (0x10)	   16 bytes    Unique partition GUID
    # 32 (0x20)	   8 bytes     First LBA (little endian)
    # 40 (0x28)	   8 bytes     Last LBA (inclusive, usually odd)
    # 48 (0x30)	   8 bytes     Attribute flags (e.g. bit 60 denotes read-only)
    # 56 (0x38)	   72 bytes    Partition name (36 UTF-16LE code units)
    # -------------------------------------------------------------------------
    # 128 bytes total
    # -------------------------------------------------------------------------
    """
    __slots__ = ('pos', 'size', 'raw', 'type', 'uuid', 'lba_first', \
                     'lba_last', 'attr', 'name')

    _FMT = '<16s16sQQQ72s'

    def __init__(self, pos, size):
        self.pos = pos
        self.size = size
        self.raw = ''

        self.type = ''
        self.uuid = ''
        self.lba_first = 0
        self.lba_last = 0
        self.attr = 0
        self.name = ''

    def __repr__(self):
        result = 'UUID partition entry {0}\n'.format(self.pos)
        result = '\t{0}type: {1}\n'.format(result, UUID(bytes_le=self.type))
        result = '\t{0}UUID: {1}\n'.format(result, UUID(bytes_le=self.uuid))
        result = '\t{0}lfirst LBA: {1}\n'.format(result, self.lba_first)
        result = '\t{0}last LBA: {1}\n'.format(result, self.lba_last)
        result = '\t{0}attribute flags: 0x{1:08x}\n'.format(result, self.attr)
        result = '\t{0}name: {1}\n' \
            .format(result, self.name.decode('utf-16le'))
        result = '\t{0}size: {1}\n'.format(result, \
                                               self.lba_last - self.lba_first)

        return result

    def read(self, raw_table):
        """
        Read a partition table entry from a GPT image
        """
        # compute start and end of part in the raw table used for this entry
        raw_entry_start = self.pos * self.size
        raw_entry_end = (self.pos + 1) * self.size
        self.raw = raw_table[raw_entry_start:raw_entry_end]

        # unpack raw partition table entry to a named tuple
        self.type, self.uuid, self.lba_first, self.lba_last, self.attr, \
            self.name = unpack(TableEntryInfos._FMT, self.raw)

    def write(self, img_file, offset, entry_info):
        """
        Use to write a partition table entries in a image file
        """
        types = {
            'Unused': '00000000-0000-0000-0000-000000000000',
            'MBR Scheme': '024DEE41-33E7-11D3-9D69-0008C781F39F',
            'EFI System': 'C12A7328-F81F-11D2-BA4B-00A0C93EC93B',
            'efi': 'C12A7328-F81F-11D2-BA4B-00A0C93EC93B',
            'BIOS Boot': '21686148-6449-6E6F-744E-656564454649',

            'Microsoft Reserved': 'E3C9E316-0B5C-4DB8-817D-F92DF00215AE',
            'Microsoft Basic Data': 'EBD0A0A2-B9E5-4433-87C0-68B6B72699C7',
            'Microsoft Logical Disk Manager metadata': \
                '5808C8AA-7E8F-42E0-85D2-E1E90434CFB3',
            'Microsoft Logical Disk Manager data': \
                'AF9B60A0-1431-4F62-BC68-3311714A69AD',

            'Apple HFS+': '48465300-0000-11AA-AA11-00306543ECAC',
            'Apple UFS': '55465300-0000-11AA-AA11-00306543ECAC',
            'Apple RAID': '52414944-0000-11AA-AA11-00306543ECAC',
            'Apple RAID (offline)': '52414944-5F4F-11AA-AA11-00306543ECAC',
            'Apple Boot': '426F6F74-0000-11AA-AA11-00306543ECAC',
            'Apple Label': '4C616265-6C00-11AA-AA11-00306543ECAC',
            'Apple TV Recovery': '5265636F-7665-11AA-AA11-00306543ECAC',
            'Apple ZFS': '6A898CC3-1DD2-11B2-99A6-080020736631',

            'Intel Android': '0fc63daf-8483-4772-8e79-3d69d8477de4',
            'data': '0fc63daf-8483-4772-8e79-3d69d8477de4'
            }

        if entry_info.type in types:
            tuuid = UUID(types[entry_info.type]).bytes_le
        else:
            error('Unknown partiton type: {0}'.format(entry_info.type))
            raise SystemExit
        puuid = UUID(entry_info.uuid).bytes_le
        last = int(entry_info.size) + int(entry_info.begin) - 1

        self.raw = pack(TableEntryInfos._FMT, tuuid, \
                            puuid, \
                            int(entry_info.begin), last, \
                            int(entry_info.tag), \
                            entry_info.label.encode('utf-16le'))

        img_file.seek(offset)
        img_file.write(self.raw)


TLB_INFO = namedtuple('TLB_INFO', ('begin', 'size', 'type', 'uuid', 'label', \
                                       'tag', 'part', 'device'))


class TLBInfos(list):
    """
    TLB informations extracted from TLB partition file
    """
    __slots__ = ('path')

    def __init__(self, path):
        super(TLBInfos, self).__init__()
        self.path = path

    def __repr__(self):
        result = ''

        for item in self:
            line = 'add -b {0} -s {1} -t {2} -u {3} -l {4} -T {5} -P {6} ' \
                '{7}\n'.format(item.begin, item.size, item.type, item.uuid, \
                                   item.label, item.tag, item.part, \
                                   item.device)
            result = '{0}{1}'.format(result, line)

        return result

    def read(self):
        """
        Read a TLB file
        """
        with open(self.path, 'r') as tlb_file:
            re_parser = re_compile(r'^add\s-b\s(?P<begin>\w+)\s-s\s' \
                                       '(?P<size>[\w$()-]+)\s-t\s' \
                                       '(?P<type>\w+)\s-u\s' \
                                       '(?P<uuid>[\w-]+)\s' \
                                       '-l\s(?P<label>\w+)\s-T\s' \
                                       '(?P<tag>\w+)\s-P\s(?P<part>\w+)\s' \
                                       '(?P<device>[\w//]+)\s$')

            for line in tlb_file:
                debug('TLB reading line: {0}'.format(line))
                parsed_line = re_parser.match(line)

                if parsed_line:
                    debug('TLB parsed line: {0}'.format(line))
                    debug('\t begin: {0}'.format(parsed_line.group('begin')))
                    debug('\t size: {0}'.format(parsed_line.group('size')))
                    debug('\t type: {0}'.format(parsed_line.group('type')))
                    debug('\t uuid: {0}'.format(parsed_line.group('uuid')))
                    debug('\t label: {0}'.format(parsed_line.group('label')))
                    debug('\t tag: {0}'.format(parsed_line.group('tag')))
                    debug('\t part: {0}'.format(parsed_line.group('part')))
                    debug('\t device: {0}'.format(parsed_line.group('device')))

                    ntuple = TLB_INFO(*parsed_line.groups())
                    self.append(ntuple)
                else:
                    debug('TLB not parsed line: {0}'.format(line))

    def compute_last_size_entry(self, img_size, block_size, entry_size, \
                                    table_length):
        """
        Compute the size of the last TLB entry
        """
        re_last = re_compile(r'\$calc\(\$lba_end-[0-9]+\)')
        if re_last.match(self[-1].size):
            size_in_block = (img_size * 1024 * 1024 * 1024) / block_size
            entries_size_in_block = (entry_size * table_length) / block_size
            value = size_in_block - 1 - entries_size_in_block \
                - int(self[-1].begin)
            self[-1] = self[-1]._replace(size=value)


class GPTImage(object):
    """
    GPT image
    """
    __slots__ = ('path', 'size', 'block_size', 'mbr', \
                     'gpt_header', 'table')

    DEFAULT_ANDROID_PARTITIONS = {
        'ESP': 'esp.img',
        'reserved': 'none',
        'boot': 'boot.img',
        'recovery': 'recovery.img',
        'fastboot': 'droidboot.img',
        'reserved_1': 'none',
        'test': 'none',
        'panic': 'none',
        'factory': 'dummy.img',
        'misc': 'none',
        'config': 'dummy.img',
        'cache': 'dummy.img',
        'logs': 'dummy.img',
        'system': 'system.img',
        'data': 'dummy.img',
        }

    def __init__(self, path, size=2, block_size=512, gpt_header_size=92):
        self.path = path
        self.size = size
        self.block_size = block_size

        self.mbr = MBRInfos(self.block_size)
        self.gpt_header = GPTHeaderInfos(size, block_size, gpt_header_size)
        self.table = PartTableInfos()

    def __repr__(self):

        result = 'Read EFI informations from {0}\n'.format(self.path)
        result = '{0}{1}'.format(result, self.mbr)
        result = '{0}{1}'.format(result, self.gpt_header)
        result = '{0}{1}'.format(result, self.table)

        return result

    def read(self):
        """
        Read information from a GPT image
        """
        # open and read image
        with open(self.path, 'rb') as img_file:

            # read MBR
            debug('read MBR from {0}'.format(self.path))
            self.mbr.read(img_file)

            # read GPT header
            debug('read GPT header from {0}'.format(self.path))
            offset = self.block_size
            self.gpt_header.read(img_file, offset)

            # read partition table
            debug('read partitions table from {0}'.format(self.path))
            offset = self.block_size * self.gpt_header.lba_start
            self.table.read(img_file, offset, self.gpt_header.table_length, \
                                self.gpt_header.entry_size)

    def _write_partitions(self, img_file, tlb_infos, binaries_path):
        """
        Used to write partitions of image with binary files given. Call by
        write method
        """
        for tlb_part in tlb_infos:
            bin_path = binaries_path[tlb_part.label]
            offset = int(tlb_part.begin) * self.block_size

            # dummy
            if basename(bin_path) == 'dummy.img':
                make_dummy_bin(bin_path, tlb_part.size, self.block_size, \
                                   tlb_part.label)
            # system
            elif basename(bin_path) == 'system.img':
                make_system_bin(bin_path)
                bin_path = '{0}.decomp'.format(bin_path)

            # none
            if basename(bin_path) == '':
                line = '\0x00'
                img_file.seek(offset)
                img_file.write(line)
                continue

            # check partition size >= binary file
            bin_size = stat(bin_path).st_size / 512
            if tlb_part.size < bin_size:
                error('Size of binary file is greather than partition ' \
                          'size: {0}'.format(bin_path))
                raise SystemExit

            # open and read binary file to write partition
            with open(bin_path, 'rb') as bin_file:
                img_file.seek(offset)
                # Doesn't work if image size exceed the largest integer on that
                # machine, image size is intepreted as a negative size by
                # Python interpeter
                # for line in bin_file:
                #     img_file.write(line)
                while True:
                    data = bin_file.read(8192)
                    if not data:
                        break
                    img_file.write(data)

            # delete dummy.img or system.img.decomp wrote
            if basename(bin_path) == 'dummy.img' \
                    or basename(bin_path) == 'system.img.decomp':
                remove(bin_path)

    def _write_crc(self, img_file):
        """
        Calculate and write CRC32 of GPT partition table, header and backup
        """
        # read the partition table
        img_file.seek(2 * self.block_size)
        raw_table = img_file.read(self.gpt_header.table_length \
                                      * self.gpt_header.entry_size)

        # calculate CRC 32 of partition table
        table_crc = crc32(raw_table) & 0xffffffff

        # create a raw with the calculate CRC32 of partition table
        raw_table_crc = pack('<I', table_crc)

        # write calculate CRC 32 of partition table in GPT header
        img_file.seek(self.block_size + 88)
        img_file.write(raw_table_crc)

        # write calculate CRC 32 of partition table in GPT backup
        size_bytes = self.size * 1024 * 1024 * 1024
        img_file.seek(size_bytes - self.block_size + 88)
        img_file.write(raw_table_crc)

        # read the GPT header
        img_file.seek(self.block_size)
        raw_header = img_file.read(self.gpt_header.size)

        # calcultate CRC 32 of GPT header
        header_crc = crc32(raw_header) & 0xffffffff

        # create a raw with the calculate CRC32 of GPT header
        raw_header_crc = pack('<I', header_crc)

        # write calculate CRC 32 of GPT header
        img_file.seek(self.block_size + 16)
        img_file.write(raw_header_crc)

        # read the GPT backup
        img_file.seek(size_bytes - self.block_size)
        raw_backup = img_file.read(self.gpt_header.size)

        # calcultate CRC 32 of GPT backup
        backup_crc = crc32(raw_backup) & 0xffffffff

        # create a raw with the calculate CRC32 of GPT backup
        raw_backup_crc = pack('<I', backup_crc)

        # write calculate CRC 32 of GPT backup
        img_file.seek(size_bytes - self.block_size + 16)
        img_file.write(raw_backup_crc)

    def write(self, tlb_infos, binaries_path):
        """
        Used to write a new GPT image with values read in TLB file and the
        binaries
        """
        with open(self.path, 'wb+') as img_file:
            info('Start writing of image file: {0}'.format(self.path))

            # fill output image header with 0x00: MBR size + GPT header size +
            # (partition table length * entry size)
            zero = '\x00' * (2 * self.block_size + \
                self.gpt_header.table_length * self.gpt_header.entry_size)
            img_file.seek(0)
            img_file.write(zero)

            info('Writing MBR of image file: {0}'.format(self.path))
            offset = 0
            self.mbr.write(img_file, offset)

            info('Writing GPT Header of image file: {0}'.format(self.path))
            offset = self.block_size
            self.gpt_header.write(img_file, offset, self.block_size)

            info('Writing partitions table of image file: {0}' \
                     .format(self.path))
            offset = 2 * self.block_size
            self.table.write(img_file, offset, self.gpt_header.entry_size, \
                                 tlb_infos, self.gpt_header.lba_last)

            info('Writing partitions of image files {0}'.format(self.path))
            self._write_partitions(img_file, tlb_infos, binaries_path)

            info('Calculating CRCs and write them')
            self._write_crc(img_file)

            info('Image file {0} created successfully !!'.format(self.path))


def make_dummy_bin(path, part_size, block_size, label):
    """
    Write a empty binary file
    """
    host_out_dir = environ.get('ANDROID_HOST_OUT', '')
    make_bin = join(host_out_dir, 'bin', 'make_ext4fs')

    # check output directy exist
    if not isfile(make_bin):
        error('Invalid make fs command: {0}'.format(make_bin))
        raise SystemError

    # convert binary size in block
    part_size = int(part_size) * int(block_size)

    # make dummy.img
    cmd = '{0} -l {2} -L {3} {1} 1> {1}.{3}.out.log 2> ' \
        '{1}.{3}.err.log' .format(make_bin, path, part_size, label)
    call(cmd, shell=True)

    if not isfile(path):
        error('{0} isn\'t created'.format(path))
        raise SystemExit


def make_system_bin(path):
    """
    Write a system.img.decomp file
    """
    host_out_dir = environ.get('ANDROID_HOST_OUT', '')
    make_bin = join(host_out_dir, 'bin', 'simg2img')

    # check output directy exist
    if not isfile(make_bin):
        error('Invalid make fs command: {0}'.format(make_bin))
        raise SystemError

    cmd = '{0} {1} {1}.decomp'.format(make_bin, path)
    call(cmd, shell=True)

    if not isfile(path):
        error('{0} isn\'t created'.format(path))
        raise SystemExit


def usage():
    """
    Used to make main args parser and helper
    """
    # command line arguments parser definition
    desc = 'This script can check, show and create a GPT image'
    cmdparser = ArgumentParser(description=desc)

    # command line option used to specify the GPT image filename
    cmdparser.add_argument('file', type=str, help='filename of GPT image')

    # command line option used to specify the path of working directory
    cmdparser.add_argument('-p', '--path', action='store', \
                               type=str, default=environ.get('OUT', ''), \
                               help='path of working directory')

    # command line option used to create a GPT image
    cmdparser.add_argument('-c', '--create', action='store_true', \
                               help='GPT image filename')

    # command line option used to show information read in a GPT image
    cmdparser.add_argument('-s', '--show', action='store_true', \
                            help='show contents of GPT image file')

    # commande line option used to specify the path of TBL file
    cmdparser.add_argument('-t', '--table', action='store', \
                            default='partition.tbl', \
                            help='partition table filename')

    # command line option used to specify a new block size value
    cmdparser.add_argument('-b', '--block', action='store', type=int, \
                            default=512, help='block size in bytes')

    # command line option used to specify the size of image wrote
    cmdparser.add_argument('-l', '--len', action='store', type=int, \
                               default=2, \
                               help='length of the new image file in GBytes')

    # command line option to print debug informations
    cmdparser.add_argument('-g', '--debug', action='store_true', \
                               help='verbose debug informations')

    # command line option used to specify binary filename used to wrote
    # partitions of new image file
    for (key, value) in GPTImage.DEFAULT_ANDROID_PARTITIONS.items():
        cmdparser.add_argument('--{0}'.format(key), action='store', type=str, \
                                   default=value, \
                                   help='binary filename for the ' \
                                   'partition {0}'.format(value))

    return cmdparser


def main():
    """
    Main function used to created or show GPT image informations
    """
    # catch command line arguments
    cmdargs = usage().parse_args()

    # set verbose informations
    logger = getLogger()
    # basicConfig(format=' %(levelname)s %(filename)s:%(lineno)s: %(message)s')
    basicConfig(format=' %(levelname)s %(message)s')
    if cmdargs.debug:
        logger.setLevel(DEBUG)
    else:
        logger.setLevel(INFO)

    # print a message when GPT image path default value is used
    if cmdargs.path == environ.get('OUT', ''):
        info('working directory isn\'t set, default value used: {0}' \
                 .format(cmdargs.path))

    # normalise working directory path and check if it is available
    working_dir = realpath(normpath(normcase(cmdargs.path)))
    if not isdir(working_dir):
        error('Invalid working directory: {0}'.format(working_dir))
        raise SystemExit

    # TODO check the working directory has free space

    # init block size value, default value used is 512 Bytes
    block_size = cmdargs.block

    # check if block size value is valid
    if block_size <= 0:
        error('Invalid block size value: {0}'.format(block_size))
        raise SystemExit

    # normalize GPT image file path
    img_path = realpath(normpath(normcase(join(cmdargs.path, cmdargs.file))))

    # init and check the image size value, default value used is 2 GBytes
    img_size = cmdargs.len
    if img_size <= 0:
        error('Invalid image size value: {0} GBytes'.format(img_size))
        raise SystemExit
    else:
        info('Image size value: {0} GBytes'.format(img_size))

    # instanciate GPTImage class with GPT image path and block size value
    gpt_img = GPTImage(img_path, img_size, block_size)

    # process the command create, to write GPT image through a TBL file and
    # binary filenames
    if cmdargs.create:

        # remove image file if already exist
        if isfile(img_path):
            info('Image already exist, it\'s removed: {0}'.format(img_path))
            remove(img_path)

        # normalize and test TBL file path is available
        tlb_path = realpath(normpath(normcase(join(working_dir, \
                                                       cmdargs.table))))
        if not isfile(tlb_path):
            error('Invalid TBL file path: {0}'.format(tlb_path))
            raise SystemExit

        # read TLB file
        tlb_infos = TLBInfos(tlb_path)
        info('Read TLB file: {0}'.format(tlb_path))
        tlb_infos.read()
        tlb_infos.compute_last_size_entry(gpt_img.size, gpt_img.block_size, \
                                              gpt_img.gpt_header.entry_size, \
                                              gpt_img.gpt_header.table_length)

        # check TLB file read contains valid informations
        if not tlb_infos:
            error('Invalid TLB file: {0}'.format(tlb_path))
            raise SystemExit

        # print TLB informations read
        debug(tlb_infos)

        # check all binary filenames used to wrote image are availables
        binaries_path = {}
        # contruct binaries path and check it
        for key in GPTImage.DEFAULT_ANDROID_PARTITIONS:
            binary_path = realpath(normpath(normcase(
                        join(working_dir, getattr(cmdargs, key)))))

            # check binary file need exist and contruct the list
            # if binary file is none
            if basename(binary_path) == 'none':
                debug('Partition {0} don\'t use a binary file'.format(key))
                binaries_path[key] = ''
            # if binary file is dummy.img
            elif basename(binary_path) == 'dummy.img':
                debug('Partition {0} use a binary file: {1}' \
                         .format(key, binary_path))
                binaries_path[key] = binary_path
            # if binary file need don't exist
            elif not isfile(binary_path):
                error('Invalid binary for the partition {0}: {1}' \
                          .format(key, binary_path))
                raise SystemExit
            # if binary file need exist
            else:
                debug('Partition {0} use a binary file: {1}' \
                         .format(key, binary_path))
                binaries_path[key] = binary_path

        # call function to write new image file
        gpt_img.write(tlb_infos, binaries_path)

    # check if image file is available
    if not isfile(img_path):
        error('Image file not found: {0}'.format(img_path))
        raise SystemExit

    # read image file to check it's well, it uses CRC32
    gpt_img.read()

    # process the command show, to print EFI informations
    if cmdargs.show:
        print gpt_img

    sys_exit(0)

if __name__ == '__main__':
    main()
