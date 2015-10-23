#!/usr/bin/env python
from __future__ import print_function
import os
import collections
import tokenize
import argparse


def nocomments(fpin):
    result = []
    g = tokenize.generate_tokens(fpin.readline)
    for toknum, tokval, _, _, _ in g:
        if toknum != tokenize.COMMENT:
            result.append((toknum, tokval))
    return tokenize.untokenize(result)


def getcc(in_file_name):
    with open(in_file_name, 'r') as fpin:
        if cmd_line.strip:
            code = nocomments(fpin)
        else:
            code = fpin.read()
        for cc in code:
            yield cc


def decode(array, asize):
    out_str = ''
    for ii in range(asize):
        byte = ii // 4
        nibble = ii % 4
        out_str += chr((array[byte] >> (nibble * 8)) & 0xFF)
    return out_str


fmodule = collections.namedtuple('fmodule', 'name values size strdata')


def str_to_array(data, name=None):
    values = []
    for bcount, cc in enumerate(data):
        nibble = bcount % 4
        if not nibble:
            values.append(0)
        values[-1] += ord(cc) << (nibble * 8)
    return fmodule(name, values, bcount + 1, data)        # always add 1 because enumerate 0 1 2 3 is size 4


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='convert text for frozen')
    parser.add_argument("-d", "--dir-name", dest="dir_name", help="directory name")
    parser.add_argument("-o", "--out-file", dest="out_file", help="out file name")
    parser.add_argument("-s", "--strip", dest="strip", action='store_true', help="dump strip comments")
#   '--rname', nargs=argparse.REMAINDER)
    cmd_line = parser.parse_args()

    module_names = list()
    for dirpath, dirnames, filenames in os.walk(cmd_line.dir_name):
        for f in filenames:
            fullpath = os.path.join(dirpath, f)
            module_names.append(os.path.basename(fullpath))

    modules = dict()
    for fname in module_names:
        data = ''.join(getcc(os.path.join(cmd_line.dir_name, fname)))
        # data = data.replace('"', '\\"')
        modules[fname] = str_to_array(data, os.path.splitext(fname)[0])

    def roundup(num, div):
        return num + ((div - (num % div)) % div)

    with open(cmd_line.out_file, 'w') as of:
        of.write("#include <stdint.h>\n")
        of.write('#include "esp_frozen.h"\n\n')
        of.write("const uint16_t mp_frozen_table_size = %d;\n\n" % len(modules))
        of.write("const esp_frozen_t mp_frozen_table[] = {\n")
        position = 0
        for module in modules.itervalues():
            of.write('\t{"%s", %d, %d},\n' % (module.name, position, module.size))
            position += len(module.values)
        of.write('};\n\n')

        of.write("const uint32_t mp_frozen_qwords[] = {\n")
        of.write(', '.join([str(ii) for mod in modules.values() for ii in mod.values]))
        of.write("};\n")
