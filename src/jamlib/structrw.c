/*
    structrw - Platform-independent reading and writing of JAM structs
    Copyright (C) 1999 Johan Billing

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Changes made by Johan Billing 2003-10-22

    - Added #include <string.h>
*/

#include <stdio.h>
#include <string.h>

#include "jam.h"
#include "structrw.h"

ushort jamgetuword(uchar *buf,ulong offset)
{
   return (ushort) buf[offset]+
                   buf[offset+1]*256;
}

void jamputuword(uchar *buf,ulong offset,ushort num)
{
   buf[offset]=num%256;
   buf[offset+1]=num/256;
}

void jamputulong(uchar *buf,ulong offset,ulong num)
{
   buf[offset]=num%256;
   buf[offset+1]=(num / 256) % 256;
   buf[offset+2]=(num / 256 / 256) % 256;
   buf[offset+3]=(num / 256 / 256 / 256) % 256;
}

ulong jamgetulong(uchar *buf,ulong offset)
{
   return (ulong) buf[offset]+
                  buf[offset+1]*256+
                  buf[offset+2]*256*256+
                  buf[offset+3]*256*256*256;
}

int freadjambaseheader(FILE *fp,s_JamBaseHeader *s_JamBaseHeader)
{
   uchar buf[SIZE_JAMBASEHEADER];

   if(fread(buf,SIZE_JAMBASEHEADER,1,fp) != 1)
      return 0;

   memcpy(s_JamBaseHeader->Signature,&buf[JAMBASEHEADER_SIGNATURE],4);

   s_JamBaseHeader->DateCreated = jamgetulong(buf,JAMBASEHEADER_DATECREATED);
   s_JamBaseHeader->ModCounter  = jamgetulong(buf,JAMBASEHEADER_MODCOUNTER);
   s_JamBaseHeader->ActiveMsgs  = jamgetulong(buf,JAMBASEHEADER_ACTIVEMSGS);
   s_JamBaseHeader->PasswordCRC = jamgetulong(buf,JAMBASEHEADER_PASSWORDCRC);
   s_JamBaseHeader->BaseMsgNum  = jamgetulong(buf,JAMBASEHEADER_BASEMSGNUM);

   memcpy(s_JamBaseHeader->RSRVD,&buf[JAMBASEHEADER_RSRVD],1000);

   return 1;
}

int fwritejambaseheader(FILE *fp,s_JamBaseHeader *s_JamBaseHeader)
{
   uchar buf[SIZE_JAMBASEHEADER];

   memcpy(&buf[JAMBASEHEADER_SIGNATURE],s_JamBaseHeader->Signature,4);

   jamputulong(buf,JAMBASEHEADER_DATECREATED, s_JamBaseHeader->DateCreated);
   jamputulong(buf,JAMBASEHEADER_MODCOUNTER,  s_JamBaseHeader->ModCounter);
   jamputulong(buf,JAMBASEHEADER_ACTIVEMSGS,  s_JamBaseHeader->ActiveMsgs);
   jamputulong(buf,JAMBASEHEADER_PASSWORDCRC, s_JamBaseHeader->PasswordCRC );
   jamputulong(buf,JAMBASEHEADER_BASEMSGNUM,  s_JamBaseHeader->BaseMsgNum);

   memcpy(&buf[JAMBASEHEADER_RSRVD],s_JamBaseHeader->RSRVD,1000);

   if(fwrite(buf,SIZE_JAMBASEHEADER,1,fp) != 1)
      return 0;

   return 1;
}

