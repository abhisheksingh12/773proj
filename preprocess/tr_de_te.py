#!/usr/bin/env python2.7

import sys
import random

def output_lines(docs, how_many, fo):
    while docs and how_many > 0:
        doc = docs.pop()
        for line in doc: fo.write(line.strip() + '\n')
        how_many -= len(doc)


try:
    tr, de, te = map(float, sys.argv[1:4])
    out_prefix = sys.argv[4]
    files = sys.argv[5:]
except:
    print 'Usage: {} tr_weight de_weight te_weight out_prefix files'.format(sys.argv[0])
    exit(1)

docs = []
for i in files:
    with open(i) as f: docs.append(list(f))

random.shuffle(docs)

total_lines = sum(map(len, docs))
tr_de_te = tr + de + te

for (part, prop) in zip(['train', 'dev', 'test'], [tr, de, te]):
    num_lines = prop / tr_de_te * total_lines
    with open(out_prefix + '.' + part, 'w') as f:
        output_lines(docs, num_lines, f)
