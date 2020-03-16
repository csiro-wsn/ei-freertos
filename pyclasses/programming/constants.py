#!/usr/bin/env python3

"""
generate_constants.py

Binary file generator for simple structs defined in a c header file.
  - Generator assume that the struct has __attribute__(( packed ))
  - Parameter validation only checks that length is not too long, not
    that there is enough data to completely fill a variable
"""
__author__ = "Jordan Yates"

import binascii

from pycparser import c_ast, parse_file

type_lengths = {
    'int8_t': 1,
    'uint8_t': 1,
    'int16_t': 2,
    'uint16_t': 2,
    'int32_t': 4,
    'uint32_t': 4,
}

quiet = True


def get_struct_fields(header_file, struct_name):
    # Use pycparser to parse a non-nested struct declaration in a header file
    ast = parse_file(header_file)
    struct_fields = []
    # For all top level declarations
    for ext in ast.ext:
        # We only care about declared structs
        if not isinstance(ext, c_ast.Typedef):
            continue
        if not isinstance(ext.type, c_ast.TypeDecl):
            continue
        if not isinstance(ext.type.type, c_ast.Struct):
            continue
        struct = ext.type.type
        # Check if the declaration is the one we are interested in
        if struct.name == struct_name:
            # Loop through all the fields in our struct
            for field in struct.decls:
                # Get the field name
                field_name = field.name
                # If the field is an array
                if isinstance(field.type, c_ast.ArrayDecl):
                    # Get the base type and array lengths
                    field_type = field.type.type.type.names[0]
                    field_length = int(field.type.dim.value)
                # Otherwise assume that it is a normal basic variable
                else:
                    field_type = field.type.type.names[0]
                    field_length = 1
                # Convert the type string to a byte size
                type_len = type_lengths[field_type]
                # Append our knowledge of the field to our dictionary
                f = {'name': field_name, 'type': field_type,
                     'type_length': type_len, 'length': field_length}
                struct_fields.append(f)
    return struct_fields


def construct_bytearrays(fields, values):
    # For every field in the struct
    for f in fields:
        # If a value was provided, store that
        if f['name'] in values:
            f['value'] = int(values[f['name']], 0)
        # Otherwise set it to 0
        else:
            f['value'] = 0
        # Store the size of the field in bytes
        byte_length = f['type_length'] * f['length']
        # Convert the field value to a bytearray
        f['bytes'] = bytearray(f['value'].to_bytes(byte_length, byteorder='little'))
        # Print out the binary representation of the struct field we will save to the file
        if not quiet:
            print("{:s}: {:s}".format(f['name'], str(binascii.hexlify(f['bytes']))))


def construct_constant_blob(fields, values):
    # Push our specified values into the field information
    construct_bytearrays(fields, values)
    # Push all the fields into an output bytearray
    output_bytes = bytearray()
    for f in fields:
        output_bytes.extend(f['bytes'])
    return output_bytes


def generate_binary(filename, fields, values):
    output_bytes = construct_constant_blob(fields, values)
    # Write our binary blob to a file
    with open(filename, 'wb') as f_out:
        f_out.write(output_bytes)


def generate_constants(header, struct, output, values):
    # Get Struct Fields
    fields = get_struct_fields(header, struct)
    # Generate the binary
    generate_binary(output, fields, values)
