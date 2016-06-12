#!/bin/sh

INCDIR=/usr/local/cmmInclude
LIBDIR=/usr/local/cmmLib

for i in $*; do
   j=`basename ${i}`
   echo '[!!!' ${j} '!!!]'
   n=`expr ${j} : '\([^\.]*\)'`
   cc -E -DC -std=c99 -nostdinc -I${INCDIR} -I${LIBDIR} - < ${i} |
   ../../../src/c-- -O > t.$$
   diff ${n}.s t.$$
done

rm t.$$
