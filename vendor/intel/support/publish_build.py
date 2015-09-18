#!/usr/bin/env python
# this script publish the flash files
import sys
import shutil
import glob
import os
import json
import zipfile
import re
from flashfile import FlashFile
from subprocess import Popen, PIPE

bldpub = None
ifwi_external_dir = "prebuilts/intel/vendor/intel/fw/prebuilts/ifwi"
ifwi_private_dir = "vendor/intel/fw/PRIVATE/ifwi"
flashfile_version = "1.0"

def get_link_path(gl, quiet=False):
    if os.path.islink(gl):
        if quiet is False:
            print "\t- Link found for %s" % (os.path.basename(gl))
        return os.path.relpath(os.path.realpath(gl))
    elif os.path.exists(gl):
        if quiet is False:
            print "\t- File found for %s" % (os.path.basename(gl))
        return gl
    else:
        if quiet is False:
            print "\t- No file found for %s" % (os.path.basename(gl))
    #return None if nothing found


def get_build_options(key, key_type=None, default_value=None):
    try:
        value = os.environ[key]
        if key_type == 'boolean':
            return value.lower() in "true yes".split()
        elif key_type == 'hex':
            return int(value, 16)
        else:
            if len(value) != 0:
                print "Env key: %s set to '%s'" % (key, value)
                return value
    except KeyError:
        if default_value:
            return default_value
        else:
            print "Build option: " + key + " not set"
            #return None if nothing found

def init_global():
    global bldpub
    bldpub = get_build_options(key='TARGET_PUBLISH_PATH')


def publish_file_without_formatting(src, dst, enforce=True):
    if enforce:
        error = "error"
    else:
        error = "warning"
    # first glob the src specification
    srcl = glob.glob(src)
    if len(srcl) != 1:
        print >> sys.stderr, error, ", " + src + \
            " did not match exactly one file (matched %d files): %s" % (
                len(srcl), " ".join(srcl))
        if enforce:
            sys.exit(1)
        else:
            return
    src = srcl[0]
    # ensure dir are created
    os.system("mkdir -p " + dst)
    # copy
    print "   copy", src, dst
    shutil.copyfile(src, os.path.join(dst, os.path.basename(src)))


def publish_file(args, src, dest, enforce=True):
    return publish_file_without_formatting(src % args, dest % args,
                                           enforce=enforce)


def find_stitched_ifwis(ifwi_base_dir):
    ifwis = {}

    gl = os.path.join(ifwi_base_dir, "*/ifwi*")
    print "looking for ifwis in the tree for %s" % bld_prod
    for ifwidir in glob.glob(gl):
        ifwi_name = os.path.splitext(os.path.basename(ifwidir))[0]
        board_name = ifwi_name.replace("ifwi_", "")
        if os.path.exists(os.path.join(os.path.dirname(ifwidir), "version")):
            with open(os.path.join(os.path.dirname(ifwidir), "version"), "r") as fo:
                ifwiversion = fo.readline()[:-1]
        else:
            ifwiversion = os.path.splitext(os.path.basename(ifwidir))[0]
        print "->> found stitched ifwi %s version:%s in %s" % (ifwi_name, ifwiversion, ifwidir)
        path_ifwi_name = ifwi_name.replace("ifwi_", "")
        ifwis[board_name] = dict(ifwiversion=ifwiversion,
                                 ifwi=ifwidir,
                                 capsule=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['capsule_', path_ifwi_name, '.bin']))),
                                 fwdnx=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['dnx_fwr_', path_ifwi_name, '.bin']))),
                                 osdnx=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['dnx_osr_', path_ifwi_name, '.bin']))),
                                 softfuse=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['soft_fuse_', path_ifwi_name, '.bin']))),
                                 xxrdnx=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['dnx_xxr_', path_ifwi_name, '.bin']))),
                                 stage2=get_link_path(os.path.join(os.path.dirname(ifwidir), ''.join(['stage2_', path_ifwi_name, '.bin']))),
                                 ulpmc=get_build_options(key='ULPMC_BINARY'))

    return ifwis


