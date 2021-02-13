#! /bin/bash
GCC=${CROSS_COMPILE}gcc

if [ ! -f $1.c ]; then
    echo "not found $1.c in $CODETOP/src/app "
    exit
fi
echo "single app build $1.c in $CODETOP/src/app "
OUTPATH=out
mkdir -p $OUTPATH
$GCC -o $OUTPATH/$1 $1.c 
# $GCC -static  -fno-exceptions  -o hello hello.c
cp $OUTPATH/$1 $CODETOP/nfs/root
