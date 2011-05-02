#!/usr/bin/env python2.7

import sys
import cPickle

import feat_writer
import common

import nltk.stem

# must use integers as labels since megam requires it!
LABEL_ID = common.IncrCounter()
PADDING = [(None, None, None, [])]
STEM = False

stem = nltk.stem.PorterStemmer().stem


def bigrams(sent, window):
    N = len(sent)
    for i in range(N):
        for j in range(i+1, min(N, i + window + 1)):
            yield (sent[i], sent[j])


def trigrams(sent, window):
    N = len(sent)
    for i in range(N):
        for j in range(i+1, min(N, i + window + 1)):
            for k in range(j+1, min(N, i + window + 1)):
                yield (sent[i], sent[j], sent[k])


def fourgrams(sent, window):
    N = len(sent)
    for i in range(N):
        for j in range(i+1, min(N, i + window + 1)):
            for k in range(j+1, min(N, i + window + 1)):
                for l in range(k+1, min(N, i + window + 1)):
                    yield (sent[i], sent[j], sent[k], sent[l])


def iter_features(docs, window_back=1, window_forward=1, ngram_window=5):
    """A generator of features

    `docs`: [(dyad, role, code, unit)]
    `window_back`: how far to look back
    `window_forward`: how far to look forward
    """
    global LABEL_ID
    global PADDING
    global STEM

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
            if STEM: unit = map(stem, unit)
            # integer label; make megam happy
            label = LABEL_ID(code)
            # add features
            feats = set()
            feats.update(('CUR_UNIGRAM_{}'.format(w)
                          for w in unit))
            feats.update(('CUR_BIGRAM_{}/{}'.format(*w)
                          for w in bigrams(['<s>'] + unit + ['</s>'], ngram_window)))
            feats.update(('CUR_TRIGRAM_{}/{}/{}'.format(*w)
                          for w in trigrams(['<s>', '<s>'] + unit + ['</s>'], ngram_window)))
            feats.update(('CUR_FOURGRAM_{}/{}/{}'.format(*w)
                          for w in fourgrams(['<s>', '<s>', '<s>'] + unit + ['</s>'], ngram_window)))

            for j in range(1, window_back+1):
                _, _, _, unit = doc[i-j]
                if STEM: unit = map(stem, unit)
                feats.update(('BACK_{}_UNIGRAM_{}'.format(j, w)
                              for w in unit))
                feats.update(('BACK_{}_BIGRAM_{}/{}'.format(j, *w)
                              for w in bigrams(['<s>'] + unit + ['</s>'], ngram_window)))
                feats.update(('BACK_{}_TRIGRAM_{}/{}/{}'.format(j, *w)
                              for w in trigrams(['<s>', '<s>'] + unit + ['</s>'], ngram_window)))
                feats.update(('BACK_{}_FOURGRAM_{}/{}/{}'.format(j, *w)
                              for w in fourgrams(['<s>', '<s>', '<s>'] + unit + ['</s>'], ngram_window)))
            for j in range(1, window_forward+1):
                _, _, _, unit = doc[i+j]
                if STEM: unit = map(stem, unit)
                feats.update(('FORWARD_{}_UNIGRAM_{}'.format(j, w)
                              for w in unit))
                feats.update(('FORWARD_{}_BIGRAM_{}/{}'.format(j, *w)
                              for w in bigrams(['<s>'] + unit + ['</s>'], ngram_window)))
                feats.update(('FORWARD_{}_TRIGRAM_{}/{}/{}'.format(j, *w)
                              for w in trigrams(['<s>', '<s>'] + unit + ['</s>'], ngram_window)))
                feats.update(('FORWARD_{}_FOURGRAM_{}/{}/{}'.format(j, *w)
                              for w in fourgrams(['<s>', '<s>', '<s>'] + unit + ['</s>'], ngram_window)))

            feat_doc.append((label, sorted(feats)))
        yield feat_doc


if __name__ == '__main__':
    try:
        which = sys.argv[1]
        writer = {'megam': feat_writer.megam_writer,
                  'crfsuite': feat_writer.crfsuite_writer}[which]
        out_dir = sys.argv[2]
        train_in, dev_in, test_in = sys.argv[3:6]

        window_back = int(sys.argv[6])
        window_forward = int(sys.argv[7])
        ngram_window = int(sys.argv[8])

        STEM = int(sys.argv[9])
    except:
        print 'Usage: {} which(=megam|crfsuite) out_dir train dev test window_back window_forward ngram_window stem'.format(sys.argv[0])
        exit(1)

    for (purpose, path) in zip(["train", "dev", "test"], [train_in, dev_in, test_in]):
        with open(path) as fi:
            with open(out_dir + '/' + purpose + '.' + which, 'w') as fo:
                writer(iter_features(common.lazy_load_dyads(fi),
                                     window_back, window_forward,
                                     ngram_window),
                       fo)

    with open(out_dir + '/' + 'map.' + which, 'w') as f:
        cPickle.dump(LABEL_ID, f)