def find_ifwis(board_soc):

    ifwi_out_dir = os.path.join('out/target/product', bld, 'ifwi')
    if os.path.exists(os.path.join(ifwi_out_dir, 'version')):
        """ Stitched ifwi availlable. Prebuilt ifwi remain in the legacy path"""
        return find_stitched_ifwis(ifwi_out_dir)
    """ walk the ifwi directory for matching ifwi
    return a dict indexed by boardname, with all the information in it"""
    ifwis = {}
    if os.path.exists(ifwi_private_dir):
        ifwi_base_dir = ifwi_private_dir
    else:
        ifwi_base_dir = ifwi_external_dir
    # IFWI for Merrifield/Moorefield VP, HVP and SLE are not published
    # Filter on _vp, _vp_next, _hvp, _hvp_next, _sle, _sle_next
    isvirtualplatorm = re.match('.*_(vp|hvp|sle)($|\s|_next($|\s))', bld_prod)
    if not(isvirtualplatorm):
        bios_type = get_build_options(key='TARGET_BIOS_TYPE', default_value='uefi')
        if bios_type != "uefi":
             ifwiglobs = {"redhookbay": "ctp_pr[23] ctp_pr3.1 ctp_vv2 ctp_vv3",
                          "redhookbay_xen": "ctp_pr[23]/XEN ctp_pr3.1/XEN",
                          "ctp7160": "cpa_v3_vv cpa_v3_vv_b0_b1",
                          "pf450cl": "me372cl/pf450cl",
                          "baylake": "baytrail/baylake",
                          "byt_t_ffrd8": "baytrail/byt_t"
                         }[bld_prod]
        else:
            kernel_x64_format = get_build_options(key='BOARD_USE_64BIT_KERNEL', default_value='false')
            if kernel_x64_format != "true":
                ifwiglobs = {"byt_t_crv2": "baytrail_edk2/byt_crv2/ia32",
                             "cht_rvp": "no_directory",
                             "byt_t_ffrd8": "baytrail_edk2/byt_t/ia32",
                             "bsw_rvp": "braswell_edk2/bsw_rvp"
                            }[bld_prod]
            else:
                ifwiglobs = {"byt_t_crv2": "baytrail_edk2/byt_crv2/x64",
                             "byt_t_ffrd8": "baytrail_edk2/byt_t/x64",
                             "cht_rvp": "no_directory"
                            }[bld_prod]

        print "looking for ifwis in the tree for %s" % bld_prod

        for ifwiglob in ifwiglobs.split(" "):
            gl = os.path.join(ifwi_base_dir, ifwiglob)
            base_ifwi = ifwiglob.split("/")[0]
            for ifwidir in glob.glob(gl):
                # When the ifwi directory is placed within a subdir, we shall
                # take the subdir in the board name
                nameindex = 0 - ifwiglob.count('/') - 1

                board = ifwidir.split("/")[nameindex]
                for idx in range(nameindex + 1, 0):
                    board = board + '_' + ifwidir.split("/")[idx]
                ifwi = get_link_path(os.path.join(ifwidir, "dediprog.bin"), True)
                if ifwi is None:
                    ifwi = get_link_path(os.path.join(ifwidir, "ifwi.bin"), True)
                ifwiversion = os.path.splitext(os.path.basename(ifwi))[0]
                if ifwiversion:
                    print "->> found ifwi %s version: %s in %s" % (os.path.basename(ifwi), ifwiversion, ifwidir)
                    capsule = get_link_path(os.path.join(ifwidir, "capsule.bin"))
                    fwdnx = get_link_path(os.path.join(ifwidir, "dnx_fwr.bin"))
                    osdnx = get_link_path(os.path.join(ifwidir, "dnx_osr.bin"))
                    xxrdnx = get_link_path(os.path.join(ifwidir, "dnx_xxr.bin"))
                    softfuse = get_link_path(os.path.join(ifwidir, "soft_fuse.bin"))
                    stage2 = get_link_path(os.path.join(ifwidir, "stage2.bin"))
                    ulpmc = get_build_options(key='ULPMC_BINARY')

                    ifwis[board] = dict(ifwiversion=ifwiversion,
                                        ifwi=ifwi,
                                        capsule=capsule,
                                        stage2=stage2,
                                        fwdnx=fwdnx,
                                        osdnx=osdnx,
                                        softfuse=softfuse,
                                        xxrdnx=xxrdnx,
                                        ulpmc=ulpmc)
    return ifwis


def get_publish_conf():
    res = get_build_options(key='PUBLISH_CONF', default_value=42)
    if res == 42:
        return None
    else:
        return json.loads(res)


def do_we_publish_bld_variant(bld_variant):
    r = get_publish_conf()
    if r:
        return bld_variant in r
    else:
        return True


def do_we_publish_extra_build(bld_variant, extra_build):
    """do_we_publish_bld_variant(bld_variant)
    must be True"""
    r = get_publish_conf()
    if r:
        return extra_build in r[bld_variant]
    else:
        return True

def publish_kernel_keys(product_out, bld_variant):
    # Publish the kernel module signing keys only for engineering and userdebug builds
    if bld_variant == "userdebug" or bld_variant == "eng":
        signing_keys_dir = os.path.join(bldpub, "signing_keys", bld_variant)
        publish_file(locals(), "%(product_out)s/linux/kernel/signing_key.priv", signing_keys_dir, enforce=False)
        publish_file(locals(), "%(product_out)s/linux/kernel/signing_key.x509", signing_keys_dir, enforce=False)

