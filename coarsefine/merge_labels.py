#!/usr/bin/env python2.7

"""Merge labels for megam"""

import sys
import re
import cPickle


def load_group(fi, label_map):
    """
    Load grouping information from `fi`.

    `fi`: a grouping info file; groups are separated by an empty line
    (i.e. two \\n)

    `label_map`: a function-like label to integer mapping; usually an
    common.IncrCounter

    return: a dictionary from class to its group, and a list of group
    ids.
    """
    d = {}
    gids = []
    for (gid, members) in enumerate(fi.read().strip().split('\n\n')):
        for (l, i) in enumerate(map(label_map, members.split())):
            d[i] = (gid, l)
        gids.append(gid)
    return d, gids


def merge_labels(lines, groups, fo, go):
    """
    Merge labels according to `groups`.

    `lines`: an iterable of raw lines.

    `groups`: a mapping from class to its group and its in group
    label; both must be an integer; usually obtained
    from load_group().

    `fo`: output file of merged lines.

    `go`: a mapping of group-wise output files.
    """
    for line in lines:
        label, feat_str = line.split('\t', 1)
        label = int(label)
        group, new_label = groups[label]
        # write to group-wise data
        go[group].write(str(new_label) + '\t' + feat_str)
        # write label-merged line
        fo.write(str(group) + '\t' + feat_str)


if __name__ == '__main__':
    try:
        gi = open(sys.argv[1], 'rU')
        go_prefix = sys.argv[2]
        if len(sys.argv) > 3:
            with open(sys.argv[3]) as f:
                label_map = cPickle.load(f)
        else:
            label_map = int
    except:
        print >> sys.stderr, 'Usage: {} gi go_prefix [label_map] < data > merged'.format(sys.argv[0])
        raise

    groups, gids = load_group(gi, label_map)
    go = dict([(gid, open('{}.{}'.format(go_prefix, gid), 'w'))
               for gid in gids])

    merge_labels(sys.stdin, groups, sys.stdout, go)

    for f in go.values(): f.close()
