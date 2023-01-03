#!/bin/sh
#
# Script for generating a release archive.
#

HOST="${1:-i386-aros}"

make HOST=$HOST clean
make HOST=$HOST all autodoc

DESTDIR='tmp'
#FULLVERS=`version filesysbox.library`
#NUMVERS=`echo "${FULLVERS}" | cut -d' ' -f2`

rm -rf ${DESTDIR}
mkdir -p ${DESTDIR}/filesysbox/Libs
mkdir -p ${DESTDIR}/filesysbox/Developer/AutoDocs
mkdir -p ${DESTDIR}/filesysbox/Developer/include/clib
mkdir -p ${DESTDIR}/filesysbox/Developer/include/defines
mkdir -p ${DESTDIR}/filesysbox/Developer/include/inline
mkdir -p ${DESTDIR}/filesysbox/Developer/include/libraries
mkdir -p ${DESTDIR}/filesysbox/Developer/include/proto
mkdir -p ${DESTDIR}/filesysbox/Developer/include/sys

cp -p README ${DESTDIR}/filesysbox
cp -p LICENSE.APL ${DESTDIR}/filesysbox
cp -p releasenotes ${DESTDIR}/filesysbox
if [ "$HOST" = "m68k-amigaos" ]; then
  cp -p filesysbox.library.000 ${DESTDIR}/filesysbox/Libs
  cp -p filesysbox.library.020 ${DESTDIR}/filesysbox/Libs
else
  cp -p filesysbox.library ${DESTDIR}/filesysbox/Libs
fi;
cp -p filesysbox.doc ${DESTDIR}/filesysbox/Developer/AutoDocs
cp -p include/clib/filesysbox_protos.h ${DESTDIR}/filesysbox/Developer/include/clib
cp -p include/defines/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/defines
cp -p include/inline/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/inline
cp -p include/libraries/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/libraries
cp -p include/proto/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/proto
cp -p include/sys/statvfs.h ${DESTDIR}/filesysbox/Developer/include/sys

rm -f filesysbox.${HOST}.7z
7za u filesysbox.${HOST}.7z ./${DESTDIR}/filesysbox

rm -rf ${DESTDIR}

echo "filesysbox.${HOST}.7z created"