def publish_build_iafw(bld, bld_variant, bld_prod, buildnumber, board_soc):
    board = ""
    bld_supports_droidboot = get_build_options(key='TARGET_USE_DROIDBOOT', key_type='boolean')
    bld_supports_ramdump = get_build_options(key='TARGET_USE_RAMDUMP', key_type='boolean')
    bld_supports_silentlake = get_build_options(key='INTEL_FEATURE_SILENTLAKE', key_type='boolean')
    bldx = get_build_options(key='GENERIC_TARGET_NAME')
    publish_system_img = do_we_publish_extra_build(bld_variant, 'system_img')

    product_out = os.path.join("out/target/product", bld)
    fastboot_dir = os.path.join(bldpub, "fastboot-images", bld_variant)
    iafw_dir = os.path.join(bldpub, "iafw")
    flashfile_dir = os.path.join(bldpub, "flash_files")

    print "publishing fastboot images"
    publish_kernel_keys(product_out, bld_variant)
    if bld_supports_silentlake:
        publish_file(locals(), "%(product_out)s/sl_vmm.bin", fastboot_dir)
    system_img_path_in_out = None

    if bld_supports_droidboot:
        publish_file(locals(), "%(product_out)s/droidboot.img", fastboot_dir, enforce=False)
        publish_file(locals(), "%(product_out)s/droidboot.img.POS.bin", fastboot_dir, enforce=False)
        system_img_path_in_out = os.path.join(product_out, "system.img")
    else:
        publish_file(locals(), "%(product_out)s/recovery.img.POS.bin", fastboot_dir, enforce=False)
        system_img_path_in_out = os.path.join(product_out, "system.tar.gz")

    publish_file(locals(), "%(product_out)s/%(bldx)s-img-%(buildnumber)s.zip", fastboot_dir)

    if bld_supports_ramdump:
        publish_file(locals(), "%(product_out)s/ramdump.img", fastboot_dir, enforce=False)

    if publish_system_img:
        publish_file_without_formatting(system_img_path_in_out, fastboot_dir)

    publish_file(locals(), "%(product_out)s/installed-files.txt", fastboot_dir, enforce=False)
    publish_file(locals(), "%(product_out)s/ifwi/iafw/ia32fw.bin", iafw_dir, enforce=False)
    ifwis = find_ifwis(board_soc)

    f = FlashFile(os.path.join(flashfile_dir,  "build-" + bld_variant, "%(bldx)s-%(bld_variant)s-fastboot-%(buildnumber)s.zip" % locals()), "flash.xml")

    f.xml_header("fastboot", bld, flashfile_version)

    f.add_file("UPDATE", os.path.join(fastboot_dir,  bldx + "-img-" + buildnumber + ".zip"), buildnumber)

    if bld_supports_silentlake:
        f.add_file("SILENTLAKE", os.path.join(fastboot_dir, "sl_vmm.bin"), buildnumber)

    if bld_supports_droidboot:
        f.add_file("FASTBOOT", os.path.join(fastboot_dir, "droidboot.img"), buildnumber)

    for board, args in ifwis.items():
        if args["ulpmc"]:
            f.add_codegroup("ULPMC", (("ULPMC", args["ulpmc"], args["ifwiversion"]),))
        if args["capsule"]:
            f.add_codegroup("CAPSULE", (("CAPSULE_" + board.upper(), args["capsule"], args["ifwiversion"]),))
        else:
            if "PROD" not in args["ifwi"]:
                f.add_codegroup("FIRMWARE", (("IFWI_" + board.upper(), args["ifwi"], args["ifwiversion"]),
                                            ("FW_DNX_" + board.upper(),  args["fwdnx"], args["ifwiversion"])))

    f.add_buildproperties("%(product_out)s/system/build.prop" % locals())

    if bld_supports_droidboot:
        f.add_command("fastboot flash fastboot $fastboot_file", "Flashing fastboot")

    for board, args in ifwis.items():
        if args["capsule"]:
            f.add_command("fastboot flash capsule $capsule_%s_file" % (board.lower()), "Flashing capsule")
        else:
            if "PROD" not in args["ifwi"]:
                f.add_command("fastboot flash dnx $fw_dnx_%s_file" % (board.lower(),), "Attempt flashing ifwi " + board)
                f.add_command("fastboot flash ifwi $ifwi_%s_file" % (board.lower(),), "Attempt flashing ifwi " + board)
        if args["ulpmc"]:
            f.add_command("fastboot flash ulpmc $ulpmc_file", "Flashing ulpmc", retry=3, mandatory=0)

    publish_format_partitions(f, ["cache"])

    if bld_supports_silentlake:
        f.add_command("fastboot flash silentlake $silentlake_file", "Flashing silentlake")

    update_timeout = 600000
    f.add_command("fastboot update $update_file", "Flashing AOSP images", timeout=update_timeout)

    # build the flash-capsule.xml
    for board, args in ifwis.items():
        if args["capsule"]:
            f.add_xml_file("flash-capsule.xml")
            f_capsule = ["flash-capsule.xml"]
            f.xml_header("fastboot", bld, flashfile_version, xml_filter=f_capsule)
            f.add_codegroup("CAPSULE", (("CAPSULE_" + board.upper(), args["capsule"], args["ifwiversion"]),), xml_filter=f_capsule)
            f.add_buildproperties("%(product_out)s/system/build.prop" % locals(), xml_filter=f_capsule)
            f.add_command("fastboot flash capsule $capsule_%s_file" % (board.lower(),), "Attempt flashing ifwi " + board, xml_filter=f_capsule)
            f.add_command("fastboot continue", "Reboot", xml_filter=f_capsule)

    # build the flash-ramdump.xml to flash and enable the ramdump
    if bld_supports_ramdump:
        f.add_xml_file("flash-ramdump.xml")
        f_ramdump = ["flash-ramdump.xml"]
        f.xml_header("fastboot", bld, flashfile_version, xml_filter=f_ramdump)
        f.add_file("RAMDUMP", os.path.join(fastboot_dir, "ramdump.img"), buildnumber, xml_filter=f_ramdump)
        f.add_command("fastboot flash ramdump $ramdump_file", "Flashing ramdump", xml_filter=f_ramdump)
        f.add_command("fastboot oem custom_boot 0x80", "Enabling ramdump", xml_filter=f_ramdump)
        f.add_command("fastboot continue", "Reboot", xml_filter=f_ramdump)

    f.finish()


