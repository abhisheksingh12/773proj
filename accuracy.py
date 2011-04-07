#!/usr/bin/env python2.7

import sys

try:
    ref = open(sys.argv[1])
    ans = open(sys.argv[2])
except:
    print 'Usage: {} ref ans'.format(sys.argv[0])
    exit(1)

ref_vals = [i.split()[0] for i in ref if i.strip()]
ans_vals = [i.split()[0] for i in ans if i.strip()]

assert len(ref_vals) == len(ans_vals)

correct = sum([i == j for (i, j) in zip(ref_vals, ans_vals)])
print '{} / {} = {:.2f}%'.format(correct, len(ref_vals), correct * 100. / len(ref_vals))
