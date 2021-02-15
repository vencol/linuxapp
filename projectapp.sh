#! /bin/bash


if [[ "$1" == "" ]] || [[ ! -d $1 ]]; then
    echo "not found priject $1 in $CODETOP/src/app/$1 "
    exit
fi
OUTPATH=$(realpath .)/out
mkdir -p $OUTPATH
cd $1
if [[ "$2" == "" ]] || [[ "$2" == "nfs" ]]; then
    ./app.sh 
    if [ $? == 10 ]; then
        CPDIR=-r
    else
        CPDIR=
    fi
    if [ "$2" == "nfs" ]; then
        cp $CPDIR $OUTPATH/$1/$1 $CODETOP/nfs/root
    fi
else
    ./app.sh $2
fi
