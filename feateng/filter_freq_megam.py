#!/usr/bin/env python2.7
"""Filter features that only appeared once"""

import sys
import collections

def feat_of_line(line):
    label, feats = line.split(None, 1)
    return label, feats.split()

try:
    min_freq = int(sys.argv[1])
except:
    print >> sys.stderr, "Usage: {} min_freq < input > output".format(sys.argv[0])
    raise

lines = map(feat_of_line, sys.stdin)
counts = collections.defaultdict(int)
for _, feats in lines:
    for f in feats:
        counts[f] += 1

for label, feats in lines:
    sys.stdout.write('{}\t{}\n'.format(
        label,
        ' '.join([f for f in feats if counts[f] >= min_freq])))
