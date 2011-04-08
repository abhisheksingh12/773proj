#!/usr/bin/env python2.7
"""Converts number labels back to textual codes"""

import cPickle
import sys

try:
    sys.argv[1]
except:
    print >> sys.stderr, "Usage: {} label_map < in > out".format(sys.argv[0])
    exit(1)

with open(sys.argv[1]) as f:
    label_map = cPickle.load(f)

for line in sys.stdin:
    line = line.strip()
    if not line:
        sys.stdout.write('\n')
        continue
    num = int(line.split()[0])
    sys.stdout.write(str(label_map[num]))
    sys.stdout.write('\n')
