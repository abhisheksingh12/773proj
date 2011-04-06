#!/bin/sh

if [ -z "$1" -a -z "$2" ]; then
    echo "Usage: $0 input outdir"
    exit 1
fi

if [ -z "$2" ]; then
    input=data/out
    output="$1"
else
    input="$1"
    output="$2"
fi

mkdir -p "$output"
rm -r "$output"
mkdir -p "$output"

if [ "$input" = data/out ]; then
    cat "$input"
else
    preprocess/fields.py < "$input" 2> /dev/null
fi | \
    preprocess/drop.py 36 41 74 | \
    preprocess/lower_case.py | \
    preprocess/char_subst.py | \
    preprocess/strip_comma.py | \
    preprocess/tokenize.py | \
    preprocess/split.py "$output"
