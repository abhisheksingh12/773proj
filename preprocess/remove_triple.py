#!/usr/bin/env python2.7

import sys
import re

def remove_triples(words):
    ret = []
    N = len(words)
    for i in range(N):
        if i + 2 >= N or words[i] != words[i + 1] or words[i] != words[i + 2]:
            ret.append(words[i])
    return ret


for line in sys.stdin:
    words = re.sub('_', '', line).split()
    sys.stdout.write(' '.join(remove_triples(words)))
    sys.stdout.write('\n')
