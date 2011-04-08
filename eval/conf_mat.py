#!/usr/bin/env python2.7
"""Confusion matrix"""

import sys
import csv

from collections import defaultdict

try:
    ref = open(sys.argv[1])
    ans = open(sys.argv[2])
except:
    print 'Usage: {} ref ans > out.csv'.format(sys.argv[0])
    exit(1)

ref_vals = [i.split()[0] for i in ref if i.strip()]
ans_vals = [i.split()[0] for i in ans if i.strip()]

assert len(ref_vals) == len(ans_vals)

keys = sorted(set(ref_vals).union(ans_vals))

counts = defaultdict(int)
for (r, a) in zip(ref_vals, ans_vals):
    counts[r,a] += 1

writer = csv.writer(sys.stdout)

writer.writerow([''] + keys + [''])

for r in keys:
    row = [counts[r,a] for a in keys]
    writer.writerow([r] + [i if i else '' for i in row] + [sum(row)])

writer.writerow([''] + [sum([counts[r,a] for r in keys]) for a in keys] + [''])
