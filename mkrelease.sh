#!/bin/sh
#
# Script for generating a release archive.
#

HOST="${1:-i386-aros}"
FORMAT="${2:-lha}"

#make HOST=$HOST clean
make HOST=$HOST all autodoc

if [ "$HOST" = "m68k-amigaos" ]; then
  chmod 775 bin/filesysbox.library.000
  chmod 775 bin/filesysbox.library.020
  chmod 775 bin/filesysbox.library.060
else
  CPU=`echo "${HOST}" | cut -d'-' -f1`
  chmod 775 bin/filesysbox.library.${CPU}
fi;

DESTDIR='tmp'
#FULLVERS=`version bin/filesysbox.library`
#NUMVERS=`echo "${FULLVERS}" | cut -d' ' -f2`

rm -rf ${DESTDIR}
mkdir -p ${DESTDIR}/filesysbox/C
mkdir -p ${DESTDIR}/filesysbox/Libs
mkdir -p ${DESTDIR}/filesysbox/Developer/AutoDocs
mkdir -p ${DESTDIR}/filesysbox/Developer/include/sfd
mkdir -p ${DESTDIR}/filesysbox/Developer/include/fd
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/clib
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/defines
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/inline
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/libraries
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/proto
mkdir -p ${DESTDIR}/filesysbox/Developer/include/include_h/sys

cp -p README ${DESTDIR}/filesysbox
cp -p LICENSE.APL ${DESTDIR}/filesysbox
cp -p releasenotes ${DESTDIR}/filesysbox
if [ "$HOST" = "m68k-amigaos" ]; then
  cp -p Install ${DESTDIR}/filesysbox
  cp -p bin/filesysbox.library.000 ${DESTDIR}/filesysbox/Libs
  cp -p bin/filesysbox.library.020 ${DESTDIR}/filesysbox/Libs
  cp -p bin/filesysbox.library.060 ${DESTDIR}/filesysbox/Libs
  cp -p dismount/bin/FbxDismount ${DESTDIR}/filesysbox/C
else
  cp -p Install-AROS ${DESTDIR}/filesysbox/Install
  cp -p bin/filesysbox.library.${CPU} ${DESTDIR}/filesysbox/Libs/filesysbox.library
  cp -p dismount/bin/FbxDismount.${CPU} ${DESTDIR}/filesysbox/C/FbxDismount
fi;
cp -p filesysbox.doc ${DESTDIR}/filesysbox/Developer/AutoDocs
cp -p filesysbox_lib.sfd ${DESTDIR}/filesysbox/Developer/include/sfd
cp -p include/fd/filesysbox_lib.fd ${DESTDIR}/filesysbox/Developer/include/fd
cp -p include/clib/filesysbox_protos.h ${DESTDIR}/filesysbox/Developer/include/include_h/clib
cp -p include/defines/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/include_h/defines
cp -p include/inline/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/include_h/inline
cp -p include/libraries/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/include_h/libraries
cp -p include/proto/filesysbox.h ${DESTDIR}/filesysbox/Developer/include/include_h/proto
cp -p include/sys/statvfs.h ${DESTDIR}/filesysbox/Developer/include/include_h/sys

cp -p icons/def_drawer.info ${DESTDIR}/filesysbox.info
cp -p icons/def_doc.info ${DESTDIR}/filesysbox/README.info
cp -p icons/def_doc.info ${DESTDIR}/filesysbox/LICENSE.APL.info
cp -p icons/def_doc.info ${DESTDIR}/filesysbox/releasenotes.info
cp -p icons/def_install.info ${DESTDIR}/filesysbox/Install.info

case "${FORMAT}" in
  "7z")
    rm -f filesysbox.${HOST}.7z
    7za u filesysbox.${HOST}.7z ./${DESTDIR}/*
    echo "filesysbox.${HOST}.7z created"
    ;;
  "iso")
    rm -f filesysbox.${HOST}.iso
    PREVDIR=`pwd`
    cd ${DESTDIR} && mkisofs -R -o ../filesysbox.${HOST}.iso -V FILESYSBOX .
    cd ${PREVDIR}
    echo "filesysbox.${HOST}.iso created"
    ;;
  "lha")
    rm -rf filesysbox.${HOST}.lha
    PREVDIR=`pwd`
    cd ${DESTDIR} && lha ao5 ../filesysbox.${HOST}.lha *
    cd ${PREVDIR}
    echo "filesysbox.${HOST}.lha created"
    ;;
esac

rm -rf ${DESTDIR}

