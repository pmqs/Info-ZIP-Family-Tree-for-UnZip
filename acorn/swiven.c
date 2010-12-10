/*
  Copyright (c) 1990-2010 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 1999-Oct-05 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, both of these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.cdrom.com/pub/infozip/license.html
*/
#include "kernel.h"
#include "swis.h"

#include "riscos.h"

/* This is not defined in Acorn C swis.h header: */
#ifndef DDEUtils_Prefix
#define DDEUtils_Prefix 0x042580
#endif
#ifndef MimeMap_Translate
#define MimeMap_Translate 0x50B00
#endif

_kernel_oserror *SWI_OS_FSControl_26(char *source, char *dest, int actionmask)
{ /* copy */
  return _swix(OS_FSControl,_INR(0,3),26,source,dest,actionmask);
}

_kernel_oserror *SWI_OS_FSControl_27(char *filename, int actionmask)
{ /* wipe */
  return _swix(OS_FSControl,_INR(0,3),27,filename,0,actionmask);
}

_kernel_oserror *SWI_OS_GBPB_9(char *dirname, void *buf, int *number,
                               int *offset, int size, char *match)
{ /* read dir */
  int _number,_offset;
  _kernel_oserror *err = _swix(OS_GBPB,_INR(0,6)|_OUTR(3,4),9,dirname,buf,*number,*offset,size,match,&_number,&_offset);
  if(err)
    return err;
  *number = _number;
  *offset = _offset;
  return 0;
}

_kernel_oserror *SWI_OS_File_1(char *filename, unsigned int loadaddr,
                               unsigned int execaddr, int attrib)
{ /* write file attributes */
  return _swix(OS_File,_INR(0,5),1,filename,loadaddr,execaddr,0,attrib);
}

_kernel_oserror *SWI_OS_File_5(char *filename, int *objtype,
                               unsigned int *loadaddr, unsigned int *execaddr,
                               int *length, int *attrib)
{ /* read file info */
  int _objtype,_length,_attrib;
  unsigned int _loadaddr,_execaddr;
  _kernel_oserror *err = _swix(OS_File,_INR(0,1)|_OUT(0)|_OUTR(2,5),5,filename,&_objtype,&_loadaddr,&_execaddr,&_length,&_attrib);
  if(err)
    return err;
  if(objtype)
    *objtype = _objtype;
  if(loadaddr)
    *loadaddr = _loadaddr;
  if(execaddr)
    *execaddr = _execaddr;
  if(length)
    *length = _length;
  if(attrib)
    *attrib = _attrib;
  return 0;
}

_kernel_oserror *SWI_OS_File_6(char *filename)
{ /* delete */
  return _swix(OS_File,_INR(0,1),6,filename);
}

_kernel_oserror *SWI_OS_File_7(char *filename, int loadaddr, int execaddr,
                               int size)
{ /* create an empty file */
  return _swix(OS_File,_INR(0,5),7,filename,loadaddr,execaddr,0,size);
}

_kernel_oserror *SWI_OS_File_8(char *dirname)
{ /* create directory */
  return _swix(OS_File,_INR(0,1)|_IN(4),8,dirname,0);
}

_kernel_oserror *SWI_OS_CLI(char *cmd)
{ /* execute a command */
  return _swix(OS_CLI,_IN(0),cmd);
}

int SWI_OS_ReadC(void)
{ /* get a key from the keyboard buffer */
  int key;
  _swix(OS_ReadC,_OUT(0),&key);
  return key;
}

_kernel_oserror *SWI_OS_ReadVarVal(char *var, char *buf, int len,
                                   int *bytesused)
{ /* reads an OS variable */
  int _bytesused;
  _kernel_oserror *err = _swix(OS_ReadVarVal,_INR(0,4)|_OUT(2),var,buf,len,0,0,&_bytesused);
  if(err)
    return err;
  if(bytesused)
    *bytesused = _bytesused;
  return 0;
}

_kernel_oserror *SWI_OS_FSControl_54(char *buffer, int dir, char *fsname,
                                     int *size)
{ /* reads the path of a specified directory */
  int _size;
  _kernel_oserror *err = _swix(OS_FSControl,_INR(0,5)|_OUT(5),54,buffer,dir,fsname,0,*size,&_size);
  if(err)
    return err;
  *size = _size;
  return 0;
}

_kernel_oserror *SWI_OS_FSControl_37(char *pathname, char *buffer, int *size)
{ /* canonicalise path */
  int _size;
  _kernel_oserror *err = _swix(OS_FSControl,_INR(0,5)|_OUT(5),37,pathname,buffer,0,0,*size,&_size);
  if(err)
    return err;
  *size = _size;
  return 0;
}


_kernel_oserror *SWI_DDEUtils_Prefix(char *dir)
{ /* sets the 'prefix' directory */
  return _swix(DDEUtils_Prefix,_IN(0),dir);
}

int SWI_Read_Timezone(void)
{ /* returns the timezone offset (centiseconds) */
  int ofs;
  _kernel_oserror *err = _swix(Territory_ReadCurrentTimeZone,_OUT(0),&ofs);
  if(err)
     return 0;
  return ofs;
}

int SWI_MimeMap_Translate(const char *ext)
{
  int result;
  _kernel_oserror *err = _swix(MimeMap_Translate,_INR(0,2)|_OUT(3),3,ext,0,&result);
  if(err)
    return -1;
  return result;
}
