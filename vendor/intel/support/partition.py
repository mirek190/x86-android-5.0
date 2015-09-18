#!/usr/bin/python

import sys
import json
import os.path
import collections

"""
    Description:
        Purpose of this script is to generate fstab files based on JSON
        description files for all platform.

    Support:
        - MBR
        - GPT
        - JSON base file for partition description
        - JSON base file for partition to mount for each OS
        - JSON override file
"""



"""
    Name    :   merge_dict
    Role    :   Merge two dictionaries recursively: it handles sub-dictionaries
    Input   :
                base_dict       : dictionary to override
                override_dict   : override content
    Output  :   None
    Return  :   overrided dictionary
"""
def merge_dict(base_dict, override_dict):

    for k, v in override_dict.iteritems():
        if not isinstance(v, collections.Mapping):
            base_dict[k] = override_dict[k]
        else:
            sub_dict = merge_dict(base_dict.get(k, {}), v)
            base_dict[k] = sub_dict

    return base_dict


"""
    Name    :   get_dict_from
    Role    :   Get dictionary from a JSON structure/dict
    Input   :
                json_array  : dictionary
                k           : key
    Output  :   None
    Return  :   return sub-dictionary if exists or empty dict
"""
def get_dict_from(json_array, k):
    if json_array.has_key(k):
        return json_array[k]
    else:
        return dict()


"""
    Name    :   update_fill_list
    Role    :   Update indentation dict
    Input   :
                l: list of words forming a fstab line
                d: indent dictionary
    Output  :   d updated if word entry does not exist
                or word length greater than previous (indentation)
    Return  :   None
"""
def update_fill_list(l, d):
    for i in range(0, len(l)):
        if d.has_key(i) is False or len(l[i]) > d[i]:
            d[i] = len(l[i])


"""
    Name    :   generate_gpt_fstab_file
    Role    :   Generate a "GPT" fstab file
                (GPT means mouting with label or uuid)
                Feature disabled until all patches merged
    Input   :
                storage: dictionary with storage parameters
                      (storage key in JSON)
                global_data: dictionary with global parameters
                             (global key in JSON)
                os_mount: dictionary with partition to be mounted for each OS
    Output  :   None
    Return  :   tuple: Fstab lines (list), and associated indentation dictionary
"""
def generate_gpt_fstab_file(storage, global_data, os_mount):

    recovery_lines = list()
    fill = dict()

    for partition_name in os_mount["mount"]:

        partition = get_dict_from(partition_data, partition_name)

        if partition["label"] is None:
            display_partition_name = partition_name
        else:
            display_partition_name = partition["label"]

        if partition["mount_point"] is None:
            display_mount_point = "/%s" % partition_name
        else:
            display_mount_point = "/%s" % partition["mount_point"]

        current_line = list()
        current_line.append(global_data["gpt"]["base_name_name"] + display_partition_name)
        current_line.append("\t")
        current_line.append("%s" % (display_mount_point))
        current_line.append("\t")
        current_line.append("%s" % (partition["fs_type"]))
        current_line.append("\t")
        current_line.append("%s" % (partition["mount_options"]))
        current_line.append("\t")
        current_line.append("%s" % (partition["fs_mgr"]))
        current_line.append("\n")
        update_fill_list(current_line, fill)
        recovery_lines.append(current_line)

    for k in global_data.keys():
        if "used" in global_data[k] and "fstab_entry" in global_data[k]:
            if (global_data[k]["used"]) == 1:
                recovery_lines.append(global_data[k]["fstab_entry"])
                update_fill_list(current_line, fill)

    return recovery_lines, fill

"""
    Name    :   generate_gpt_fstab_file
    Role    :   Generate a "MBR" fstab file
                (MBR means mouting with block id)
    Input   :
                storage: dictionary with storage parameters
                      (storage key in JSON)
                global_data: dictionary with global parameters
                             (global key in JSON)
                os_mount: dictionary with partition to be mounted for each OS
    Output  :   None
    Return  :   tuple: Fstab lines (list), and associated indentation dictionary
"""
def generate_mbr_fstab_file(storage, global_data, os_mount):

    recovery_lines = list()
    fill = dict()

    for partition_name in os_mount["mount"]:

        partition = get_dict_from(partition_data, partition_name)

        if partition["label"] is None:
            display_partition_name = partition_name
        else:
            display_partition_name = partition["label"]

        if partition["mount_point"] is None:
            display_mount_point = "/%s" % partition_name
        else:
            display_mount_point = "/%s" % partition["mount_point"]

        current_line = list()

        partition_id = partition["id"]

        current_line.append(global_data[""] % (storage["base_name"], partition_id))
        current_line.append("\t")
        current_line.append("%s" % (display_mount_point))
        current_line.append("\t")
        current_line.append("%s" % (partition["fs_type"]))
        current_line.append("\t")
        current_line.append("%s" % (partition["mount_options"]))
        current_line.append("\t")
        current_line.append("%s" % (partition["fs_mgr"]))
        current_line.append("\n")
        update_fill_list(current_line, fill)
        recovery_lines.append(current_line)

    return recovery_lines, fill


