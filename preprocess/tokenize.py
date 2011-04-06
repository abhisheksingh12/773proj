#!/usr/bin/env python2.7

import sys
import string
import re

from nltk.tokenize import word_tokenize

from common import *

PUNCT = set(list(string.punctuation))
RE_PUNCT = '(' + '|'.join(map(re.escape, PUNCT)) + ')'

def fix_punct(word):
    if not PUNCT.intersection(word):
        return word
    word = re.sub(',,+', ',', word)
    word = re.sub(r'\.\.+', '...', word)
    nopunct = re.sub(RE_PUNCT, '', word)
    if not nopunct or re.match('^[0-9]+$', nopunct):
        return word
    word = re.sub(',', ' , ', word)
    word = re.sub(r'\.\.\.', ' ... ', word)
    return word


write_dyads(((dyad, role, code, ' '.join(map(fix_punct, word_tokenize(unit))))
             for one_dyad in lazy_load_dyads(sys.stdin)
             for (dyad, role, code, unit) in one_dyad),
            sys.stdout)
