#!/usr/bin/env python2.7

import sys

from common import *

to_drop = set(sys.argv[1:])

write_dyads(((dyad, role, code, unit)
             for one_dyad in lazy_load_dyads(sys.stdin)
             for (dyad, role, code, unit) in one_dyad
             if dyad not in to_drop),
            sys.stdout)
