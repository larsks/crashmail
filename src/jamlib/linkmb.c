/***********************************************************************
**
**  LinkMB - Links a messagebase on MSGID/REPLY
**
**  by Johan Billing
**
***********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "jam.h"

struct Msg
{
   unsigned long MsgIdCRC;
   unsigned long ReplyCRC;
   unsigned long ReplyTo;
   unsigned long Reply1st;
   unsigned long ReplyNext;
   unsigned long OldReplyTo;
   unsigned long OldReply1st;
   unsigned long OldReplyNext;
};

int CompareMsgIdReply(s_JamBase *Base_PS,ulong msgidmsg,ulong replymsg)
{
   int Status_I;

   s_JamMsgHeader 	MsgIdHeader_S;
   s_JamMsgHeader 	ReplyHeader_S;
   s_JamSubPacket*   MsgIdSubPacket_PS;
   s_JamSubPacket*   ReplySubPacket_PS;
   s_JamSubfield*    MsgIdField_PS = NULL;
   s_JamSubfield*    ReplyField_PS = NULL;

   Status_I = JAM_ReadMsgHeader(Base_PS,msgidmsg,&MsgIdHeader_S,&MsgIdSubPacket_PS );

   if(Status_I)
   {
      return(0);
   }

   Status_I = JAM_ReadMsgHeader(Base_PS,replymsg,&ReplyHeader_S,&ReplySubPacket_PS );

   if(Status_I)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      return(0);
   }

   for ( MsgIdField_PS = JAM_GetSubfield( MsgIdSubPacket_PS ); MsgIdField_PS; MsgIdField_PS = JAM_GetSubfield( NULL ) )
      if(MsgIdField_PS->LoID == JAMSFLD_MSGID) break;

   for ( ReplyField_PS = JAM_GetSubfield( ReplySubPacket_PS ); ReplyField_PS; ReplyField_PS = JAM_GetSubfield( NULL ) )
      if(ReplyField_PS->LoID == JAMSFLD_REPLYID) break;

   if(!ReplyField_PS || !MsgIdField_PS)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(0);
   }

   if(ReplyField_PS->DatLen != MsgIdField_PS->DatLen)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(0);
   }

   if(strncmp(ReplyField_PS->Buffer,MsgIdField_PS->Buffer,ReplyField_PS->DatLen) != 0)
   {
      JAM_DelSubPacket(MsgIdSubPacket_PS);
      JAM_DelSubPacket(ReplySubPacket_PS);
      return(0);
   }

   JAM_DelSubPacket(MsgIdSubPacket_PS);
   JAM_DelSubPacket(ReplySubPacket_PS);
   return(1);
}

int jam_linkmb(struct Area *area,ulong oldnum)
{
   struct jam_Area *ja;
   s_JamBaseHeader BaseHeader_S;
   ulong nummsgs,res;
   struct Msg *msg,*msgs;

   printf("Linking JAM area %s                       \x0d",area->Tagname);
   fflush(stdout);

   if(!(ja=jam_getarea(area)))
      return(FALSE);

   if(JAM_GetMBSize(Base_PS,&nummsgs))
   {
failed
   }

   if(nummsgs == 0)
      return; /* Nothing to do */

   /* Read msgid/reply */

   if(!(msgs=malloc(nummsgs*sizeof(struct Msg))))
   {
      LogWrite(1,SYSTEMERR,"Out of memory, cannot link JAM area %s",area->Tagname);
      return;
   }

   for(c=0;c<nummsgs;c++)
   {
      s_JamMsgHeader         Header_S;

      res = JAM_ReadMsgHeader( Base_PS, c, &Header_S, NULL);

      msgs[c].MsgIdCRC=-1;
      msgs[c].ReplyCRC=-1;
      msgs[c].ReplyTo=0;
      msgs[c].Reply1st=0;
      msgs[c].ReplyNext=0;
      msgs[c].OldReplyTo=0;
      msgs[c].OldReply1st=0;
      msgs[c].OldReplyNext=0;

      if(!Status_I)
      {
         msgs[c].MsgIdCRC=Header_S.MsgIdCRC;
         msgs[c].ReplyCRC=Header_S.ReplyCRC;
         msgs[c].OldReplyTo=Header_S.ReplyTo;
         msgs[c].OldReply1st=Header_S.Reply1st;
         msgs[c].OldReplyNext=Header_S.ReplyNext;
      }
   }

   for(c=oldnum;c<nummsgs;c++)
   {
      if(msgs[c].ReplyCRC != -1)
      {
         /* See if this is reply to a message */

         for(d=0;d<nummsgs;d++)
            if(jam_CompareMsgIdReply(ja->Base_PS,nummsgs,d,c)) break;
            {
               /* Backward link found */

               jam_setreplyto(nummsgs,c,d);
               jam_setreply(nummsgs,d,c);
            }
         }
      }

      /* See if there are any replies to this message */ 

      if(msgs[c].MsgIdCRC != -1)
      {
         for(d=0;d<nummsgs;d++)
            if(jam_CompareMsgIdReply(ja->Base_PS,nummsgs,c,d)) 
            {
               /* Forward link found */

               jam_setreplyto(nummsgs,d,c);
               jam_setreply(nummsgs,c,d);
            }
      }
   }

   /* Update links */

   for(c=0;c<nummsgs;c++)
      if(msgs[c].ReplyTo != msgs[c].OldReplyTo || msgs[c].Reply1st != msgs[c].OldReply1st || msgs[c].ReplyNext != msgs[c].OldReplyNext)
      {
         res = JAM_ReadMsgHeader( Base_PS, c, &Header_S, NULL);

         if(!res)
         {
            Header_S.ReplyTo=msgs[c].ReplyTo;
            Header_S.Reply1st=msgs[c].Reply1st;
            Header_S.ReplyNext=msgs[c].ReplyNext;
            JAM_ChangeMsgHeader(Base_PS,c,&Header_S);
         }
      }
                           
   free(msgs);
}

