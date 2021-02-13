#! /bin/bash
GCC=${CROSS_COMPILE}gcc

echo "build $1 app in $CODETOP/src/app/$1 "
OUTPATH=../out/$1
mkdir -p $OUTPATH
$GCC -o $OUTPATH/$1 hello.c 
# $GCC -static  -fno-exceptions  -o hello hello.c
cp $OUTPATH/$1 $CODETOP/nfs/root
