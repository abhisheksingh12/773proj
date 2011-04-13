#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

# prepare input
tmp="megam.run"
cp train.megam "$tmp"
echo "DEV" >> "$tmp"
cat dev.megam >> "$tmp"
echo "TEST" >> "$tmp"
cat test.megam >> "$tmp"

# # training
# megam -maxi 500 -lambda 100 -tune multiclass "$tmp" > "$tmp.weights"
# testing
megam -predict "$tmp.weights" multiclass test.megam | ../../eval/megam_kbest.py > "$tmp.out"

# evaluation
../../feateng/num_to_code.py map.megam < test.megam > test.labels
../../feateng/num_to_code.py map.megam < megam.run.out > megam.run.out.labels
../../eval/accuracy.py test.labels megam.run.out.labels > megam.run.eval
../../eval/conf_mat.py test.labels megam.run.out.labels > megam.run.csv
