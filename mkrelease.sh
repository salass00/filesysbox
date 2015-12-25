#!/bin/sh
#
# Script for generating a release archive.
#

HOST="${1:-i386-aros}"

make HOST=${HOST} clean

make HOST=${HOST}

DESTDIR='tmp'
FULLVERS=`version filesysbox.library`
NUMVERS=`echo "${FULLVERS}" | cut -d' ' -f2`

rm -rf ${DESTDIR}
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Libs
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/clib
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/defines
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/inline
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/libraries
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/proto
mkdir -p ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/sys

cp -p LICENSE.APL ${DESTDIR}/filesysbox-${NUMVERS}
cp -p releasenotes ${DESTDIR}/filesysbox-${NUMVERS}
cp -p filesysbox.library ${DESTDIR}/filesysbox-${NUMVERS}/Libs
cp -p include/clib/filesysbox_protos.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/clib
cp -p include/defines/filesysbox.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/defines
cp -p include/inline/filesysbox.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/inline
cp -p include/libraries/filesysbox.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/libraries
cp -p include/proto/filesysbox.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/proto
cp -p include/sys/statvfs.h ${DESTDIR}/filesysbox-${NUMVERS}/Developer/include/sys

rm -f filesysbox.7z
7za u filesysbox.7z ./${DESTDIR}/filesysbox-${NUMVERS}

rm -rf ${DESTDIR}

echo "filesysbox.7z created"

