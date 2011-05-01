#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""

"""

from collections import defaultdict
from math import *

# Use defaultdict to represent a dictionary probability source

SEP = "_"
TRACE = True

def mesg(s):
    if TRACE: print s


def log2(x):
    if x > 0:
        return log(x, 2.0)
    else:
        return float("-inf")


def m_log_n(m, n):
    """make sure 0 log 0 = 0"""
    if m != 0:
        return m * log2(n)
    else:
        return 0


def n_log_n(n):
    return m_log_n(n, n)


def make_index(data):
    res = defaultdict(set)
    for pos in xrange(len(data)):
        res[data[pos]].add(pos)
    return dict(res)


def update_index_adjacent(index, wa, wb, pa):
    # mesg("update index")
    len_a = len(wa.split(SEP))
    len_b = len(wb.split(SEP))
    pb = set([i + len_a for i in pa])
    ind_a = index[wa]
    ind_b = index[wb]
    ind_a.difference_update(pa)
    ind_b.difference_update(pb)
    index[wa + SEP + wb] = set([i for i in pa])
    if not ind_a:
        del index[wa]
    if not ind_b:
        del index[wb]


def find_pos(words, target, cur_words, cur_data):
    while cur_data < target:
        cur_data += len(words[cur_words].split(SEP))
        cur_words += 1
    return cur_words, cur_data


def update_words_gamma_d_adjacent(gamma_d, words, wa, wb):
    # FIXME: slow!
    # mesg("update words")
    new_word = wa + SEP + wb
    pos_words = 0
    pos_data = 0
    for pos in sorted(gamma_d[wa][wb]):
        pos_words, pos_data = find_pos(words, pos, pos_words, pos_data)
        if pos_data != pos:
            mesg("w: %d d: %d p: %d - %s %s" % (pos_words, pos_data, pos, words[pos_words], words[pos_words + 1]))
        assert pos_data == pos
        if pos_words - 1 >= 0:
            # has left word
            left = words[pos_words - 1]
            left_pos = pos - len(left.split(SEP))
            gamma_d[left][wa].remove(left_pos)
            gamma_d[left][new_word].add(left_pos)
        if pos_words + 2 < len(words):
            # has right word
            right = words[pos_words + 2]
            right_pos = pos + len(wa.split(SEP))
            gamma_d[wb][right].remove(right_pos)
            gamma_d[new_word][right].add(pos)
        words[pos_words:pos_words + 2] = [new_word]
    del gamma_d[wa][wb]


def compressed_bits(index, l, omega, gamma_d, wa, wb):
    # return cp
    len_a = len(wa.split(SEP))
    len_b = len(wb.split(SEP))
    ind_a = index[wa]
    ind_b = index[wb]

    alpha = len(ind_a)
    beta = len(ind_b)
    gamma = len(gamma_d[wa][wb])

    # data part
    data_compression = n_log_n(alpha - gamma) + n_log_n(beta - gamma) + \
                       n_log_n(gamma) - n_log_n(omega - gamma) + \
                       n_log_n(omega) - n_log_n(alpha) - n_log_n(beta)
    # model part
    if alpha != gamma:
        if beta != gamma:
            model_d_size = len_a + len_b + 1
        else:
            model_d_size = len_a
    else:
        if beta != gamma:
            model_d_size = len_b
        else:
            model_d_size = -1
    model_compression = log2(l + 1) * (-model_d_size)

    total_compression = data_compression + model_compression
    # mesg("omega: %d gamma: %d alpha: %d beta: %d" % \
    #      (omega, gamma, alpha, beta))
    # mesg("d: %g m: %g t: %g" % (data_compression, model_compression, total_compression))
    return total_compression


puncts = set([".", ",", "!", "?", ";", ":"])

def choose_adjacent_pair(index, l, gamma_d, words, skip_heads, skip_tails):
    # return (wa, wb, last_compress)
    if len(words) < 2:
        # no more pairs
        return None, None, None, None, 0
    # still have pairs
    omega = len(words)
    # first pair
    start = 0
    visited = set()
    while start < len(words) and \
              (words[start] in skip_heads or \
               words[start + 1] in skip_tails):
        visited.add(words[start] + SEP + words[start + 1])
        start += 1
    if start == len(words):
        return None, None, None, None, 0
    mwa, mwb = words[start], words[start + 1]
    visited.add(mwa + SEP + mwb)
    mcp = compressed_bits(index, l, omega, gamma_d, mwa, mwb)
    # mesg("%s_%s %g" % (mwa, mwb, mcp))
    # rest
    for i in xrange(start + 1, len(words) - 1):
        # check next word pair
        wa, wb = words[i], words[i + 1]
        new_word = wa + SEP + wb
        if new_word in visited:
            # mesg("%s_%s visited" % (wa, wb))
            continue
        if wa in skip_heads or \
           wb in skip_tails:
            # we don't want wa to be skip_heads, wb to be skip_tails
            continue
        if wa == wb:
            continue
        # not visited
        visited.add(new_word)
        # compute compression
        cp = compressed_bits(index, l, omega, gamma_d, wa, wb)
        # mesg("%s_%s %g" % (wa, wb, cp))
        if cp > mcp:
            mwa, mwb, mcp = wa, wb, cp
    # mesg("%s_%s %g" % (mwa, mwb, mcp))
    return mwa, mwb, mcp

def make_gamma_d(data):
    res = defaultdict(lambda: defaultdict(set))
    for i in xrange(len(data) - 1):
        res[data[i]][data[i + 1]].add(i)
    return res


def induct_cml2(init_data, min_compress=5.0, skip_heads=puncts, skip_tails=puncts, output=None):
    """MDL induct with crude_model_length2"""

    # sample size
    n = len(init_data)
    mesg("sample size: %d" % n)

    # initialize alphabet
    alphabet = set(init_data)
    l = len(alphabet)
    mesg("alphabet size: %d" % l)

    # initialize working data
    words = init_data[:]

    # make index
    word_index = make_index(init_data)
    mesg("word_index done")

    # make gamma_d
    gamma_d = make_gamma_d(init_data)
    mesg("gamma_d done")

    # start looping
    changes = []
    iter_count = 1
    try:
        while True:
            mesg("iteration %d" % iter_count)
            # choose one adjacent pair of words that make
            wa, wb, cp = choose_adjacent_pair(word_index, l, gamma_d, words, skip_heads, skip_tails)
            pa = gamma_d[wa][wb]
            # mesg("%s %s" % (wa, wb))
            if cp > min_compress:
                mesg("compressed %g bits with %d %s+%s" % (cp, len(pa), wa, wb))
                changes.append(wa + '+' + wb)
                # update_gamma_d_adjacent(gamma_d, words, wa, wb)
                update_index_adjacent(word_index, wa, wb, pa)
                update_words_gamma_d_adjacent(gamma_d, words, wa, wb)
            else:
                mesg("iteration has converged")
                break
            # write temporary result
            # if type(output) is str and output:
            #     owrite = file("%s-%04d" % (output, iter_count), "w")
            #     owrite.write("%s %s %d %g\n\n" % (wa, wb, len(pa), cp))
            #     owrite.write("\n".join(words))
            #     owrite.close()
            # next iteration
            iter_count += 1
    except KeyboardInterrupt:
        mesg("interrupted, output current results")
    # apply changes
    if type(output) is str and output:
        file(output + "-words", "w").write("\n".join(words))
        file(output + "-changes", "w").write("\n".join(changes))
    return words
