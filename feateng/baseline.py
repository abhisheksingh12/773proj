#!/usr/bin/env python2.7
"""Baseline features: bag of words witin a small window and current speaker"""

import sys

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
            feats.add('SPEAKER_' + role)
            feats.update(('CUR_' + w for w in unit))
            for j in range(1, window_back+1):
                prefix = 'BACK_{}_'.format(j)
                _, _, _, unit = doc[i-j]
                feats.update((prefix + w for w in unit))
            for j in range(1, window_forward+1):
                prefix = 'FORWARD_{}_'.format(j)
                _, _, _, unit = doc[i+j]
                feats.update((prefix + w for w in unit))
            feat_doc.append((label, sorted(feats)))
        yield feat_doc


if __name__ == '__main__':
    try:
        which = sys.argv[1]
        writer = {'megam': feat_writer.megam_writer,
                  'crfsuite': feat_writer.crfsuite_writer}[which]
        train_in, dev_in, test_in = sys.argv[2:5]

        sys.argv.extend(["1", "1"])
        window_back = int(sys.argv[5])
        window_forward = int(sys.argv[6])
    except:
        print 'Usage: {} which(=megam|crfsuite) train dev test [window_back=1 window_forward=1]'.format(sys.argv[0])
        exit(1)

    for path in [train_in, dev_in, test_in]:
        with open(path) as fi:
            with open(path + '.' + which, 'w') as fo:
                writer(iter_features(common.lazy_load_dyads(fi),
                                     window_back, window_forward),
                       fo)
