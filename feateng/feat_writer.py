"""Feature writers for various tools. All writers share the same
parameter interface: (docs, fo).

`docs` is an iterable of documents where each document is represented
by an iterable of (label, feats) pair. `label` must be an integer;
values in `feats` will be output as a string with all whitespaces
replaces with _.

`fo` is the output file object.
"""

__ALL__ = ['megam_writer', 'crfsuite_writer']

import re

def replace_white_space(s):
    return re.sub(r'\s', '_', s)

def escape_colon(s):
    return re.sub(r':', r'\:', re.sub(r'\\', r'\\', s))

def megam_writer(docs, fo):
    for data in docs:
        for (label, feats) in data:
            fo.write(str(label) + '\t' +
                     '\t'.join(('F_' + replace_white_space(str(i)) for i in feats)) +
                     '\n')


def crfsuite_writer(docs, fo):
    for data in docs:
        for (label, feats) in data:
            fo.write(str(label) + '\t' +
                     '\t'.join(('F_' + escape_colon(replace_white_space(str(i))) for i in feats)) +
                     '\n')
        # mark document boundary
        fo.write('\n')
