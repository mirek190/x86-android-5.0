# Copyright (C) 2012 The Android Open Source Project
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

"""Emit extra commands needed for Group during OTA installation
(installing the bootloader)."""

import struct
import common


def WriteCapsule(info):
  info.script.AppendExtra('package_extract_file("capsule.bin", "/tmp/capsule.bin");')
  info.script.AppendExtra("""flash_capsule("/tmp/capsule.bin");""")

def WriteEspUpdate(info):
  info.script.AppendExtra('package_extract_file("esp.zip", "/tmp/esp.zip");')
  info.script.AppendExtra("""flash_esp_update("/tmp/esp.zip");""")

def WriteDroidboot(info):
  info.script.WriteRawImage("/fastboot", "droidboot.img")

def FullOTA_InstallEnd(info):
  try:
    capsule = info.input_zip.read("RADIO/capsule.bin")
  except KeyError:
    print "no capsule in target_files; skipping install"
  else:
    common.ZipWriteStr(info.output_zip, "capsule.bin", capsule)
    WriteCapsule(info)
  try:
    esp = info.input_zip.read("RADIO/esp.zip")
  except KeyError:
    print "no esp in target_files; skipping install"
  else:
    common.ZipWriteStr(info.output_zip, "esp.zip", esp)
    WriteEspUpdate(info)
  try:
    droidboot_img = info.input_zip.read("RADIO/droidboot.img")
  except KeyError:
    print "no droidboot.img in target_files; skipping install"
  else:
    common.ZipWriteStr(info.output_zip, "droidboot.img", droidboot_img)
    WriteDroidboot(info)


def IncrementalOTA_InstallEnd(info):
  try:
    target_capsule = info.target_zip.read("RADIO/capsule.bin")
    try:
      source_capsule = info.source_zip.read("RADIO/capsule.bin")
    except KeyError:
      source_capsule = None
    if source_capsule == target_capsule:
      print "capsule unchanged; skipping"
    else:
      print "capsule changed; adding it"
      common.ZipWriteStr(info.output_zip, "capsule.bin", target_capsule)
      WriteCapsule(info)
  except KeyError:
    print "no capsule in target target_files; skipping install"

  try:
    target_esp = info.target_zip.read("RADIO/esp.zip")
    try:
      source_esp = info.source_zip.read("RADIO/esp.zip")
    except KeyError:
      source_esp = None
    if source_esp == target_esp:
      print "esp unchanged; skipping"
    else:
      print "esp changed; adding it"
      common.ZipWriteStr(info.output_zip, "esp.zip", target_esp)
      WriteEspUpdate(info)
  except KeyError:
    print "no esp in target target_files; skipping install"

  try:
    target_droidboot_img = info.target_zip.read("RADIO/droidboot.img")
    try:
      source_droidboot_img = info.source_zip.read("RADIO/droidboot.img")
    except KeyError:
      source_droidboot_img = None
    if source_droidboot_img == target_droidboot_img:
      print "droidboot unchanged; skipping"
    else:
      print "droidboot changed; adding it"
      common.ZipWriteStr(info.output_zip, "droidboot.img", target_droidboot_img)
      WriteDroidboot(info)
  except KeyError:
    print "no droidboot in target target_files; skipping install"
