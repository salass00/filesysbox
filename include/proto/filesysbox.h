/* Automatically generated header (sfdc 1.11e)! Do not edit! */

#ifndef PROTO_FILESYSBOX_H
#define PROTO_FILESYSBOX_H

#include <clib/filesysbox_protos.h>

#ifndef _NO_INLINE
# if defined(__GNUC__)
#  ifdef __AROS__
#   include <defines/filesysbox.h>
#  else
#   include <inline/filesysbox.h>
#  endif
# else
#  include <pragmas/filesysbox_pragmas.h>
# endif
#endif /* _NO_INLINE */

#ifdef __amigaos4__
# include <interfaces/filesysbox.h>
# ifndef __NOGLOBALIFACE__
   extern struct FileSysBoxIFace *IFileSysBox;
# endif /* __NOGLOBALIFACE__*/
#endif /* !__amigaos4__ */
#ifndef __NOLIBBASE__
  extern struct Library *
# ifdef __CONSTLIBBASEDECL__
   __CONSTLIBBASEDECL__
# endif /* __CONSTLIBBASEDECL__ */
  FileSysBoxBase;
#endif /* !__NOLIBBASE__ */

#endif /* !PROTO_FILESYSBOX_H */
