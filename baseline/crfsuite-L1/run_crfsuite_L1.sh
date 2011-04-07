#!/bin/sh

export PATH="`pwd`/../../tools:$PATH"

declare -i lambda=128
declare -i max_accuracy=0
declare -i max_lambda=$lambda

echo "LAMBDA CORRECT" > crf.log

while [ $lambda -ge 1 ]; do
    crfsuite learn -m crf.model.$lambda -p regularization=L1 -p regularization.sigma=$lambda train.crfsuite > crf.log.$lambda 2>&1
    declare -i accuracy=`crfsuite tag -qt -m crf.model.$lambda dev.crfsuite 2>&1 | tee -a crf.log.$lambda | grep '^Item accuracy' | grep -oE '[0-9]+ /' | grep -oE '[0-9]+'`
    echo $lambda $accuracy | tee -a crf.log
    if [ $accuracy -ge $max_accuracy ]; then
	max_accuracy=$accuracy
	max_lambda=$lambda
    fi
    unset accuracy
    lambda=$(($lambda/2))
done

crfsuite tag -m crf.model.$max_lambda test.crfsuite > crf.out 2> /dev/null
crfsuite tag -qt -m crf.model.$max_lambda test.crfsuite > crf.test 2> /dev/null