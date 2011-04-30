#!/usr/bin/env python2.7

"""Merge feature files"""

import sys
from itertools import izip

from common import feat_of_line


def merge_feats(feats):
    labels = set([l for l,_ in feats])
    if len(labels) != 1:
        print labels
        assert False
    label = list(labels)[0]
    feat = list(reduce(set.union, [f for _,f in feats], set()))
    return label, feat


if __name__ == '__main__':
    try:
        output_path = sys.argv[1]
        input_paths = sys.argv[2:]
    except:
        print >> sys.stderr, "Usage: {} output_path input_path [input_paths ...]".format(sys.argv[0])
        raise

    fo = open(output_path, 'w')
    fis = map(open, input_paths)

    for lines in izip(*fis):
        label, feat = merge_feats(map(feat_of_line, lines))
        fo.write('{}\t{}\n'.format(
            label,
            ' '.join(feat)))

    for f in fis:
        f.close()
    fo.close()
