#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

# grouping
for i in train dev test; do
    ../../coarsefine/merge_labels.py megam.groups $i.group map.megam < $i.megam > $i.group.all
done

for i in "all" `seq 0 8`; do
    # prepare input
    tmp="megam.run.$i"
    cp train.group.$i "$tmp"
    echo "DEV" >> "$tmp"
    cat dev.group.$i >> "$tmp"
    echo "TEST" >> "$tmp"
    cat test.group.$i >> "$tmp"

    # training
    megam -maxi 500 -lambda 100 -tune multiclass "$tmp" > "megam.weights.$i"
    # # testing
    # megam -predict "megam.weights.$i" multiclass test.group.$i | ../../eval/megam_kbest.py > "$tmp.out"

    # # # evaluation
    # # ../../feateng/num_to_code.py map.megam < test.megam > test.labels
    # # ../../feateng/num_to_code.py map.megam < megam.run.out > megam.run.out.labels
    # # ../../eval/accuracy.py test.labels megam.run.out.labels > megam.run.eval
    # # ../../eval/conf_mat.py test.labels megam.run.out.labels > megam.run.csv

    # # k-best evaluation
    # for k in `seq 10`; do
    # 	../../eval/accuracy.py test.group.$i "$tmp.out" $k | grep Overall
    # done | grep -oE '[0-9]+\.[0-9]+' | cat -n > "$tmp.kbest"
done
# clean up
rm megam.run.* {train,test,dev}.group*

for i in "all" `seq 0 8`; do
    megam -predict "megam.weights.$i" multiclass test.megam 2> /dev/null > "megam.out.$i"
done