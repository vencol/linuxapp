#! /bin/bash
GCC=${CROSS_COMPILE}gcc

if [[ "$1" == "" ]] || [[ ! -d $1 ]]; then
    echo "not found priject $1 in $CODETOP/src/app/$1 "
    exit
fi
cd $CODETOP/src/app/$1
./app.sh $1
