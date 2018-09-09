#!/usr/bin/python

import sys
import struct
import argparse

BL_MODULES_LIST_MAGIC = 0x54534c444f4d4c42 # "BLMODLST"
BL_MODULE_MAGIC = 0x000000444f4d4c42 # "BLMOD"

def args_parse():
    parser = argparse.ArgumentParser(description='Boot loader modules')

    parser.add_argument('-b', '--boot-loader', type=argparse.FileType('ab'),
                        help='Boot loader to store in the given modules')

    parser.add_argument('-m', '--modules-file', type=argparse.FileType('wb'),
                        help='The file to write in the list of modules')

    parser.add_argument('-l', '--list', nargs='+', type=argparse.FileType('rb'),
                        help='List of the boot loader modules', required=True)

    return parser.parse_args()

def modules_list(mods):
    l = ''
    total_size = 0

    for m in mods:
        buf = m.read()
        total_size += len(buf)
        l += struct.pack('<QI', BL_MODULE_MAGIC, len(buf)) + buf

    return struct.pack('<QII', BL_MODULES_LIST_MAGIC, len(mods), total_size) + l

def main():
    args = args_parse()

    if args.boot_loader is None and args.modules_file is None:
        sys.exit('Error: No output file specified')

    l = modules_list(args.list)

    if args.boot_loader:
       args.boot_loader.write(l)
 
    if args.modules_file:
        args.modules_file.write(l)

if __name__ == '__main__':
    main()

