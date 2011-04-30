#!/usr/bin/env python2.7
"""Baseline features: bag of words witin a small window and current speaker"""

import sys
import cPickle

import feat_writer
import common

# must use integers as labels since megam requires it!
LABEL_ID = common.IncrCounter()
PADDING = [(None, None, None, [])]

def iter_features(docs, window_back=1, window_forward=1):
    """A generator of features

    `docs`: [(dyad, role, code, unit)]
    `window_back`: how far to look back
    `window_forward`: how far to look forward
    """
    global LABEL_ID
    global PADDING
    for doc in docs:
        if type(doc) is not list:
            doc = list(doc)
        # add dummy items and tokenize units
        doc = PADDING * window_back + \
              [(dyad, role, code, unit.split()) for (dyad, role, code, unit) in doc] + \
              PADDING * window_forward
        feat_doc = []
        for i in range(window_back, len(doc) - window_forward):
            _, role, code, unit = doc[i]
            # integer label; make megam happy
            label = LABEL_ID(code)
            # add features
            feats = set()
            feats.add('CUR_SPEAKER_' + role)
            for j in range(1, window_back+1):
                _, role, _, _ = doc[i-j]
                feats.add('BACK_{}_SPEAKER_{}'.format(j, role))
            for j in range(1, window_forward+1):
                _, role, _, _ = doc[i+j]
                feats.add('FORWARD_{}_SPEAKER_{}'.format(j, role))
            feat_doc.append((label, sorted(feats)))
        yield feat_doc


if __name__ == '__main__':
    try:
        which = sys.argv[1]
        writer = {'megam': feat_writer.megam_writer,
                  'crfsuite': feat_writer.crfsuite_writer}[which]
        out_dir = sys.argv[2]
        train_in, dev_in, test_in = sys.argv[3:6]

        sys.argv.extend(["1", "1"])
        window_back = int(sys.argv[6])
        window_forward = int(sys.argv[7])
    except:
        print 'Usage: {} which(=megam|crfsuite) out_dir train dev test [window_back=1 window_forward=1]'.format(sys.argv[0])
        exit(1)

    for (purpose, path) in zip(["train", "dev", "test"], [train_in, dev_in, test_in]):
        with open(path) as fi:
            with open(out_dir + '/' + purpose + '.' + which, 'w') as fo:
                writer(iter_features(common.lazy_load_dyads(fi),
                                     window_back, window_forward),
                       fo)

    with open(out_dir + '/' + 'map.' + which, 'w') as f:
        cPickle.dump(LABEL_ID, f)

