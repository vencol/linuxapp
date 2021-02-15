#! /bin/bash
GCC=${CROSS_COMPILE}gcc

if [[ "$1" == "" ]] || [[ ! -f $1.c ]]; then
    echo "not found $1.c in $CODETOP/src/app "
    exit
fi
OUTPATH=$(realpath .)/out
mkdir -p $OUTPATH
if [[ "$2" == "" ]] || [[ "$2" == "nfs" ]]; then
    echo "single app build $1.c in $CODETOP/src/app "
    mkdir -p $OUTPATH
    $GCC -lpthread -o $OUTPATH/$1 $1.c 
    # $GCC -static  -fno-exceptions  -o hello hello.c
    if [ "$2" == "nfs" ]; then
        cp $OUTPATH/$1 $CODETOP/nfs/root
    fi
elif [ "$2" == "clean" ]; then
    rm $OUTPATH/$1
fi