def publish_build_uefi(bld, bld_variant, bld_prod, buildnumber, board_soc):
    product_out = os.path.join("out/target/product", bld)
    fastboot_dir = os.path.join(bldpub, "fastboot-images")
    target2file = [("fastboot", "droidboot"), ("boot", "boot"),
                    ("recovery", "recovery"), ("system", "system")]
    bldx = get_build_options(key='GENERIC_TARGET_NAME')
    flashfile_dir = os.path.join(bldpub, "flash_files")
    publish_kernel_keys(product_out, bld_variant)

    f = FlashFile(os.path.join(flashfile_dir,  "build-" + bld_variant,
                               "%(bldx)s-%(bld_variant)s-fastboot-%(buildnumber)s.zip" % locals()), "flash.xml")
    f.xml_header("fastboot", bld, flashfile_version)

    publish_attach_target2file(f, product_out, buildnumber, target2file)
    f.add_file("esp_update", os.path.join(product_out, "esp.zip"), buildnumber);

    ifwis = find_ifwis(board_soc)
    for board, args in ifwis.items():
         if args["capsule"]:
             f.add_codegroup("CAPSULE", (("CAPSULE_" + board.upper(), args["capsule"], args["ifwiversion"]),))

    f.add_file("INSTALLER", "device/intel/baytrail/installer.cmd", buildnumber)

    f.add_buildproperties("%(product_out)s/system/build.prop" % locals())

    publish_erase_partitions(f, ["system"])
    publish_format_partitions(f, ["cache"])

    f.add_command("fastboot flash esp_update $esp_update_file", "Updating ESP.")

    publish_flash_target2file(f, target2file)

    for board, args in ifwis.items():
         if args["capsule"]:
              f.add_command("fastboot flash capsule $capsule_%s_file" % (board.lower(),), "Attempt flashing capsule " + board)

    f.add_command("fastboot continue", "Rebooting now.")
    f.finish()


def publish_ota_files(bld, bld_variant, bld_prod, buildnumber):
    target_name = get_build_options(key='GENERIC_TARGET_NAME')

    # Get publish options
    publish_inputs = do_we_publish_extra_build(bld_variant, 'ota_target_files')
    publish_full_ota = do_we_publish_extra_build(bld_variant, 'full_ota')

    # Set paths
    pub_dir_inputs = os.path.join(bldpub, "ota_inputs", bld_variant)
    pub_dir_full_ota = os.path.join(bldpub, "fastboot-images", bld_variant)

    product_out = os.path.join("out/target/product", bld)
    full_ota_basename = "%(target_name)s-ota-%(buildnumber)s.zip" % locals()
    full_ota_file = os.path.join(product_out, full_ota_basename)

    inputs_out = "%(product_out)s/obj/PACKAGING/target_files_intermediates" % locals()
    inputs_basename = "%(target_name)s-target_files-%(buildnumber)s.zip" % locals()
    inputs_file = os.path.join(inputs_out, inputs_basename)

    # Publish
    if publish_full_ota:
        publish_file_without_formatting(full_ota_file, pub_dir_full_ota, enforce=False)

    if publish_inputs and bld_variant.find("user") >= 0:
        publish_file_without_formatting(inputs_file, pub_dir_inputs, enforce=False)


def publish_ota_flashfile(bld, bld_variant, bld_prod, buildnumber):
    # Get values from environment variables
    supported = not(get_build_options(key='FLASHFILE_NO_OTA', key_type='boolean'))
    published = do_we_publish_extra_build(bld_variant, 'full_ota_flashfile')
    target_name = get_build_options(key='GENERIC_TARGET_NAME')

    if not supported or not published:
        print "Do not publish ota flashfile"
        return

    # Set paths
    pub_dir_flashfiles = os.path.join(bldpub, "flash_files")

    product_out = os.path.join("out/target/product", bld)
    full_ota_basename = "%(target_name)s-ota-%(buildnumber)s.zip" % locals()
    full_ota_file = os.path.join(product_out, full_ota_basename)

    # build the ota flashfile
    f = FlashFile(os.path.join(pub_dir_flashfiles,
                               "build-" + bld_variant,
                               "%(target_name)s-%(bld_variant)s-ota-%(buildnumber)s.zip" % locals()),
                               "flash.xml")
    f.xml_header("ota", bld, flashfile_version)
    f.add_file("OTA", full_ota_file, buildnumber)
    f.add_buildproperties("%(product_out)s/system/build.prop" % locals())
    f.add_command("adb root", "As root user")
    f.add_command("adb wait-for-device", "Wait ADB availability")
    f.add_command("adb shell rm /cache/recovery/update/*", "Clean cache")
    f.add_command("adb shell rm /cache/ota.zip", "Clean ota.zip")

    ota_push_timeout = 300000

    f.add_command("adb push $ota_file /cache/ota.zip", "Pushing update", timeout=ota_push_timeout)
    f.add_command("adb shell am startservice -a com.intel.ota.OtaUpdate -e LOCATION /cache/ota.zip",
                  "Trigger os update")
    f.finish()


def publish_ota(bld, bld_variant, bld_prod, buildnumber):
    publish_ota_files(bld, bld_variant, bld_prod, buildnumber)
    publish_ota_flashfile(bld, bld_variant, bld_prod, buildnumber)


def publish_erase_partitions(f, partitions):
    for part in partitions:
        f.add_command("fastboot erase " + part, "Erase '%s' partition." % part)

def publish_format_partitions(f, partitions):
    for part in partitions:
        f.add_command("fastboot format " + part, "Format '%s' partition." % part)