int freadjammsgheader(FILE *fp,s_JamMsgHeader *s_JamMsgHeader)
{
   uchar buf[SIZE_JAMMSGHEADER];

   if(fread(buf,SIZE_JAMMSGHEADER,1,fp) != 1)
      return 0;

   memcpy(s_JamMsgHeader->Signature,&buf[JAMMSGHEADER_SIGNATURE],4);

   s_JamMsgHeader->Revision      = jamgetuword(buf,JAMMSGHEADER_REVISION);
   s_JamMsgHeader->ReservedWord  = jamgetuword(buf,JAMMSGHEADER_RESERVEDWORD);
   s_JamMsgHeader->SubfieldLen   = jamgetulong(buf,JAMMSGHEADER_SUBFIELDLEN);
   s_JamMsgHeader->TimesRead     = jamgetulong(buf,JAMMSGHEADER_TIMESREAD);
   s_JamMsgHeader->MsgIdCRC      = jamgetulong(buf,JAMMSGHEADER_MSGIDCRC);
   s_JamMsgHeader->ReplyCRC      = jamgetulong(buf,JAMMSGHEADER_REPLYCRC);
   s_JamMsgHeader->ReplyTo       = jamgetulong(buf,JAMMSGHEADER_REPLYTO);
   s_JamMsgHeader->Reply1st      = jamgetulong(buf,JAMMSGHEADER_REPLY1ST);
   s_JamMsgHeader->ReplyNext     = jamgetulong(buf,JAMMSGHEADER_REPLYNEXT);
   s_JamMsgHeader->DateWritten   = jamgetulong(buf,JAMMSGHEADER_DATEWRITTEN);
   s_JamMsgHeader->DateReceived  = jamgetulong(buf,JAMMSGHEADER_DATERECEIVED);
   s_JamMsgHeader->DateProcessed = jamgetulong(buf,JAMMSGHEADER_DATEPROCESSED);
   s_JamMsgHeader->MsgNum        = jamgetulong(buf,JAMMSGHEADER_MSGNUM);
   s_JamMsgHeader->Attribute     = jamgetulong(buf,JAMMSGHEADER_ATTRIBUTE);
   s_JamMsgHeader->Attribute2    = jamgetulong(buf,JAMMSGHEADER_ATTRIBUTE2);
   s_JamMsgHeader->TxtOffset     = jamgetulong(buf,JAMMSGHEADER_TXTOFFSET);
   s_JamMsgHeader->TxtLen        = jamgetulong(buf,JAMMSGHEADER_TXTLEN);
   s_JamMsgHeader->PasswordCRC   = jamgetulong(buf,JAMMSGHEADER_PASSWORDCRC);
   s_JamMsgHeader->Cost          = jamgetulong(buf,JAMMSGHEADER_COST);

   return 1;
}

int fwritejammsgheader(FILE *fp,s_JamMsgHeader *s_JamMsgHeader)
{
   uchar buf[SIZE_JAMMSGHEADER];

   memcpy(&buf[JAMMSGHEADER_SIGNATURE],s_JamMsgHeader->Signature,4);

   jamputuword(buf,JAMMSGHEADER_REVISION,      s_JamMsgHeader->Revision);
   jamputuword(buf,JAMMSGHEADER_RESERVEDWORD,  s_JamMsgHeader->ReservedWord);
   jamputulong(buf,JAMMSGHEADER_SUBFIELDLEN,   s_JamMsgHeader->SubfieldLen);
   jamputulong(buf,JAMMSGHEADER_TIMESREAD,     s_JamMsgHeader->TimesRead);
   jamputulong(buf,JAMMSGHEADER_MSGIDCRC,      s_JamMsgHeader->MsgIdCRC);
   jamputulong(buf,JAMMSGHEADER_REPLYCRC,      s_JamMsgHeader->ReplyCRC  );
   jamputulong(buf,JAMMSGHEADER_REPLYTO,       s_JamMsgHeader->ReplyTo);
   jamputulong(buf,JAMMSGHEADER_REPLY1ST,      s_JamMsgHeader->Reply1st);
   jamputulong(buf,JAMMSGHEADER_REPLYNEXT,     s_JamMsgHeader->ReplyNext);
   jamputulong(buf,JAMMSGHEADER_DATEWRITTEN,   s_JamMsgHeader->DateWritten);
   jamputulong(buf,JAMMSGHEADER_DATERECEIVED,  s_JamMsgHeader->DateReceived );
   jamputulong(buf,JAMMSGHEADER_DATEPROCESSED, s_JamMsgHeader->DateProcessed);
   jamputulong(buf,JAMMSGHEADER_MSGNUM,        s_JamMsgHeader->MsgNum);
   jamputulong(buf,JAMMSGHEADER_ATTRIBUTE,     s_JamMsgHeader->Attribute);
   jamputulong(buf,JAMMSGHEADER_ATTRIBUTE2,    s_JamMsgHeader->Attribute2);
   jamputulong(buf,JAMMSGHEADER_TXTOFFSET,     s_JamMsgHeader->TxtOffset);
   jamputulong(buf,JAMMSGHEADER_TXTLEN,        s_JamMsgHeader->TxtLen);
   jamputulong(buf,JAMMSGHEADER_PASSWORDCRC,   s_JamMsgHeader->PasswordCRC);
   jamputulong(buf,JAMMSGHEADER_COST,          s_JamMsgHeader->Cost);

   if(fwrite(buf,SIZE_JAMMSGHEADER,1,fp) != 1)
      return 0;

   return 1;
}

