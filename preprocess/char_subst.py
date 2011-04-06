#!/usr/bin/env python2.7

import re
import sys

def subst_question_mark(match):
    s = match.group()
    if s[1:] in ["s", "t", "d", "re", "ll", "ve"]:
        return "'" + s[1:]
    else:
        return "? " + s[1:]

def subst_non_ascii(match):
    s = match.group()
    if s == '\x92':
        return "'"
    elif s == '\x85':
        return ' '
    else:
        return ''

for line in sys.stdin:
    sys.stdout.write(re.sub(r'\bim\b', "i'm",
                            re.sub('["\x85\x91\x92\x93\x94\x96\xad\xe9]', subst_non_ascii,
                                   re.sub(r'i\? m', "i'm",
                                          re.sub(r'\?[a-z]{1,2}', subst_question_mark, line)))))