def publish_partitioning_commands(f, bld, buildnumber, filename, format_list):
    f.add_command("fastboot oem start_partitioning", "Start partitioning.")
    f.add_command("fastboot flash /tmp/%s $partition_table_file" % (filename,), "Push the new partition table to the device.")
    f.add_command("fastboot oem partition /tmp/%s" % (filename), "Apply the new partition scheme.")

    tag = "-EraseFactory"

    xml_tag_list = [i for i in f.xml.keys() if tag in i]
    f.add_command("fastboot format %s" % ("factory",), "Format '%s' partition." % ("factory",), xml_filter=xml_tag_list)

    publish_format_partitions(f, format_list)

    f.add_command("fastboot oem stop_partitioning", "Stop partitioning.")

def publish_blankphone_iafw(bld, buildnumber, board_soc):
    bld_supports_droidboot = get_build_options(key='TARGET_USE_DROIDBOOT', key_type='boolean')
    bldx = get_build_options(key='GENERIC_TARGET_NAME')
    gpflag = get_build_options(key='BOARD_GPFLAG', key_type='hex')
    single_dnx = get_build_options(key='SINGLE_DNX')
    config_list = get_build_options(key='CONFIG_LIST')
    product_out = os.path.join("out/target/product", bld)
    blankphone_dir = os.path.join(bldpub, "flash_files/blankphone")
    partition_filename = "partition.tbl"
    partition_file = os.path.join(product_out, partition_filename)
    part_scheme = get_build_options(key='TARGET_PARTITIONING_SCHEME', default_value='osip-gpt')

    if bld_supports_droidboot:
        if part_scheme == "full-gpt":
            recoveryimg = os.path.join(product_out, "droidboot_dnx.img")
        else:
            recoveryimg = os.path.join(product_out, "droidboot.img.POS.bin")
    else:
        recoveryimg = os.path.join(product_out, "recovery.img.POS.bin")
    ifwis = find_ifwis(board_soc)
    for board, args in ifwis.items():
        # build the blankphone flashfile
        f = FlashFile(os.path.join(blankphone_dir, "%(board)s-blankphone.zip" % locals()), "flash.xml")
        f.add_xml_file("flash-EraseFactory.xml")
        # create all config XML files including default config
        if config_list:
            for conf in config_list.split():
                f.add_xml_file("flash-%s.xml" % conf)

        default_files = f.xml.keys()

        if args["softfuse"]:
            softfuse_files = ["flash-softfuse.xml", "flash-softfuse-EraseFactory.xml"]
            for softfuse_f in softfuse_files:
                f.add_xml_file(softfuse_f)

            f.xml_header("system", bld, flashfile_version)
            f.add_gpflag(gpflag | 0x00000200, xml_filter=softfuse_files)
            f.add_gpflag(gpflag | 0x00000100, xml_filter=default_files)
        else:
            if bld == "baylake" or bld == "byt_t_ffrd8":
                f.xml_header("fastboot_dnx", bld, flashfile_version)
            elif args["capsule"]:
                f.xml_header("fastboot", bld, flashfile_version)
            else:
                f.xml_header("system", bld, flashfile_version)
                f.add_gpflag(gpflag, xml_filter=default_files)

        if args["capsule"]:
            default_ifwi = (("CAPSULE", args["capsule"], args["ifwiversion"]),
                            ("DEDIPROG",  args["ifwi"], args["ifwiversion"]),)
        else:
            if single_dnx == "true":
                default_ifwi = (("IFWI", args["ifwi"], args["ifwiversion"]),
                                ("FW_DNX",  args["fwdnx"], args["ifwiversion"]))
            else:
                default_ifwi = (("IFWI", args["ifwi"], args["ifwiversion"]),
                                ("FW_DNX",  args["fwdnx"], args["ifwiversion"]),
                                ("OS_DNX", args["osdnx"], args["ifwiversion"]))

        ifwis_dict = {}
        for xml_file in f.xml.keys():
            ifwis_dict[xml_file] = default_ifwi

        if args["xxrdnx"]:
            xxrdnx = ("XXR_DNX", args["xxrdnx"], args["ifwiversion"])
            for xml_file in f.xml.keys():
                ifwis_dict[xml_file] += (xxrdnx,)

        if args["softfuse"]:
            softfuse = ("SOFTFUSE", args["softfuse"], args["ifwiversion"])
            for xml_file in softfuse_files:
                ifwis_dict[xml_file] += (softfuse,)

        for xml_file in f.xml.keys():
            f.add_codegroup("FIRMWARE", ifwis_dict[xml_file], xml_filter=[xml_file])

        if part_scheme == "full-gpt":
            f.add_file("FASTBOOT", os.path.join(product_out, "droidboot.img"), buildnumber)

        if args["capsule"]:
            fastboot_dir = os.path.join(bldpub, "fastboot-images", bld_variant)
            f.add_file("FASTBOOT", os.path.join(product_out, "droidboot.img"), buildnumber)
            f.add_file("KERNEL", os.path.join(product_out, "boot.img"), buildnumber)
            f.add_file("RECOVERY", os.path.join(product_out, "recovery.img"), buildnumber)
            f.add_file("INSTALLER", "device/intel/baytrail/installer.cmd", buildnumber)
        else:
            f.add_codegroup("BOOTLOADER", (("KBOOT", recoveryimg, buildnumber),))

        f.add_codegroup("CONFIG", (("PARTITION_TABLE", partition_file, buildnumber),))

        f.add_buildproperties("%(product_out)s/system/build.prop" % locals())

        if args["capsule"]:
            if bld == "baylake" or bld == "byt_t_ffrd8":
                f.add_command("fastboot boot $fastboot_file", "Downloading fastboot image")
                f.add_command("fastboot continue", "Booting on fastboot image")
                f.add_command("sleep", "Sleep 25 seconds", timeout=25000)
            f.add_command("fastboot oem write_osip_header", "Writing OSIP header")
            f.add_command("fastboot flash boot $kernel_file", "Flashing boot")
            f.add_command("fastboot flash recovery $recovery_file", "Flashing recovery")
            f.add_command("fastboot flash fastboot $fastboot_file", "Flashing fastboot")

        if part_scheme == "full-gpt":
            f.add_command("fastboot oem erase_osip_header", "Erase OSIP header")

        publish_partitioning_commands(f, bld, buildnumber, partition_filename,
                                      ["cache", "config", "logs", "data"])

        # must provision the AOSP droidboot during blankphone
        if part_scheme == "full-gpt":
            f.add_command("fastboot flash fastboot $fastboot_file", "Flashing droidboot")

        fru_token_dir = fru_configs = get_build_options(key='FRU_TOKEN_DIR')
        if os.path.isdir(fru_token_dir):
            # Use tokens to set the fru
            for filename in os.listdir(fru_token_dir):
                f.filenames.add(os.path.join(fru_token_dir, filename))

        fru_configs = get_build_options(key='FRU_CONFIGS')
        if os.path.exists(fru_configs):
            # Use fastboot command to set the fru
            f.add_xml_file("flash-fru.xml")
            fru = ["flash-fru.xml"]
            f.xml_header("fastboot", bld, flashfile_version, xml_filter=fru)
            f.add_command("fastboot oem fru set $fru_value", "Flash FRU value on device", xml_filter=fru)
            f.add_command("popup", "Please turn off the board and update AOBs according to the new FRU value", xml_filter=fru)
            f.add_raw_file(fru_configs, xml_filter=fru)

        if args["capsule"] is None:
            # Creation of a "flash IFWI only" xml
            flash_IFWI = "flash-IFWI-only.xml"
            f.add_xml_file(flash_IFWI)
            f.xml_header("system", bld, flashfile_version, xml_filter=[flash_IFWI])
            f.add_gpflag((gpflag & 0xFFFFFFF8) | 0x00000102, xml_filter=[flash_IFWI])
            f.add_codegroup("FIRMWARE", default_ifwi, xml_filter=[flash_IFWI])

        if config_list:
            # populate alternate config XML files with corresponding configuration
            for conf in config_list.split():
                f.add_command("fastboot oem mount config ext4", "Mount config partition", xml_filter="flash-%s.xml" % conf)
                f.add_command("fastboot oem config %s" % conf, "Activating config %s" % conf, xml_filter="flash-%s.xml" % conf)

            if f.clear_xml_file("flash.xml"):
                f.xml_header("fastboot", bld, flashfile_version, xml_filter="flash.xml")
                f.add_command("popup","This build does not support generic flash file, please select the one that corresponds to your hardware", xml_filter="flash.xml")
                f.add_command("fastboot flash fail", "Fake command to always return a flash failure", xml_filter="flash.xml")
                f.add_gpflag((gpflag & 0xFFFFFFF8) | 0x00000045, xml_filter="flash.xml")

        # Create a dedicated flash file for buildbot
        f.copy_xml_file("flash.xml", "flash-buildbot.xml")

        f.finish()


