#!/bin/sh

LIBLBFGS_VERSION=1.10
CRFSUITE_VERSION=0.11

# building liblbfgs
mkdir -p liblbfgs-build
rm -rf liblbfgs-build/*

cd liblbfgs-$LIBLBFGS_VERSION
./configure --prefix=`pwd`/../liblbfgs-build
make && make install && make distclean
cd ..


# building crfsuite
mkdir -p crfsuite-build
rm -rf crfsuite-build/*

cd crfsuite-$CRFSUITE_VERSION
./configure --prefix=`pwd`/../crfsuite-build --with-liblbfgs=`pwd`/../liblbfgs-build
make && make install && make distclean
cd ..

ln -sf crfsuite-build/bin/crfsuite .