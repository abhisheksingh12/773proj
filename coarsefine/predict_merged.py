#!/usr/bin/env python2.7
"""Predicting from merged labels"""

import cPickle
import sys

from itertools import izip
from collections import defaultdict


def load_orig(fi, label_map):
    """
    Load original grouping information from `fi`.

    `fi`: a grouping info file; groups are separated by an empty line
    (i.e. two \\n)

    `label_map`: a function-like label to integer mapping; usually an
    common.IncrCounter

    return: a dictionary from subclass back to its original label, and
    the count of original labels
    """
    d = {}
    orig_labels = set()
    for (gid, members) in enumerate(fi.read().strip().split('\n\n')):
        for (l, i) in enumerate(map(label_map, members.split())):
            d[gid,l] = i
            orig_labels.add(i)
    print >> sys.stderr, d, len(orig_labels)
    return d, len(orig_labels)


def predict(merged_prob, group_probs, orig_label, label_count):
    prob = defaultdict(float)
    for (g, p) in enumerate(merged_prob):
        for (l, q) in enumerate(group_probs[g]):
            prob[orig_label[g,l]] += p * q
    pred = max(range(label_count), key=lambda x: prob[x])
    return (pred, [prob[i] for i in range(label_count)])


def restore_predict(fo, fi, gi, orig_label, label_count):
    for (all_line, group_lines) in izip(fi, izip(*gi)):
        merged_prob = map(float, all_line.split()[1:])
        group_probs = [map(float, line.split()[1:]) for line in group_lines]
        pred, probs = predict(merged_prob, group_probs, orig_label, label_count)
        fo.write(str(pred) + '\t' + ' '.join(map(str, probs)) + '\n')


if __name__ == '__main__':
    try:
        with open(sys.argv[1]) as f:
            label_map = cPickle.load(f)
        with open(sys.argv[2]) as f:
            orig_label, label_count = load_orig(f, label_map)
        fi = open(sys.argv[3])
        gi = [open(i) for i in sys.argv[4:]]
    except:
        print >> sys.stderr, 'Usage: {} label_map groups all groups... > output'.format(sys.argv[0])
        raise

    restore_predict(sys.stdout, fi, gi, orig_label, label_count)

    fi.close()
    for f in gi: f.close()