def publish_attach_target2file(f, path, buildnumber, target2file):
    for target_file in target2file:
        f.add_file(target_file[1],
                   os.path.join(path, "%s.img" % target_file[1]), buildnumber)


def publish_flash_target2file(f, target2file):
    for target_file in target2file:
        flash_timeout = 420000 if target_file[0] == "system" else 60000
        f.add_command("fastboot flash %s $%s_file" % (target_file[0], target_file[1]),
                      "Flashing '%s' image." % target_file[0], timeout=flash_timeout)


def publish_blankphone_uefi(bld, buildnumber, board_soc):
    product_out = os.path.join("out/target/product", bld)
    fastboot_dir = os.path.join(bldpub, "fastboot-images")
    target2file = [("ESP", "esp"), ("fastboot", "droidboot")]

    blankphone_dir = os.path.join(bldpub, "flash_files/blankphone")
    bldx = get_build_options(key='GENERIC_TARGET_NAME')
    f = FlashFile(os.path.join(blankphone_dir, bldx + "-blankphone.zip"), "flash.xml")
    f.add_xml_file("flash-EraseFactory.xml")
    fastboot_mode = "fastboot_dnx"
    osloader = True

    if bld == "cht_rvp":
        fastboot_mode = "fastboot"
        osloader = False

    if bld == "byt_t_ffrd8":
        f.add_xml_file("flash-rvp8.xml")

    f.xml_header(fastboot_mode, bld, flashfile_version)

    publish_attach_target2file(f, product_out, buildnumber, target2file)
    if osloader:
        f.add_file("osloader", os.path.join(product_out, "efilinux-%s.efi" % bld_variant), buildnumber);

    f.add_file("INSTALLER", "device/intel/baytrail/installer.cmd", buildnumber)
    if bld == "byt_t_ffrd8":
        efilinuxcfg_file = os.path.join(product_out, "efilinux_fakebatt.cfg")
        fe = open(efilinuxcfg_file, 'w')
        fe.write('-e fake \n')
        fe.close()
        f.add_codegroup("CONFIG", (("EFILINUX_CFG", efilinuxcfg_file, buildnumber),), xml_filter=["flash-rvp8.xml"])

    ifwis = find_ifwis(board_soc)
    for board, args in ifwis.items():
         if args["ifwi"]:
              f.add_codegroup("FIRMWARE", (("DEDIPROG_" + board.upper(), args["ifwi"], args["ifwiversion"]),))
         if args["stage2"]:
              f.add_codegroup("stage2", (("stage2_" + board.upper(), args["stage2"], args["ifwiversion"]),))

    part_file = os.path.join(product_out, "partition.tbl")
    f.add_codegroup("CONFIG", (("PARTITION_TABLE", part_file, buildnumber),))

    f.add_buildproperties("%(product_out)s/system/build.prop" % locals())

    for board, args in ifwis.items():
         if args["stage2"]:
              f.add_command("fastboot flash fw_stage2 $stage2_%s_file" % (board.lower(),),
                            "Uploading Stage 2 IFWI image.", mandatory=0)
              f.add_command("sleep", "Sleep for 5 seconds.", timeout=5000)

    if osloader:
        f.add_command("fastboot flash osloader $osloader_file",
                      "Uploading EFI OSLoader image.")
        f.add_command("fastboot boot $droidboot_file", "Uploading fastboot image.")
        f.add_command("sleep", "Sleep for 25 seconds.", timeout=25000)

    f.add_command("fastboot oem wipe ESP", "Wiping ESP partition.", mandatory=0);
    f.add_command("fastboot oem wipe reserved", "Wiping reserved partition.", mandatory=0);
    publish_partitioning_commands(f, bld, buildnumber, os.path.split(part_file)[1],
                                  ["cache", "config", "logs", "data"])

    publish_flash_target2file(f, target2file)

    if bld == "byt_t_ffrd8":
        f.add_command("fastboot oem mount ESP vfat", "Mounting ESP partition.", xml_filter=["flash-rvp8.xml"])
        f.add_command("fastboot flash /mnt/ESP/EFI/BOOT/efilinux.cfg $efilinux_cfg_file","Copy efilinux.cfg", xml_filter=["flash-rvp8.xml"])

    f.copy_xml_file("flash.xml", "flash-buildbot.xml")

    f.finish()


