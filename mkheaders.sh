#!/bin/sh
#
# Script for generating the SDK headers.
#

sfdc --mode=clib -o include/clib/filesysbox_protos.h filesysbox_lib.sfd
sfdc --mode=fd -o include/fd/filesysbox_lib.fd filesysbox_lib.sfd
sfdc --mode=pragmas -o include/pragmas/filesysbox_pragmas.h filesysbox_lib.sfd
sfdc --mode=macros --target=i386-pc-aros -o include/defines/filesysbox.h filesysbox_lib.sfd
sfdc --mode=macros --target=m68k-amigaos -o include/inline/filesysbox.h filesysbox_lib.sfd
sfdc --mode=proto -o include/proto/filesysbox.h filesysbox_lib.sfd
