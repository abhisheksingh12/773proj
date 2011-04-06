#!/usr/bin/env python2.7

import sys

from common import *

out_dir = sys.argv[1]

for one_dyad in lazy_load_dyads(sys.stdin):
    dyad, _, _, _ = one_dyad[0]
    with open(out_dir + '/' + dyad, 'w') as f:
        write_dyads(one_dyad, f)
