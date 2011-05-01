#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""

"""

from sys import argv

def has_triple(data):
    for i in xrange(len(data)):
        if i + 2 < len(data) and data[i] == data[i + 1] and data[i] == data[i + 2]:
            return True
    return False

if __name__ == "__main__":
    for path in argv[1:]:
        print path
        data = [i for i in file(path, "rU").read().split() if i]
        for i in xrange(len(data)):
            if i + 2 < len(data) and data[i] == data[i + 1] and data[i] == data[i + 2]:
                print "triple at %d" % i
        print ""