void setreplyto(struct Msg *msgs,ulong num,ulong dest)
{
   msgs
}


            if(link == -1)
            {
               for(d=c+1;d<nummsgs;d++)
                  if(msgs[d].MsgIdCRC == msgs[c].ReplyCRC)
                  {
                     if(argc > 2)
                     {
                        link=d;
                        break;
                     }
                     else if(CompareMsgIdReply(Base_PS,d,c))
                     {
                        link=d;
                        break;
                     }
                  }
            }

            if(link != -1)
            {
               /* Update backward links */

               msgs[c].ReplyTo=d+BaseHeader_S.BaseMsgNum;

               /* Update forward links */

               if(msgs[d].Reply1st == 0)
               {
                  msgs[d].Reply1st=c+BaseHeader_S.BaseMsgNum;
               }
               else
               {
                  n=msgs[d].Reply1st-BaseHeader_S.BaseMsgNum;

                  while(msgs[n].ReplyNext)
                     n=msgs[n].ReplyNext-BaseHeader_S.BaseMsgNum;

                  msgs[n].ReplyNext=c+BaseHeader_S.BaseMsgNum;
               }
            }
         }
      }

      printf("Updating messagebase...\n");

      for(c=0;c<nummsgs;c++)
      {
         s_JamMsgHeader 	Header_S;

         if(c % 50 == 0)
         {
            printf("Done %lu%% \x0d",100*c/nummsgs);
            fflush(stdout);
         }

         if(msgs[c].ReplyTo != msgs[c].OldReplyTo || msgs[c].Reply1st != msgs[c].OldReply1st || msgs[c].ReplyNext != msgs[c].OldReplyNext)
         {
            Status_I = JAM_ReadMsgHeader( Base_PS, c, &Header_S, NULL);

            if(!Status_I)
            {
               Header_S.ReplyTo=msgs[c].ReplyTo;
               Header_S.Reply1st=msgs[c].Reply1st;
               Header_S.ReplyNext=msgs[c].ReplyNext;
               JAM_ChangeMsgHeader(Base_PS,c,&Header_S);
            }

            printf("Message %lu changed!\n",c+BaseHeader_S.BaseMsgNum);
         }
      }

      printf("Done!    \n");

      free(msgs);
   }
   else
   {
      printf("Messagebase is empty, nothing to do...\n");
   }
   
   JAM_UnlockMB(Base_PS);
   JAM_CloseMB( Base_PS );

   exit(0);
}

