#!/usr/bin/env python2.7
"""Baseline features: bag of words witin a small window and current speaker"""

import sys
import cPickle
import re

import feat_writer
import common

# must use integers as labels since megam requires it!
LABEL_ID = common.IncrCounter()
PADDING = [(None, None, None, [])]
COLLOCS = []
MAX_COLLOCS_LEN = 0


def get_collocs(unit):
    global COLLOCS
    global MAX_COLLOCS_LEN

    r = set()
    for i in range(3, MAX_COLLOCS_LEN + 1):
        for j in common.ngrams(unit, i):
            cll = '_'.join(j)
            if cll in COLLOCS:
                r.add(cll)
    return r


def iter_features(docs, window_back=0, window_forward=0):
    """A generator of features

    `docs`: [(dyad, role, code, unit)]
    `window_back`: how far to look back
    `window_forward`: how far to look forward
    """
    global LABEL_ID
    global PADDING
    global COLLOCS
    global MAX_COLLOCS_LEN

    for doc in docs:
        if type(doc) is not list:
            doc = list(doc)
        # add dummy items and tokenize units
        doc = PADDING * window_back + \
              [(dyad, role, code, re.sub('_', '', unit).split()) for (dyad, role, code, unit) in doc] + \
              PADDING * window_forward
        feat_doc = []
        for i in range(window_back, len(doc) - window_forward):
            _, role, code, unit = doc[i]
            # integer label; make megam happy
            label = LABEL_ID(code)
            # add features
            feats = set()
            feats.update(('CUR_COLLOC_' + w for w in get_collocs(unit)))
            for j in range(1, window_back+1):
                prefix = 'BACK_{}_COLLOC_'.format(j)
                _, _, _, unit = doc[i-j]
                feats.update((prefix + w for w in get_collocs(unit)))
            for j in range(1, window_forward+1):
                prefix = 'FORWARD_{}_COLLOC_'.format(j)
                _, _, _, unit = doc[i+j]
                feats.update((prefix + w for w in get_collocs(unit)))
            feat_doc.append((label, sorted(feats)))
        yield feat_doc


if __name__ == '__main__':
    try:
        which = sys.argv[1]
        writer = {'megam': feat_writer.megam_writer,
                  'crfsuite': feat_writer.crfsuite_writer}[which]
        out_dir = sys.argv[2]
        train_in, dev_in, test_in = sys.argv[3:6]

        colloc = sys.argv[6]
    except:
        print 'Usage: {} which(=megam|crfsuite) out_dir train dev test colloc'.format(sys.argv[0])
        exit(1)

    with open(colloc) as f:
        COLLOCS = set(f.read().split())
        MAX_COLLOCS_LEN = max([len(i.split('_')) for i in COLLOCS])

    for (purpose, path) in zip(["train", "dev", "test"], [train_in, dev_in, test_in]):
        with open(path) as fi:
            with open(out_dir + '/' + purpose + '.' + which, 'w') as fo:
                writer(iter_features(common.lazy_load_dyads(fi)),
                       fo)

    with open(out_dir + '/' + 'map.' + which, 'w') as f:
        cPickle.dump(LABEL_ID, f)

