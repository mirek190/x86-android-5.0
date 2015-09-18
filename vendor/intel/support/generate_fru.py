#!/usr/bin/env python

import sys
import json
import os.path
import collections
import argparse

"""
    Description:
        Purpose of this script is to generate a config xml file containing the
        FRUs of golden configs
"""


def get_dict_from(json_array, k):
    if json_array.has_key(k):
        return json_array[k]
    else:
        return dict()

def json_load(json_file):

    stream = open(json_file, 'r').read()
    json_stream = json.loads(stream)
    return json_stream

def calculate_fru(config, aobs):

    aob_value = get_dict_from(config, "aobs")
    fru_value = ['0'] * 20
    for k,v in aob_value.items():
        id = int(aobs[k]["id"], 16)
        aob_id = aobs[k]["module"][v]["id"][2]
        fru_value[id] = aob_id
    return ("".join(fru_value))

def generate_token(fru, template, token_dir, golden):
    token_name = os.path.join(token_dir, "fru_token_%s.tmt" % golden)
    fd = open(token_name, 'w')
    fd.write(template.replace("data_value=\"\"", "data_value=\"(h)%s\"" % fru))
    fd.close()

def generate_tokens(token_template, token_dir):
    global configs, aob

    template_file = open(token_template, 'r')
    template_str = str(template_file.read())
    template_file.close()
    try:
        for k,v in sorted(configs.items()):
            fru_str = calculate_fru(v, aobs)
            generate_token(fru_str, template_str, token_dir, k)

    except KeyError, e:
        print "Key %s does not exist (overriding file malformed)" % str(e)
        sys.exit(-1)

def generate_config():
    global configs, aobs

    pft_xml_output = "<fru_configs>"
    try:
        for k,v in sorted(configs.items()):
            fru_str = calculate_fru(v, aobs)
            pft_xml_output += "\n<fru_config>\n<name>%s</name>\n<value>%s</value>\n<description>%s</description>\n</fru_config>" % (k, fru_str, k)

    except KeyError, e:
        print "Key %s does not exist (overriding file malformed)" % str(e)
        sys.exit(-1)

    pft_xml_output += "\n</fru_configs>\n"
    goldens_file = open(args.configs_xml, 'w')
    goldens_file.write(pft_xml_output)
    goldens_file.close()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generate fru configurations file.')
    parser.add_argument('aobs_filename', metavar='aobs.json', help='The json file describing AOBs')
    parser.add_argument('configs_filename', metavar='configs.json', help='The json file describing the predefined configurations')
    parser.add_argument('configs_xml', metavar='configs_fru.xml', help='If platform uses fastboot method, the output file that will receive the list of predefined fru configurations')
    parser.add_argument('fru_token_dir', metavar='FRU_TOKEN_DIR', help='If platform uses token method, the output directory that will receive the list of fru token configurations')
    parser.add_argument('fru_token_template_tmt', metavar='fru_token_template.tmt', help="""
        If platform uses token method, the fru token template from TMT that will be used to generate each fru token configurations.
        There will be as much output files as predefined configurations in json.
        They will be named fru_token_XXX.tmt where XXX defines the variant.
        They will be located in FRU_TOKEN_DIR.
        """)

    args = parser.parse_args()

    json_aobs = json_load(args.aobs_filename)
    json_configs = json_load(args.configs_filename)

    aobs = get_dict_from(json_aobs, "aobs")
    configs = get_dict_from(json_configs, "configs")

    if args.fru_token_template_tmt != "":
        generate_tokens(args.fru_token_template_tmt, args.fru_token_dir)
    else:
        generate_config()

