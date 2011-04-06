#!/usr/bin/env python2.7

import sys

from common import *

write_dyads(((dyad, role, code, unit.lower())
             for one_dyad in lazy_load_dyads(sys.stdin)
             for (dyad, role, code, unit) in one_dyad),
            sys.stdout)