int freadjamindex(FILE *fp,s_JamIndex *s_JamIndex)
{
   uchar buf[SIZE_JAMINDEX];

   if(fread(buf,SIZE_JAMINDEX,1,fp) != 1)
      return 0;

   s_JamIndex->UserCRC   = jamgetulong(buf,JAMINDEX_USERCRC);
   s_JamIndex->HdrOffset = jamgetulong(buf,JAMINDEX_HDROFFSET);

   return 1;
}

int fwritejamindex(FILE *fp,s_JamIndex *s_JamIndex)
{
   uchar buf[SIZE_JAMINDEX];

   jamputulong(buf,JAMINDEX_USERCRC,   s_JamIndex->UserCRC);
   jamputulong(buf,JAMINDEX_HDROFFSET, s_JamIndex->HdrOffset);

   if(fwrite(buf,SIZE_JAMINDEX,1,fp) != 1)
      return 0;

   return 1;
}

int freadjamlastread(FILE *fp,s_JamLastRead *s_JamLastRead)
{
   uchar buf[SIZE_JAMLASTREAD];

   if(fread(buf,SIZE_JAMLASTREAD,1,fp) != 1)
      return 0;

   s_JamLastRead->UserCRC     = jamgetulong(buf,JAMLASTREAD_USERCRC);
   s_JamLastRead->UserID      = jamgetulong(buf,JAMLASTREAD_USERID);
   s_JamLastRead->LastReadMsg = jamgetulong(buf,JAMLASTREAD_LASTREADMSG);
   s_JamLastRead->HighReadMsg = jamgetulong(buf,JAMLASTREAD_HIGHREADMSG);

   return 1;
}

int fwritejamlastread(FILE *fp,s_JamLastRead *s_JamLastRead)
{
   uchar buf[SIZE_JAMLASTREAD];

   jamputulong(buf,JAMLASTREAD_USERCRC,s_JamLastRead->UserCRC);
   jamputulong(buf,JAMLASTREAD_USERID,s_JamLastRead->UserID);
   jamputulong(buf,JAMLASTREAD_LASTREADMSG,s_JamLastRead->LastReadMsg);
   jamputulong(buf,JAMLASTREAD_HIGHREADMSG,s_JamLastRead->HighReadMsg);

   if(fwrite(buf,SIZE_JAMLASTREAD,1,fp) != 1)
      return 0;

   return 1;
}

int fwritejamsavesubfield(FILE *fp,s_JamSaveSubfield *s_JamSaveSubfield)
{
   uchar buf[SIZE_JAMLASTREAD];

   jamputuword(buf,JAMSAVESUBFIELD_LOID,   s_JamSaveSubfield->LoID);
   jamputuword(buf,JAMSAVESUBFIELD_HIID,   s_JamSaveSubfield->HiID);
   jamputulong(buf,JAMSAVESUBFIELD_DATLEN, s_JamSaveSubfield->DatLen);

   if(fwrite(buf,SIZE_JAMSAVESUBFIELD,1,fp) != 1)
      return 0;

   return 1;
}

void getjamsubfield(uchar *buf,s_JamSubfield *Subfield_S)
{
   Subfield_S->LoID   = jamgetuword(buf,JAMSAVESUBFIELD_LOID);
   Subfield_S->HiID   = jamgetuword(buf,JAMSAVESUBFIELD_HIID);
   Subfield_S->DatLen = jamgetulong(buf,JAMSAVESUBFIELD_DATLEN);

   Subfield_S->Buffer = (uchar *) buf + SIZE_JAMSAVESUBFIELD;
}
