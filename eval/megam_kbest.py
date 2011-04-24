#!/usr/bin/env python2.7

"""Take megam output and convert it to k best listing"""

import sys



def process_line(line):
    sp = line.split()
    best = int(sp[0])
    scores = map(float, sp[1:])
    ranked = sorted(range(len(sp)-1), key=lambda x: scores[x], reverse=True)
    assert ranked[0] == best
    return ranked


if __name__ == '__main__':
    for line in sys.stdin:
        print '\t'.join(map(str, process_line(line)))
