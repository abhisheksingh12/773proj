#!/usr/bin/env python2.7

import sys

from collections import defaultdict

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
print 'Overall: {} / {} = {:.2f}%'.format(correct, len(ref_vals), correct * 100. / len(ref_vals))

per_tag_corret = defaultdict(int)
per_tag_total = defaultdict(int)

for (r, a) in zip(ref_vals, ans_vals):
    per_tag_total[r] += 1
    per_tag_corret[r] += r == a

for t in sorted(set(ref_vals).union(ans_vals)):
    print '{}: {} / {} = {:.2f}%'.format(t, per_tag_corret[t], per_tag_total[t],
                                         per_tag_corret[t] * 100. / per_tag_total[t])