def publish_kernel(bld, bld_variant):
    product_out = os.path.join("out/target/product", bld)
    fastboot_dir = os.path.join(bldpub, "fastboot-images", bld_variant)

    print "publishing fastboot images"
    # everything is already ready in product out directory, just publish it
    publish_file(locals(), "%(product_out)s/boot.img", fastboot_dir)


def generateAllowedPrebuiltsList(customer):
    """return a list of private paths that:
       - belong to bsp-priv manifest group
       - and have customer annotation <customer>_external='bin'"""
    # we need to specify a group in the repo forall command due to following repo behavior:
    # if a project doesn't have the searched annotation, it is included in the list of repo projects to process
    cmd = "repo forall -g bsp-priv -a %s_external=bin -c 'echo $REPO_PATH'" % (customer,)
    p = Popen(cmd, stdout=PIPE, close_fds=True, shell=True)
    allowedPrebuiltsList, _ = p.communicate()
    # As /PRIVATE/ has been replaced by /prebuilts/<ref_product>/ in prebuilts dir,
    # we need to update regexp accordingly.
    # allowedPrebuilts are only directory path:
    # to avoid /my/path/to/diraaa/dirb/file1 matching
    # /my/path/to/dira, we add a trailing '/' to the path.
    return [allowedPrebuilts.replace("/PRIVATE/", "/prebuilts/[^/]+/") + '/' for allowedPrebuilts in allowedPrebuiltsList.splitlines()]


def publish_stitched_ifwi(ifwis, board_soc, z):

    def write_ifwi_bin(board, fn, arcname):
        arcname = os.path.join(ifwi_external_dir, board, arcname)
        print fn, "->", arcname
        z.write(fn, arcname)

    def find_sibling_file(fn, _type, possibilities):
        dn = os.path.dirname(fn)
        for glform in possibilities:
            glform = os.path.join(dn, *glform)
            gl = glob.glob(glform)
            print >> sys.stderr, "\npath:", gl, glform
            if len(gl) == 1:
                return gl[0]
        print >> sys.stderr, "unable to find %s for external release" % (_type,)
        print >> sys.stderr, "please put the file in:"
        print >> sys.stderr, possibilities
        sys.exit(1)

    if ifwis:
        for k, v in ifwis.items():
            write_ifwi_bin(k, v["ifwi"], "ifwi.bin")
            v["androidmk"] = find_sibling_file(v["ifwi"], "Android.mk",
                                                [("", "Android.mk"),
                                                 ("..", "Android.mk")])
            write_ifwi_bin(k, v["androidmk"], "Android.mk")

            v["version"] = find_sibling_file(v["ifwi"], "version",
                                             [("", "version")])
            write_ifwi_bin(k, v["version"], "version")

            if v["fwdnx"]:
                write_ifwi_bin(k, v["fwdnx"], "dnx_fwr.bin")
            if v["osdnx"]:
                write_ifwi_bin(k, v["osdnx"], "dnx_osr.bin")
            if v["capsule"]:
                write_ifwi_bin(k, v["capsule"], "capsule.bin")
                v["capsule"] = find_sibling_file(v["capsule"], "prod capsule",
                                                 [("PROD", "*EXT.cap")])
                write_ifwi_bin(k, v["capsule"], "capsule-prod.bin")
        write_ifwi_bin("common", os.path.join(ifwi_private_dir, "common", "external_Android.mk"), "Android.mk")
    z.close()