"""
    Name    :   generate_gpt_partition_file
    Role    :   Generate a "GPT" partition file for fastboot OS
                File pushed on target, and fastboot command reads it to
                apply partition scheme.
    Input   :
                storage: dictionary with storage parameters
                      (storage key in JSON)
                global_data: dictionary with global parameters
                             (global key in JSON)
    Output  :   None
    Return  :   Partition file lines (list)
"""
def generate_gpt_partition_file(storage, global_data):

    gpt_commands = list()

    gpt_commands.append("create -z %s" % (storage["base_name"]))
    gpt_commands.append("create %s" % (storage["base_name"]))
    gpt_commands.append("boot -p %s" % (storage["base_name"]))
    gpt_commands.append("reload %s" % (storage["base_name"]))

    lba_start_offset = global_data["gpt"]["lba_start_offset"]

    for partition_name in sorted(partition_data.keys(), key=lambda y: (partition_data[y]['id'])):
        partition = get_dict_from(partition_data, partition_name)
        partition_uuid = "%s%s" % (global_data["gpt"]["uuid_prefix"], partition["uuid"])
        #If concatenation is over 36 chars, ignore prefix
        if len(partition_uuid) > 36 :
            partition_uuid = "%s" % (partition["uuid"])
        partition_size_hint = partition["length"]


        if partition["label"] is None:
            partition_label = partition_name
        else:
            partition_label = partition["label"]

        if partition["size"] is None:
            pass
        else:
            if (partition["size"] <= 0):
                partition_size = "$calc($lba_end%s)" % (partition_size_hint)
            else:
                partition_size = ( (int(partition["size"])*1024*1024) - partition_size_hint ) / storage["sector_size"]

        if partition["lba_start"] != None:
            lba_start_offset = partition["lba_start"]

        gpt_commands.append("add -b %d -s %s -t %s -u %s -l %s -T %d -P %d %s" % (
            lba_start_offset,
            str(partition_size),
            partition["type"],
            partition_uuid,
            partition_label,
            partition["try"],
            partition["priority"],
            storage["base_name"]))

        if isinstance( partition_size, int ):
            lba_start_offset += partition_size

    gpt_commands.append("reload %s" % (storage["base_name"]))

    return gpt_commands

"""
    Name    :   generate_mbr_recovery_fstab_file
    Role    :   Generate a "MBR" partition file for fastboot OS
                File part of the fastboot OS, used to erase & partition
    Input   :
                storage: dictionary with storage parameters
                      (storage key in JSON)
                global_data: dictionary with global parameters
                             (global key in JSON)
    Output  :   None
    Return  :   tuple: Recovery fstab lines (list),
                       and associated indentation dictionary
"""
def generate_mbr_recovery_fstab_file(storage, global_data):

    fstab_lines = list()
    fill = dict()

    for partition_name in sorted(partition_data.keys(), key=lambda y: (partition_data[y]['id'])):
        partition = get_dict_from(partition_data, partition_name)
        current_line = list()
        partition_id = partition["id"]

        if partition["label"] is None:
            display_partition_name = partition_name
        else:
            display_partition_name = partition["label"]

        current_line.append("#size_hint=%d" % (partition["size"]))
        current_line.append("\n")
        current_line.append(global_data["gpt"]["base_name_name"] + display_partition_name)
        current_line.append("\t")
        current_line.append("/%s" % (partition_name))
        current_line.append("\t")
        current_line.append("%s" % (partition["fs_type"]))
        current_line.append("\t")
        current_line.append("%s" % (partition["mount_options"]))
        current_line.append("\t")
        current_line.append("length=%d" % (partition["length"]))
        current_line.append("\n")
        update_fill_list(current_line, fill)
        fstab_lines.append(current_line)

    if (global_data["sdcard"]["used"]) == 1:
        fstab_lines.append(global_data["sdcard"]["recovery_entry"])
        update_fill_list(current_line, fill)

    return fstab_lines, fill

