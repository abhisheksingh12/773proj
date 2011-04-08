#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

declare -i lambda=128
declare -i max_accuracy=0

echo "LAMBDA CORRECT" > crf-L1.log

for  lambda in 128 64 32 16 8 4 3 2 1 "0.5" "0.25" "0.1"; do
    crfsuite learn -m crf-L1.model.$lambda -p regularization=L1 -p regularization.sigma=$lambda train.crfsuite > crf-L1.log.$lambda 2>&1
    declare -i accuracy=`crfsuite tag -qt -m crf-L1.model.$lambda dev.crfsuite 2>&1 | tee -a crf-L1.log.$lambda | grep '^Item accuracy' | grep -oE '[0-9]+ /' | grep -oE '[0-9]+'`
    echo $lambda $accuracy | tee -a crf-L1.log
    if [ $accuracy -ge $max_accuracy ]; then
	max_accuracy=$accuracy
	max_lambda=$lambda
    fi
    unset accuracy
done

echo "Using $max_lambda" | tee -a crf-L1.log

crfsuite tag -m crf-L1.model.$max_lambda test.crfsuite > crf-L1.out 2> /dev/null
crfsuite tag -qt -m crf-L1.model.$max_lambda test.crfsuite > crf-L1.test 2> /dev/null

# evaluation
../../feateng/num_to_code.py map.crfsuite < test.crfsuite > test.labels
../../feateng/num_to_code.py map.crfsuite < crf-L1.out > crf-L1.out.labels
../../eval/accuracy.py test.labels crf-L1.out.labels > crf-L1.eval
../../eval/conf_mat.py test.labels crf-L1.out.labels > crf-L1.run.csv