def publish_external(bld, bld_variant, board_soc):
    os.system("mkdir -p " + bldpub)
    product_out = os.path.join("out/target/product", bld)
    prebuilts_out = os.path.join(product_out, "prebuilts/intel")
    prebuilts_pub = os.path.join(bldpub, "prebuilts.zip")
    # white_list contains the list of regular expression
    # matching the PRIVATE folders whose we want to publish binary.
    # We use 'generic' customer config to generate white list
    white_list = generateAllowedPrebuiltsList("g")
    # Need to append the top level generated makefile that includes all prebuilt makefiles
    white_list.append(os.path.relpath(os.path.join(prebuilts_out, "Android.mk"), product_out))
    white_list = re.compile("(%s)" % ("|".join(white_list)),)
    if os.path.exists(prebuilts_out):
        z = zipfile.ZipFile(prebuilts_pub, "w")
        # automatic prebuilt publication
        for root, dirs, files in os.walk(prebuilts_out):
            for f in files:
                filename = os.path.join(root, f)
                arcname = filename.replace(product_out, "")
                if white_list.search(arcname):
                    z.write(filename, arcname)
                    print arcname

        # IFWI publication
        ifwis = find_ifwis(board_soc)

        if os.path.exists(os.path.join('out/target/product', bld, 'ifwi/version')):
            return publish_stitched_ifwi(ifwis, board_soc, z)

        def write_ifwi_bin(board, fn, arcname):
            arcname = os.path.join(ifwi_external_dir, board, arcname)
            print fn, "->", arcname
            z.write(fn, arcname)

        def find_sibling_file(fn, _type, possibilities, depth=1):
            dn = os.path.dirname(fn)
            while depth >=1:
                for glform in possibilities:
                    glform = os.path.join(dn, *glform)
                    gl = glob.glob(glform)
                    if len(gl) == 1:
                        return gl[0]
                dn = os.path.split(dn)[0]
                depth -= 1
            print >> sys.stderr, "unable to find %s for external release" % (_type,)
            print >> sys.stderr, "please put the file in:"
            print >> sys.stderr, possibilities
            sys.exit(1)

        bios_type = get_build_options(key='TARGET_BIOS_TYPE', default_value='uefi')
        if ifwis and bios_type != "uefi":
            for k, v in ifwis.items():
                write_ifwi_bin(k, v["ifwi"], "ifwi.bin")
                v["ifwi"] = find_sibling_file(v["ifwi"], "prod ifwi",
                                              [("PROD", "*CRAK_PROD.bin"),
                                               ("..", "PROD", "*CRAK_PROD.bin"),
                                               ("PROD", "*EXT.bin")]
                                             )
                v["androidmk"] = find_sibling_file(v["ifwi"], "Android.mk",
                                                   [("Android.mk",)], depth=6)
                if v["fwdnx"]:
                    write_ifwi_bin(k, v["fwdnx"], "dnx_fwr.bin")
                if v["osdnx"]:
                    write_ifwi_bin(k, v["osdnx"], "dnx_osr.bin")
                if v["capsule"]:
                    write_ifwi_bin(k, v["capsule"], "capsule.bin")
                    v["capsule"] = find_sibling_file(v["capsule"], "prod capsule",
                                                     [("PROD", "*EXT.cap")])
                    write_ifwi_bin(k, v["capsule"], "capsule-prod.bin")
                write_ifwi_bin(k, v["ifwi"], "ifwi-prod.bin")
                write_ifwi_bin(k, v["androidmk"], "Android.mk")
            commonandroidmk = find_sibling_file(v["ifwi"], "external_Android.mk",
                                                [("common", "external_Android.mk")], depth=8)
            write_ifwi_bin("common", commonandroidmk, "Android.mk")
        z.close()


if __name__ == '__main__':
    # Arguments from the command line
    goal = sys.argv[1]
    bld_prod = sys.argv[2].lower()
    bld = sys.argv[3].lower()
    bld_variant = sys.argv[4]
    buildnumber = sys.argv[5]
    board_soc = sys.argv[6]
    if len(sys.argv) > 7:
        xen_flag = sys.argv[7]
        if xen_flag.lower() == "true":
            bld_prod += "_xen"

    # Arguments from the environment
    bootonly_flashfile = get_build_options(key='FLASHFILE_BOOTONLY',
                                           key_type='boolean',
                                           default_value=False)

    init_global()

    # When bootonly is set, only accept fastboot_flashfile goal
    if bootonly_flashfile:
        if goal == "fastboot_flashfile":
            publish_kernel(bld, bld_variant)
        else:
            print "FLASHFILE_BOOTONLY: Nothing to publish"
        sys.exit(0)

    bios_type = get_build_options(key='TARGET_BIOS_TYPE', default_value='uefi')

    # Publish goal
    if goal == "blankphone":
        locals()["publish_blankphone_" + bios_type](bld, buildnumber, board_soc)

    elif goal == "publish_intel_prebuilts":
        publish_external(bld, bld_variant, board_soc)

    elif goal == "fastboot_flashfile":
        if do_we_publish_bld_variant(bld_variant):
            locals()["publish_build_" + bios_type](bld, bld_variant, bld_prod, buildnumber, board_soc)

    elif goal == "ota_flashfile":
        if do_we_publish_bld_variant(bld_variant):
            publish_ota(bld, bld_variant, bld_prod, buildnumber)

    else:
        print "Unsupported publish goal [%s]" % goal
        sys.exit(1)
