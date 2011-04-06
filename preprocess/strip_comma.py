#!/usr/bin/env python2.7

import sys

from common import *

def strip_comma(unit):
    unit = unit.strip()
    while unit and unit[0] == ',':
        unit = unit[1:]
    while unit and unit[-1] == ',':
        unit = unit[:-1]
    assert unit
    return unit

write_dyads(((dyad, role, code, strip_comma(unit))
             for one_dyad in lazy_load_dyads(sys.stdin)
             for (dyad, role, code, unit) in one_dyad),
            sys.stdout)
