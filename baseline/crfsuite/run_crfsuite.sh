#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

declare -i max_accuracy=0
declare -i max_lambda=UNKNOWN

echo "LAMBDA CORRECT" > crf.log

for  lambda in 128 64 32 16 8 4 3 2 1 "0.5" "0.25" "0.1"; do
    crfsuite learn -m crf.model.$lambda -p regularization.sigma=$lambda train.crfsuite > crf.log.$lambda 2>&1
    declare -i accuracy=`crfsuite tag -qt -m crf.model.$lambda dev.crfsuite 2>&1 | tee -a crf.log.$lambda | grep '^Item accuracy' | grep -oE '[0-9]+ /' | grep -oE '[0-9]+'`
    echo $lambda $accuracy | tee -a crf.log
    if [ $accuracy -ge $max_accuracy ]; then
	max_accuracy=$accuracy
	max_lambda=$lambda
    fi
    unset accuracy
done

echo "Using $max_lambda" | tee -a crf.log

crfsuite tag -m crf.model.$max_lambda test.crfsuite > crf.out 2> /dev/null
crfsuite tag -qt -m crf.model.$max_lambda test.crfsuite > crf.test 2> /dev/null

# evaluation
../../feateng/num_to_code.py map.crfsuite < test.crfsuite > test.labels
../../feateng/num_to_code.py map.crfsuite < crf.out > crf.out.labels
../../eval/accuracy.py test.labels crf.out.labels > crf.eval
../../eval/conf_mat.py test.labels crf.out.labels > crf.run.csv
