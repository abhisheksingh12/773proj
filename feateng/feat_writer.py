"""Feature writers for various tools. All writers share the same
parameter interface: (docs, fo).

`docs` is a list of documents where each document is represented by a
list of (label, feats) pair. `label` must be an integer; values in
`feats` will be output as a string with all whitespaces replaces with
_.

`fo` is the output file object.
"""

__ALL__ = ['megam_writer', 'crfsuite_writer']

import re

def replace_white_space(s):
    return re.sub(r'\s', '_', s)

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
                     '\t'.join(('F_' + replace_white_space(str(i)) for i in feats)) +
                     '\n')
        # mark document boundary
        fo.write('\n')
