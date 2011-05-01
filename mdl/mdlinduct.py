#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""

"""


from sys import argv
from mdl import induct_cml2
from no_triple import has_triple

ifile = file(argv[1], "rU")
opref = argv[2]

data = [i for i in ifile.read().split() if i]

if has_triple(data):
    print "data has triples"
else:
    induct_cml2(data, 5, set(["."]), set([".", "a", "an", "the", "that"]), output=opref)
