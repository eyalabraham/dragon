#!/bin/bash
#
# lst2h.sh
#
#   Batch process all *asm files into *lst files and into *h files
#

LSTFILES=`ls *asm`

for FILE in $LSTFILES; do
    FILEBASENAME="${FILE%.*}"
    echo $FILE to $FILEBASENAME.h
    as9 $FILE -l
    awk -f ../../scripts/lst2h.awk $FILEBASENAME.lst > $FILEBASENAME.h
    rm $FILEBASENAME.lst
done