"""
    Name    :   fstab_indent
    Role    :   Generates an indented fstab files
    Input   :
                output_file: name of the fstab file to generate
                lines: list of fstab lines
                fill: dict with size of each word (key dict is word number for)
    Output  :   None
    Return  :   tuple: Recovery fstab lines (list),
                and associated indentation dictionary
"""
def fstab_indent(output_file, lines, fill):

    f = file(output_file, "w")

    for line in range(0, len(lines)):
        normalized_line = str()
        for word in range(0, len(lines[line])):
            normalized_line += str(lines[line][word].ljust(fill[word]))

        if (line == len(lines)):
            f.write(normalized_line[:-1])
        else:
            f.write(normalized_line)

    f.close()

"""
    Name    :   json_load
    Role    :   Open a JSON file and returns JSON content
    Input   :
                json_file: name of the JSON file to load
    Output  :   None
    Return  :   JSON content
"""
def json_load(json_file):

    stream = open(json_file, 'r').read()
    json_stream = json.loads(stream)
    return json_stream


"""
    Name    :   json_load_override
    Role    :   Open two JSON file (base and override) and returns JSON content
    Input   :
                base_file: name of the JSON base file to load
                override_file: name of the JSON override file to load
    Output  :   None
    Return  :   JSON content
"""
def json_load_override(base_file, override_file):

    base = json_load(base_file)

    if os.path.isfile(override_file):
        try:
            override = json_load(override_file)
            base=merge_dict(base,override)
        except ValueError, e:
            print "%s file malformed. No overriding (%s)" % (override_file, str(e))
            # malformed file : fatal error
            sys.exit(-1)
    else:
        print "%s file does not exist. No overriding" % (override_file)

    return base

"""
    Main script
"""
if len(sys.argv) < 6:
    print "Usage: %s partition_base filesystem_base override_file out_directory product_model" % (sys.argv[0])
else:
    try:
        if len(sys.argv) >= 6:
            storage_base_file = sys.argv[1]
            filesystem_base_file = sys.argv[2]
            override_file = sys.argv[3]
            out_path = sys.argv[4]
            product_model = sys.argv[5]

            storage_json = json_load(storage_base_file)
            filesystem_json = json_load(filesystem_base_file)

            storage_json = json_load_override(storage_base_file, override_file)
            filesystem_json = json_load_override(filesystem_base_file, override_file)

        global_data = get_dict_from(storage_json, "globals")
        storage_data = get_dict_from(storage_json, "storage")
        header_data = get_dict_from(storage_json, "header")
        partition_data = get_dict_from(storage_json, "partitions")
        os_data = get_dict_from(filesystem_json, "oses")

        if (os.getenv("PART_MOUNT_OUT_FILE")):
            target = os.getenv("PART_MOUNT_OUT_FILE")

        if (os.getenv("STORAGE_BASE_NAME")):
            storage_data["base_name"] = os.getenv("STORAGE_BASE_NAME")

        if (target.endswith("partition.tbl")):

            """
                Generate partition table for fastboot OS
            """
            out_file = "%s/%s" % (out_path, "partition.tbl")
            print out_file
            partition_table_file = file(out_file, "w")
            partition_table_file.write("partition_table=%s\n" % global_data["format_table"])

            if global_data["format_table"] == "gpt":
                gpt_commands = generate_gpt_partition_file(storage_data, global_data)
                for line in gpt_commands:
                    partition_table_file.write(line+"\n")

            partition_table_file.close()
            sys.exit()



        """
            Generate fstab file for each OS
        """
        for k_os, v_os in os_data.iteritems():
            print "Generating files for %s (%s)" % (k_os, global_data["format_table"])

            v_os["fstab_file"]=v_os["fstab_file"].replace("$model", product_model)

            if v_os["recovery_fstab_dest_file"] is not None:
                out_file = "%s/%s" % (out_path, v_os["recovery_fstab_dest_file"])
                if out_file.endswith(target):
                    print "\t%s" % (out_file)
                    recovery_lines, fill = generate_mbr_recovery_fstab_file(storage_data, global_data)
                    fstab_indent(out_file, recovery_lines, fill)
                    sys.exit()

            if v_os["fstab_file"] is not None:
                out_file = "%s/%s" % (out_path, v_os["fstab_file"])
                print "\t%s" % (out_file)

                if out_file.endswith(target):
                    if global_data["format_table"] == "mbr":
                        fstab_lines, fill = generate_mbr_fstab_file(storage_data, global_data, v_os)
                        fstab_indent(out_file, fstab_lines, fill)

                    if global_data["format_table"] == "gpt":
                       fstab_lines, fill = generate_gpt_fstab_file(storage_data, global_data, v_os)
                       fstab_indent("%s/%s" % (out_path, v_os["fstab_file"]), fstab_lines, fill)

                    sys.exit()
    except KeyError, e:
        print "Key %s does not exist (overriding file malformed)" % str(e)
