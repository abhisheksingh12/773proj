#!/usr/bin/env python2.7

"""Extract a CFG for py-cfg"""

import sys


START_SYM = "S"
SENT_SYM = "SENT"
DOC_FMT = "DOC_{}"
TOPIC_FMT = "TOPIC_{}"
WORD = "WORD"
WORDS = "WORDS"


def rules_of_sent(id, sent, topic_n):
    global SENT_SYM
    global DOC_FMT
    global TOPIC_FMT
    global WORD

    doc = DOC_FMT.format(id)
    return [(SENT_SYM, [doc]),
            (doc, ["<s>"])] + \
            [(doc, [doc, TOPIC_FMT.format(i)])
             for i in range(topic_n)] + \
             [(WORD, [i]) for i in sent]

MEMO = set()

def write_unadapted((lhs, rhs), fo):
    global MEMO
    r = '{} --> {}'.format(lhs, ' '.join(rhs))
    if r not in MEMO:
        fo.write('0 1 {}\n'.format(r))
        MEMO.add(r)

def write_adapted((lhs, rhs), fo):
    global MEMO
    r = '{} --> {}'.format(lhs, ' '.join(rhs))
    if r not in MEMO:
        fo.write(r + '\n')


if __name__ == '__main__':
    try:
        topic_n = int(sys.argv[1])
    except:
        print >> sys.stderr, "Usage: {} topic_n < input 1> sents 2> rules".format(sys.argv[0])
        raise

    # start symbol...
    write_unadapted((START_SYM, [SENT_SYM]), sys.stderr)
    # monkey at typewriter
    write_unadapted((WORDS, [WORD]), sys.stderr)
    write_unadapted((WORDS, [WORDS, WORD]), sys.stderr)
    # topic rules
    for i in range(topic_n):
        write_adapted((TOPIC_FMT.format(i), [WORDS]), sys.stderr)
    # per sent rules
    for (id, line) in enumerate(sys.stdin):
        sent = line.split()
        # sent with sos added
        sys.stdout.write('<s> ' + line)
        # rules
        for r in rules_of_sent(id, sent, topic_n):
            write_unadapted(r, sys.stderr)
