#!/usr/bin/env python3

'''
A file containing a number of functions used to autogenerate gatt files from
xml files.
'''

import argparse
import xmltodict
import os
import json
import jinja2
from datetime import datetime
from utility.general_repo_tools import get_repo_root
from textwrap import wrap


def gatt_xml_to_dict(app_path):
    # Check path exists
    xml_path = os.path.join(app_path, "src", "gatt.xml")
    if not os.path.exists(xml_path):
        raise FileExistsError("Can not find xml file.")

    with open(xml_path) as fd:
        xml_string = fd.read()

    # Recursively change all ordered dicts to dicts
    ordered_dict = xmltodict.parse(xml_string)
    regular_dict = json.loads(json.dumps(ordered_dict))

    # If the service field, characteristic field, or descriptor field contains
    # a single entry, it returns the dict, rather than a list of dicts. This
    # is bad and breaks jinja, so if there are single characteristics, just
    # stuff them into a single list.
    if type(regular_dict['gatt']['service']) is not list:
        regular_dict['gatt']['service'] = [regular_dict['gatt']['service']]
    for service in regular_dict['gatt']['service']:
        if type(service['characteristic']) is not list:
            service['characteristic'] = [service['characteristic']]

    return regular_dict


def profile_declaration(gatt_dict):
    """
    Prints a comma seperated lists of characteristics and services for
    declaration.
    """
    profile_variable_list = []
    # print(gatt_dict)
    for service in (x for x in gatt_dict.get('service')):
        profile_variable_list.append("&gattdb_" + service['@id'])
        for characteristic in (x for x in service.get('characteristic')):
            profile_variable_list.append("&gattdb_" + characteristic['@id'])

    profile_variable_list.append("NULL")
    return "\t" + ",\r\n\t".join(profile_variable_list)


def filter_uuid_and_swap_bytes(uuid_string):
    """
    A function we are going to pass into Jinja2 as a custom filter to allow us
    to easily format long UUID's
    """
    return ", ".join(["0x"+x for x in wrap(uuid_string.replace("-", ""), 2)[::-1]])


def filter_characteristic_properties(characteristic_dict):
    """
    Takes the dict from a charactistic, and spits out a string of all the
    properly formatted properties of that characteristic.
    """
    property_name_mapping = {"@indicate": "indicate",
                             "@notify": "notify",
                             "@read": "read",
                             "@reliable_write": "reliable_wr",
                             "@write": "write",
                             "@write_no_response": "write_wo_resp"}

    struct_mapping = {"@indicate": "char_props",
                      "@notify": "char_props",
                      "@read": "char_props",
                      "@reliable_write": "char_ext_props",
                      "@write": "char_props",
                      "@write_no_response": "char_props"}
    ret_str = ""

    first = True
    for key in property_name_mapping:

        # This makes the formatting look good.
        if first:
            first = False
        else:
            ret_str += "\r\n\t"

        # Adds the characteristics into a string.
        if (key in characteristic_dict["properties"]) and (characteristic_dict["properties"][key] == 'true'):
            ret_str += "xCharacteristicSettings.{}.{} = true;".format(struct_mapping[key], property_name_mapping[key])
        else:
            ret_str += "xCharacteristicSettings.{}.{} = false;".format(struct_mapping[key], property_name_mapping[key])

    return ret_str


def filter_characteristic_vlen(characteristic_dict):
    ret_str = ""

    try:
        ret_str = "xCharMetadataAttr.vlen = {};".format(characteristic_dict['value']['@variable_length'].lower())
    except:
        ret_str = "xCharMetadataAttr.vlen = false;"

    return ret_str


def filter_characteristic_value(characteristic_dict):
    ret_str = ""

    # Add the length to the string
    if 'value' not in characteristic_dict:
        Exception("Characteristics must have a value attribute")
    if '@length' in characteristic_dict['value']:
        ret_str += "xCharValue.max_len = {};".format(characteristic_dict['value']['@length'])
    else:
        ret_str += "xCharValue.max_len = 0;"

    # Add the value to the string
    ret_str += "\r\n\t"

    if '#text' in characteristic_dict['value']:
        ret_str += "xCharValue.p_value = (uint8_t *) \"{}\";".format(characteristic_dict['value']['#text'])
    else:
        ret_str += "xCharValue.p_value = NULL;"

    return ret_str


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Autogenerates a gatt.c and gatt.h file given a gatt.xml which describes the characteristics.')
    parser.add_argument('--app_path', '-a', dest='app_path', required=True, type=str, help='The path to the application for which we are generating the gatt.c/.h files for, and contains a gatt.xml')
    parser.add_argument('--output_path_h', '-o_h', dest='out_path_h', required=True, type=str, help='Path to where we are going to output the h file.')
    parser.add_argument('--output_path_c', '-o_c', dest='out_path_c', required=True, type=str, help='Path to where we are going to output the c file.')

    args = parser.parse_args()

    # Load Jinja2 environment
    template_loader = jinja2.FileSystemLoader(searchpath=os.path.join(get_repo_root(), "pyclasses", "templates", "gatt"))
    template_env = jinja2.Environment(loader=template_loader, trim_blocks=True, lstrip_blocks=True, keep_trailing_newline=True)
    template_env.filters['parse_long_uuid'] = filter_uuid_and_swap_bytes
    template_env.filters['filter_characteristic_properties'] = filter_characteristic_properties
    template_env.filters['filter_characteristic_vlen'] = filter_characteristic_vlen
    template_env.filters['filter_characteristic_value'] = filter_characteristic_value
    template_env.filters['profile_declaration'] = profile_declaration

    # Generate dictionary from XML
    xml_dict = gatt_xml_to_dict(args.app_path)

    template_env.get_template('template_gatt.c').stream(date=datetime.now().strftime("%m/%d/%Y, %H:%M:%S"), gatt_dict=xml_dict).dump(os.path.join(args.out_path_c, 'gatt_nrf52.c'))
    template_env.get_template('template_gatt.h').stream(date=datetime.now().strftime("%m/%d/%Y, %H:%M:%S"), gatt_dict=xml_dict).dump(os.path.join(args.out_path_h, 'gatt_nrf52.h'))
