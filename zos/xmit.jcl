//Q4XTWNBU JOB (EQ00,P),'AL DUNSMUIR',NOTIFY=&SYSUID,
//     MSGCLASS=T
/*JOBPARM S=XBAT
//*
//*-------------------------------------------------------------------*/
//* Encode UNZIP LOADLIB using TSO XMIT                               */
//*-------------------------------------------------------------------*/
//XMIT     EXEC PGM=IKJEFT01,DYNAMNBR=20
//DDIN     DD  DISP=SHR,DSN=PS1353.IZ.UNZIP.V610.LOADLIB
//DDOUT    DD  DISP=OLD,DSN=PS1353.IZ.UNZIP.V610.XMIT(UNZIP)
//SYSPRINT DD  SYSOUT=*
//SYSTSPRT DD  SYSOUT=*
//SYSTSIN  DD  *
  TRANSMIT INFOZIP +
     NOCOPYLIST +
     DDNAME(DDIN) +
     NOEPILOG +
     NOLOG +
     NONOTIFY +
     OUTDDNAME(DDOUT) +
     PDS +
     NOPROLOG
//
