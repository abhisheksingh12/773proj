#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

# prepare input
tmp="megam.run"
cp train.megam "$tmp"
echo "DEV" >> "$tmp"
cat dev.megam >> "$tmp"
echo "TEST" >> "$tmp"
cat test.megam >> "$tmp"

# training
megam -maxi 500 -lambda 100 -tune multiclass "$tmp" > "$tmp.weights"
# testing
megam -predict "$tmp.weights" multiclass test.megam | grep -oE '^[0-9]+' > "$tmp.out"