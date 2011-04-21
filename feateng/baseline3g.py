#!/usr/bin/env python2.7
"""Baseline features: bag of n-grams (n<=3) and current speaker"""

import sys
import cPickle

import feat_writer
import common

# must use integers as labels since megam requires it!
LABEL_ID = common.IncrCounter()
PADDING = [(None, None, None, [])]

def iter_features(docs):
    """A generator of features

    `docs`: [(dyad, role, code, unit)]
    """
    global LABEL_ID
    global PADDING
    for doc in docs:
        if type(doc) is not list:
            doc = list(doc)
        # tokenize units
        doc = [(dyad, role, code, unit.split()) for (dyad, role, code, unit) in doc]
        feat_doc = []
        for i in range(len(doc)):
            _, role, code, unit = doc[i]
            # integer label; make megam happy
            label = LABEL_ID(code)
            # add features
            feats = set()
            feats.add('SPEAKER_' + role)
            feats.update(('CUR1_' + w for w in unit))
            sb_unit = ['<s>'] + unit + ['</s>']
            feats.update(('CUR2_{}/{}'.format(*w) for w in zip(sb_unit, sb_unit[1:])))
            sb_unit = ['<s>'] + sb_unit
            feats.update(('CUR3_{}/{}/{}'.format(*w) for w in zip(sb_unit, sb_unit[1:], sb_unit[2:])))
            feat_doc.append((label, sorted(feats)))
        yield feat_doc


if __name__ == '__main__':
    try:
        which = sys.argv[1]
        writer = {'megam': feat_writer.megam_writer,
                  'crfsuite': feat_writer.crfsuite_writer}[which]
        out_dir = sys.argv[2]
        train_in, dev_in, test_in = sys.argv[3:6]

    except:
        print 'Usage: {} which(=megam|crfsuite) out_dir train dev test'.format(sys.argv[0])
        exit(1)

    for (purpose, path) in zip(["train", "dev", "test"], [train_in, dev_in, test_in]):
        with open(path) as fi:
            with open(out_dir + '/' + purpose + '.' + which, 'w') as fo:
                writer(iter_features(common.lazy_load_dyads(fi)),
                       fo)

    with open(out_dir + '/' + 'map.' + which, 'w') as f:
        cPickle.dump(LABEL_ID, f)

