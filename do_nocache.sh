#! /bin/bash

tmp=${1}"_tmp"

echo remove $tmp
rm -rf $tmp

echo copy $1 to $tmp
cp -rf $1 $tmp

echo 'gcc nocache.c -o nocache'
gcc nocache.c -o nocache

echo './nocache'$1 $tmp
./nocache $1 $tmp

rm -rf $1

echo rename $tmp $1
mv $tmp $1