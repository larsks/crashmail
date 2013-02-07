#include "crashmail.h"

static size_t ptrsize = sizeof(void *);

void SendRemoteAreafix(void);
struct Arealist *FindForward(uchar *tagname,uchar *flags);
void RemoteAreafixAdd(uchar *area,struct ConfigNode *node);
void RemoteAreafixRemove(uchar *area,struct ConfigNode *node);
bool CheckFlags(uchar group,uchar *node);

struct afReply
{
   struct MemMessage *mm;
	uchar subject[72];
	ulong lines;
	ulong part;
};

struct afReply *afInitReply(uchar *fromname,struct Node4D *from4d,uchar *toname,struct Node4D *to4d,uchar *subject);
void afFreeReply(struct afReply *af);
void afAddLine(struct afReply *af,uchar *fmt,...);
void afSendMessage(struct afReply *af);

void AddCommandReply(struct afReply *af,uchar *cmd,uchar *reply);

void rawSendList(short type,struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode);
void rawSendHelp(struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode);
void rawSendInfo(struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode);

void SendRemoveMessages(struct Area *area);
void RemoveDeletedAreas(void);

#define COMMAND_UPDATE     1
#define COMMAND_ADD        2
#define COMMAND_REMOVE     3

bool AreaFix(struct MemMessage *mm)
{
   struct Arealist *arealist;
   struct ConfigNode *cnode;
   struct Area *area;
   struct TossNode   *temptnode;
   struct BannedNode *bannednode;
   struct TextChunk *chunk;
   ulong c,d,q,jbcpos;
   uchar password[100],buf[100],buf2[100];
   bool stop,sendareaquery,sendarealist,sendareaunlinked,sendhelp,sendinfo,done,iswild;
   bool globalrescan,wasfeed;
   uchar *opt,areaname[100],command;
   struct Route *tmproute;
   struct afReply *afr;
		
   Print4D(&mm->OrigNode,buf);
   LogWrite(4,AREAFIX,"AreaFix: Request from %s",buf);

   /* Init reply structure */

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&mm->OrigNode)==0) break;

   if(!tmproute)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(1,TOSSINGERR,"No route found for %s",buf);
      return(TRUE);
   }

 	if(!(afr=afInitReply(config.cfg_Sysop,&tmproute->Aka->Node,mm->From,&mm->OrigNode,"AreaFix response")))
		return(FALSE);

   for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
      if(Compare4D(&cnode->Node,&mm->OrigNode)==0) break;

   if(!cnode)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(2,AREAFIX,"AreaFix: Unknown node %s",buf);
      afAddLine(afr,"Sorry, your node is not configured here.");
      afSendMessage(afr);
		afFreeReply(afr);
      return(TRUE);
   }

   jbcpos=0;
   jbstrcpy(password,mm->Subject,100,&jbcpos);

   if(stricmp(password,cnode->AreafixPW)!=0)
   {
      Print4D(&mm->OrigNode,buf);
      LogWrite(2,AREAFIX,"AreaFix: Wrong password \"%s\" from %s",password,buf);
      afAddLine(afr,"Sorry, wrong password.");
      afSendMessage(afr);
		afFreeReply(afr);
      return(TRUE);
   }

   sendarealist=FALSE;
   sendareaquery=FALSE;
   sendareaunlinked=FALSE;
   sendinfo=FALSE;
   sendhelp=FALSE;
   
   globalrescan=FALSE;

   done=FALSE;

   if(jbstrcpy(password,mm->Subject,100,&jbcpos))
   {
      if(stricmp(password,"-q")==0 || stricmp(password,"-l")==0)
      {
         done=TRUE;
         sendarealist=TRUE;
         AddCommandReply(afr,password,"Sending list of all areas");
      }
   }

   stop=FALSE;

   for(chunk=(struct TextChunk *)mm->TextChunks.First;chunk && !stop && !ctrlc;chunk=chunk->Next)
   {
      for(c=0;c<chunk->Length && !stop && !ctrlc;c++)
      {
         for(d=0;d<100 && chunk->Data[c+d]!=13 && chunk->Data[c+d]!=10 && c+d<chunk->Length;d++)
            buf[d]=chunk->Data[c+d];

         buf[d]=0;
         c+=d;

         if(strncmp(buf,"---",3)==0)
            stop=TRUE;

			stripleadtrail(buf);

         if(buf[0]=='%')
         {
            jbcpos=0;
            jbstrcpy(buf2,buf,100,&jbcpos);

            if(stricmp(buf2,"%PAUSE")==0)
            {
               if(cnode->Flags & NODE_PASSIVE)
               {
						AddCommandReply(afr,buf,"Your system is already passive");
               }
               else
               {
                  cnode->Flags|=NODE_PASSIVE;
						cnode->changed=TRUE;
                  config.changed=TRUE;
                  done=TRUE;
						AddCommandReply(afr,buf,"Your system is marked as passive");
						AddCommandReply(afr,"","Send %%RESUME to get echomail again");
               }
            }
            else if(stricmp(buf2,"%RESUME")==0)
            {
               if(cnode->Flags & NODE_PASSIVE)
               {
                  cnode->Flags&=~NODE_PASSIVE;
						cnode->changed=TRUE;
                  config.changed=TRUE;
                  done=TRUE;
						AddCommandReply(afr,buf,"Your system is active again");
               }
               else
               {
						AddCommandReply(afr,buf,"Your system is not paused");
               }
            }
            else if(stricmp(buf2,"%PWD")==0)
            {
               if(jbstrcpy(buf2,buf,40,&jbcpos))
               {
                  strcpy(cnode->AreafixPW,buf2);
						cnode->changed=TRUE;
                  config.changed=TRUE;
                  done=TRUE;
						AddCommandReply(afr,buf,"AreaFix password changed");
               }
               else
               {
						AddCommandReply(afr,buf,"No new password specified");
               }
            }
            else if(stricmp(buf2,"%RESCAN")==0)
            {
               if(config.cfg_Flags & CFG_ALLOWRESCAN)
               {
						AddCommandReply(afr,buf,"Will rescan all areas added after this line");
                  globalrescan=TRUE;
               }
               else
               {
						AddCommandReply(afr,buf,"No rescanning allowed");
               }
            }
            else if(stricmp(buf2,"%COMPRESS")==0)
            {
               if(jbstrcpy(buf2,buf,40,&jbcpos))
               {
                  bool gotpacker;
                  struct Packer *tmppacker;

                  gotpacker=FALSE;
                  tmppacker=NULL;

                  if(buf2[0]!='?')
                  {
                     if(stricmp(buf2,"NONE")==0)
                     {
                        tmppacker=NULL;
                        gotpacker=TRUE;
                     }
                     else
                     {
                       
								for(tmppacker=(struct Packer *)config.PackerList.First;tmppacker;tmppacker=tmppacker->Next)
                           if(stricmp(buf2,tmppacker->Name)==0 && tmppacker->Packer[0]) break;

                        if(tmppacker)
                           gotpacker=TRUE;

                        else
									AddCommandReply(afr,buf,"Unknown packer. Choose from this list:");
                     }
                  }
                  else
                  {
							AddCommandReply(afr,buf,"Sending list of packers:");
                  }

                  if(gotpacker)
                  {
                     cnode->Packer=tmppacker;
							cnode->changed=TRUE;
                     config.changed=TRUE;
							AddCommandReply(afr,buf,"Packed changed");
                  }
                  else
                  {
                     for(tmppacker=(struct Packer *)config.PackerList.First;tmppacker;tmppacker=tmppacker->Next)
                     {
                        if(tmppacker->Packer[0])
									AddCommandReply(afr,"",tmppacker->Name);
                     }

							AddCommandReply(afr,"","NONE");
                  }
                  done=TRUE;
               }
               else
               {
						AddCommandReply(afr,buf,"No new method specified");
               }
            }
            else if(stricmp(buf2,"%LIST")==0)
            {
               sendarealist=TRUE;
               done=TRUE;
					AddCommandReply(afr,buf,"Sending list of all areas");
            }

            else if(stricmp(buf2,"%QUERY")==0)
            {
               sendareaquery=TRUE;
               done=TRUE;
					AddCommandReply(afr,buf,"Sending query");
            }

            else if(stricmp(buf2,"%UNLINKED")==0)
            {
               sendareaunlinked=TRUE;
               done=TRUE;
					AddCommandReply(afr,buf,"Sending list of all unlinkedareas");
            }
            else if(stricmp(buf2,"%HELP")==0)
            {
               sendhelp=TRUE;
               done=TRUE;
					AddCommandReply(afr,buf,"Sending help file");
            }
            else if(stricmp(buf2,"%INFO")==0)
            {
               sendinfo=TRUE;
               done=TRUE;
					AddCommandReply(afr,buf,"Sending configuration info");
            }
            else
            {
               done=TRUE;
					AddCommandReply(afr,buf,"Unknown command");
            }
         }
         else if(buf[0]!=1 && buf[0]!=0)
         {
            ulong rescannum;
            bool patterndone,dorescan,areaexists,success;
            struct Area *rescanarea;

            done=TRUE;
            rescannum=0;

            dorescan=FALSE;

            /* Separate command, name and opt */

            mystrncpy(areaname,buf,100);
				
            opt="";

            for(q=0;areaname[q];q++)
               if(areaname[q]==',') opt=&areaname[q];

            if(opt[0]==',')
            {
               opt[0]=0;
               opt=&opt[1];

               while(opt[0]==32)
                  opt=&opt[1];
            }

            striptrail(areaname);
            striptrail(opt);

            if(areaname[0]=='-')
            {
               command=COMMAND_REMOVE;
               strcpy(areaname,&areaname[1]);
            }
            else if(areaname[0]=='=')
            {
               command=COMMAND_UPDATE;
               strcpy(areaname,&areaname[1]);
            }
            else
            {
               command=COMMAND_ADD;

               if(areaname[0]=='+')
                  strcpy(areaname,&areaname[1]);
            }

            if(!osCheckPattern(areaname))
            {
               afAddLine(afr,"%-30.30s Invalid pattern",buf);
            }
            else
            {
					iswild=osIsPattern(areaname);

               if(iswild)
               {
                  afAddLine(afr,"%s",buf);
                  afAddLine(afr,"");
               }

               patterndone=FALSE;
               areaexists=FALSE;
               rescanarea=NULL;

               for(area=(struct Area *)config.AreaList.First;area && !ctrlc;area=area->Next)
                  if(area->AreaType == AREATYPE_ECHOMAIL)
                  {
                     if(osMatchPattern(areaname,area->Tagname))
                     {
                        areaexists=TRUE;

                        for(temptnode=(struct TossNode *)area->TossNodes.First;temptnode;temptnode=temptnode->Next)
                           if(temptnode->ConfigNode == cnode) break;

                        switch(command)
                        {
                           case COMMAND_ADD:
                              if(!temptnode)
                              {
                                 /* Do we have access? */

                                 if(CheckFlags(area->Group,cnode->Groups) || CheckFlags(area->Group,cnode->ReadOnlyGroups))
                                 {
                                    patterndone=TRUE;

                                    for(bannednode=(struct BannedNode *)area->BannedNodes.First;bannednode;bannednode=bannednode->Next)
                                       if(bannednode->ConfigNode == cnode) break;

                                    /* Are we banned? */

                                    if(bannednode)
                                    {
                                       if(iswild)
                                          afAddLine(afr,"   You have been banned from %s",area->Tagname);

                                       else
														AddCommandReply(afr,buf,"You have been banned from that area");

                                       LogWrite(3,AREAFIX,"AreaFix: This node is banned in %s",area->Tagname);
                                    }
                                    else
                                    {
                                       if((area->Flags & AREA_DEFREADONLY) || CheckFlags(area->Group,cnode->ReadOnlyGroups))
                                       {
                                          LogWrite(4,AREAFIX,"AreaFix: Attached to %s as read-only",area->Tagname);

                                          if(iswild)
                                             afAddLine(afr,"   Attached to %s as read-only",area->Tagname);

                                          else
															AddCommandReply(afr,buf,"Attached as read-only");
                                       }
                                       else
                                       {
                                          LogWrite(4,AREAFIX,"AreaFix: Attached to %s",area->Tagname);

                                          if(iswild)
                                             afAddLine(afr,"   Attached to %s",area->Tagname);

                                          else
															AddCommandReply(afr,buf,"Attached");
                                       }

                                       if(!(temptnode=osAllocCleared(sizeof(struct TossNode))))
                                       {
														afFreeReply(afr);
                                          nomem=TRUE;
                                          return(FALSE);
                                       }

                                       temptnode->ConfigNode=cnode;

                                       if((area->Flags & AREA_DEFREADONLY) || CheckFlags(area->Group,cnode->ReadOnlyGroups))
                                          temptnode->Flags=TOSSNODE_READONLY;

                                       jbAddNode(&area->TossNodes,(struct jbNode *)temptnode);
                                       rescanarea=area;
													area->changed=TRUE;
                                       config.changed=TRUE;
                                    }
                                 }
                                 else if(!iswild)
                                 {
												AddCommandReply(afr,buf,"You don't have access to that area");
                                 }
                              }
                              else
                              {
                                 patterndone=TRUE;

                                 if(iswild)
                                    afAddLine(afr,"   You are already attached to %s",area->Tagname);

                                 else
												AddCommandReply(afr,buf,"You are already attached to that area");
                              }
                              break;

                           case COMMAND_REMOVE:
                              if(!temptnode)
                              {
                                 if(!iswild)
                                 {
												AddCommandReply(afr,buf,"You are not attached to that area");
                                    patterndone=TRUE;
                                 }
                              }
                              else
                              {
                                 patterndone=TRUE;

                                 if((area->Flags & AREA_MANDATORY) && !(temptnode->Flags & TOSSNODE_FEED))
                                 {
                                    if(iswild)
                                       afAddLine(afr,"   You are not allowed to detach from %s",area->Tagname);

                                    else
													AddCommandReply(afr,buf,"You are not allowed to detach from that area");
                                 }
                                 else
                                 {
                                    LogWrite(4,AREAFIX,"AreaFix: Detached from %s",area->Tagname);

                                    if(iswild)
                                       afAddLine(afr,"   Detached from %s",area->Tagname);

                                    else
													AddCommandReply(afr,buf,"Detached");

                                    wasfeed=FALSE;

                                    if(temptnode->Flags & TOSSNODE_FEED)
                                       wasfeed=TRUE;

                                    jbFreeNode(&area->TossNodes,(struct jbNode *)temptnode);
											   area->changed=TRUE;
                                    config.changed=TRUE;

                                    if(wasfeed && (config.cfg_Flags & CFG_REMOVEWHENFEED))
                                    {
                                       LogWrite(2,AREAFIX,"AreaFix: Feed disconnected, removing area %s",area->Tagname);
                                       SendRemoveMessages(area);

                                       area->AreaType=AREATYPE_DELETED;
                                    }
                                    else if(config.cfg_Flags & CFG_AREAFIXREMOVE)
                                    {
                                       if(area->TossNodes.First == NULL ||
                                         (((struct TossNode*)area->TossNodes.First)->Next==NULL &&
                                          ((struct TossNode*)area->TossNodes.First)->Flags & TOSSNODE_FEED))
                                       {
                                          if(!area->Messagebase)
                                          {
                                             if(area->TossNodes.First)
                                             {
                                                LogWrite(3,AREAFIX,"AreaFix: Area %s removed, message sent to areafix",area->Tagname);
                                                RemoteAreafixRemove(area->Tagname,((struct TossNode*)area->TossNodes.First)->ConfigNode);
                                             }
                                             else
                                             {
                                                LogWrite(3,AREAFIX,"AreaFix: Area %s removed",area->Tagname);
                                             }

                                             area->AreaType=AREATYPE_DELETED;
                                          }
                                       }
                                    }
                                 }
                              }
                              break;

                           case COMMAND_UPDATE:
                              if(temptnode)
                              {
                                 patterndone=TRUE;

                                 if(iswild)
                                 {
                                    afAddLine(afr,"   Nothing to do with %s",area->Tagname);
                                 }
                                 else
                                 {
												AddCommandReply(afr,buf,"Will rescan area");
                                    rescanarea=area;
                                 }
                              }
                              break;
                        }
                     }
                  }


               if(command==COMMAND_UPDATE || command==COMMAND_ADD)
               {
                  if(!iswild && patterndone && rescanarea)
                  {
                     if(strnicmp(opt,"r=",2)==0)
                     {
                        rescannum=atoi(&opt[2]);
                        dorescan=TRUE;
                     }
                     else if(opt[0])
                     {
                        afAddLine(afr,"%-30.30s Unknown option %s","",opt);
                     }

                     if(globalrescan || dorescan)
                     {
                        if(config.cfg_Flags & CFG_ALLOWRESCAN)
                        {
                           if(!rescanarea->Messagebase)
                           {
                              afAddLine(afr,"%-30.30s Can't rescan, area is pass-through","");
                           }
									else if(!rescanarea->Messagebase->rescanfunc)
									{
                              afAddLine(afr,"%-30.30s Can't rescan, messagebase does not support rescan","");
									}
									else
                           {
                              LogWrite(4,AREAFIX,"AreaFix: Rescanning %s",rescanarea->Tagname);

										RescanNode=cnode;
								   	rescan_total=0;
										success=(*rescanarea->Messagebase->rescanfunc)(rescanarea,rescannum,HandleRescan);
										RescanNode=NULL;

										if(!success)
										{
											afFreeReply(afr);
	                              return(FALSE);
										}

                              LogWrite(4,AREAFIX,"AreaFix: Rescanned %lu messages",rescan_total);
                              afAddLine(afr,"%-30.30s Rescanned %lu messages","",rescan_total);
                           }
                        }
                        else
                        {
                           afAddLine(afr,"%-30.30s No rescanning allowed","");
                        }
                     }
                  }
               }

               switch(command)
               {
                  case COMMAND_ADD:
                     if(!patterndone)
                     {
                        if(iswild)
                        {
                           afAddLine(afr,"   There were no matching areas to connect to");
                        }
                        else
                        {
                           if(!areaexists)
                           {
                              if(cnode->Flags & NODE_FORWARDREQ)
                              {
                                 arealist=FindForward(areaname,cnode->Groups);

                                 if(arealist)
                                 {
												uchar buf2[100];

                                    LogWrite(3,AREAFIX,"AreaFix: %s requested from %u:%u/%u.%u",areaname,arealist->Node->Node.Zone,arealist->Node->Node.Net,arealist->Node->Node.Node,arealist->Node->Node.Point);

                                    sprintf(buf2,"Request sent to %u:%u/%u.%u",arealist->Node->Node.Zone,arealist->Node->Node.Net,arealist->Node->Node.Node,arealist->Node->Node.Point);
												AddCommandReply(afr,buf,buf2);
                                    RemoteAreafixAdd(areaname,arealist->Node);

                                    area=AddArea(areaname,&arealist->Node->Node,&tmproute->Aka->Node,TRUE,config.cfg_Flags & CFG_FORWARDPASSTHRU);
                                    area->Group=arealist->Group;

                                    if(area)
                                    {
                                       ushort flags;

                                       flags=0;

                                       if(CheckFlags(area->Group,cnode->ReadOnlyGroups))
                                          flags|=TOSSNODE_READONLY;

                                       AddTossNode(area,cnode,flags);

                                       config.changed=TRUE;
													area->changed=TRUE;
                                       areaexists=TRUE;
                                    }
                                 }
                              }
                           }

                           if(!areaexists)
                           {
										AddCommandReply(afr,buf,"Unknown area");
                              LogWrite(3,AREAFIX,"AreaFix: Unknown area %s",areaname);
                           }
                        }
                     }
                     break;

                  case COMMAND_REMOVE:
                     if(!patterndone)
                     {
                        if(iswild)
                           afAddLine(afr,"   There were no matching areas to detach from");

                        else
									AddCommandReply(afr,buf,"Unknown area");
                     }
                     break;

                  case COMMAND_UPDATE:
                     if(!patterndone)
                     {
                        if(iswild)
                           afAddLine(afr,"   There were no matching areas");

                        else
									AddCommandReply(afr,buf,"You are not attached to this area");
                     }
                     else
                     {
                        if(rescanarea && !globalrescan && opt[0]==0)
									AddCommandReply(afr,buf,"Nothing to do");
                     }
                     break;
               }

               if(iswild)
                  afAddLine(afr,"");
            }
			}

			RemoveDeletedAreas();
      }
   }

   if(done==FALSE)
      afAddLine(afr,"Nothing to do.");

   if(nomem || ioerror || ctrlc)
   {
      afFreeReply(afr);
      return(FALSE);
   }

   afSendMessage(afr);
	afFreeReply(afr);

   if(sendarealist)
      rawSendList(SENDLIST_FULL,&tmproute->Aka->Node,mm->From,cnode);

   if(sendareaquery)
      rawSendList(SENDLIST_QUERY,&tmproute->Aka->Node,mm->From,cnode);

   if(sendareaunlinked)
      rawSendList(SENDLIST_UNLINKED,&tmproute->Aka->Node,mm->From,cnode);

   if(sendhelp)
      rawSendHelp(&tmproute->Aka->Node,mm->From,cnode);

   if(sendinfo)
      rawSendInfo(&tmproute->Aka->Node,mm->From,cnode);

   /* Restore old MemMessage */

   SendRemoteAreafix();

	return(TRUE);
}

void SendRemoveMessages(struct Area *area)
{
   struct TossNode *tn;
   uchar buf[100];
   struct MemMessage *mm;
	
   for(tn=(struct TossNode *)area->TossNodes.First;tn;tn=tn->Next)
   {
      if(tn->ConfigNode->Flags & NODE_SENDAREAFIX)
      {
         LogWrite(5,AREAFIX,"AreaFix: Sending message to AreaFix at %d:%d/%d.%d",
            tn->ConfigNode->Node.Zone,
            tn->ConfigNode->Node.Net,
            tn->ConfigNode->Node.Node,
            tn->ConfigNode->Node.Point);

			if(!(mm=mmAlloc()))
			{	
				nomem=TRUE;
				return;
			}
		
         Copy4D(&mm->DestNode,&tn->ConfigNode->Node);
         Copy4D(&mm->OrigNode,&area->Aka->Node);

         strcpy(mm->From,config.cfg_Sysop);
         strcpy(mm->To,tn->ConfigNode->RemoteAFName);
         strcpy(mm->Subject,tn->ConfigNode->RemoteAFPw);

         mm->Attr = FLAG_PVT;

 			MakeFidoDate(time(NULL),mm->DateTime);  
 
   		mm->Flags |= MMFLAG_AUTOGEN;

   		MakeNetmailKludges(mm);

		   if(config.cfg_Flags & CFG_ADDTID)
      		AddTID(mm);

         sprintf(buf,"-%s\x0d",area->Tagname);
         mmAddLine(mm,buf);

         sprintf(buf,"---\x0dGenerated by CrashMail "VERSION"\x0d");
			mmAddLine(mm,buf);

         HandleMessage(mm);
			
			mmFree(mm);
      }

      if((tn->ConfigNode->Flags & NODE_SENDTEXT) && !(tn->Flags & TOSSNODE_FEED))
      {
         LogWrite(5,AREAFIX,"AreaFix: Notifying sysop at %d:%d/%d.%d",
            tn->ConfigNode->Node.Zone,
            tn->ConfigNode->Node.Net,
            tn->ConfigNode->Node.Node,
            tn->ConfigNode->Node.Point);

			if(!(mm=mmAlloc()))
				return;

         Copy4D(&mm->DestNode,&tn->ConfigNode->Node);
         Copy4D(&mm->OrigNode,&area->Aka->Node);

         strcpy(mm->From,config.cfg_Sysop);
         strcpy(mm->To,tn->ConfigNode->SysopName);
         strcpy(mm->Subject,"Area removed");

         mm->Attr = FLAG_PVT;

 			MakeFidoDate(time(NULL),mm->DateTime);  
 
   		mm->Flags |= MMFLAG_AUTOGEN;

   		MakeNetmailKludges(mm);

		   if(config.cfg_Flags & CFG_ADDTID)
      		AddTID(mm);

         sprintf(buf,"The area \"%s\" has been removed by the uplink.\x0d",area->Tagname);
         mmAddLine(mm,buf);

         if(tn->ConfigNode->Flags & NODE_SENDAREAFIX)
         {
            sprintf(buf,"A message has been sent to your AreaFix.\x0d");
            mmAddLine(mm,buf);
         }

         HandleMessage(mm);

         mmFree(mm);
      }
   }
}

void RemoveDeletedAreas(void)
{
	struct Area *area,*temparea;

   area=(struct Area *)config.AreaList.First;

   while(area)
   {
      temparea=area->Next;

	   if(area->AreaType == AREATYPE_DELETED)
		{
         jbFreeList(&area->TossNodes);
			jbFreeNode(&config.AreaList,(struct jbNode *)area);
			config.changed=TRUE;
      }

      area=temparea;
   }
}

struct StatsNode
{
   struct StatsNode *Next;
   uchar *Tagname;
   uchar *Desc;
   uchar Group;
   bool Attached;
   bool Feed;
   ulong WeekKB; /* -1 means unknown */
};

struct jbList SortList;

bool AddSortList(uchar *tagname,uchar *desc,uchar group,bool attached,bool feed,long weekkb)
{
   uchar *mtagname,*mdesc;
   struct StatsNode *ss;

   mtagname=(uchar *)osAlloc(strlen(tagname)+1);
   mdesc=(uchar *)osAlloc(strlen(desc)+1);
   ss=(struct StatsNode *)osAlloc(sizeof(struct StatsNode));

   if(!mtagname || !mdesc || !ss)
	{
		if(mtagname) osFree(mtagname);
		if(mdesc) osFree(mdesc);
		if(ss) osFree(ss);
      return(FALSE);
	}
	
   strcpy(mtagname,tagname);
   strcpy(mdesc,desc);

   ss->Tagname=mtagname;
   ss->Desc=mdesc;
   ss->Group=group;
   ss->Attached=attached;
   ss->Feed=feed;
   ss->WeekKB=weekkb;

   jbAddNode(&SortList,(struct jbNode *)ss);

   return(TRUE);

}

void FreeSortList(void)
{
	struct StatsNode *ss;

   for(ss=(struct StatsNode *)SortList.First;ss;ss=ss->Next)
	{
		if(ss->Tagname) osFree(ss->Tagname);
		if(ss->Desc) osFree(ss->Desc);
	}

	jbFreeList(&SortList);
}

int CompareAreas(const void *a1,const void *a2)
{
   struct StatsNode **s1,**s2;

   s1=(struct StatsNode **)a1;
   s2=(struct StatsNode **)a2;

	if((*s1)->Group < (*s2)->Group) return(-1);
	if((*s1)->Group > (*s2)->Group) return(1);

   return(stricmp((*s1)->Tagname,(*s2)->Tagname));
}                                                           

void SortSortList(void)
{
   ulong nc;
   struct StatsNode *ss,**buf,**work;

   nc=0;

   for(ss=(struct StatsNode *)SortList.First;ss;ss=ss->Next)
      nc++;

   if(nc==0)
      return;

   if(!(buf=(struct StatsNode **)osAlloc(nc*sizeof(struct StatsNode *))))
   {
      nomem=TRUE;
      return;
   }

   work=buf;

   for(ss=(struct StatsNode *)SortList.First;ss;ss=ss->Next)
      *work++=ss;

   qsort(buf,nc,ptrsize,CompareAreas);

   jbNewList(&SortList);

   for(work=buf;nc--;)
      jbAddNode(&SortList,(struct jbNode *)*work++);

   osFree(buf);
}

long CalculateWeekKB(struct Area *area)
{
   if(area->FirstTime == 0 || DayStatsWritten == 0)
   {
      return(-1);
   }
   else
   {
      int days,c;
      unsigned long sum;

      days=DayStatsWritten-area->FirstTime / (24*60*60);
      if(days > 7) days=7;

      sum=0;

      for(c=1;c<days+1;c++)
         sum+=area->Last8Days[c];

      if(sum == 0 && area->Texts!=0)
      {
         days=DayStatsWritten-area->FirstTime / (24*60*60);
         if(days==0) days=1;
         return(area->Texts/days);
      }
      else
      {
         if(days == 0) days=1;
         return(sum/days);
      }
   }
}

bool AddForwardList(struct Arealist *arealist)
{
   bool res;
   osFile fh;
   uchar buf[200];
   uchar desc[100];
   ulong c,d;
   struct Area *area;
   struct StatsNode *ss;
   
   if(!(fh=osOpen(arealist->AreaFile,MODE_OLDFILE)))
   {
		ulong err=osError();
      LogWrite(1,SYSTEMERR,"AreaFix: File %s not found",arealist->AreaFile);
		LogWrite(1,SYSTEMERR,"AreaFix: Error: %s",osErrorMsg(err));
      return(TRUE);
   }

   while(osFGets(fh,buf,199))
   {
      desc[0]=0;

      for(c=0;buf[c]>32;c++);

      if(buf[c]!=0)
      {
         buf[c]=0;

         c++;
         while(buf[c]<=32 && buf[c]!=0) c++;

         if(buf[c]!=0)
         {
            d=0;
            while(buf[c]!=0 && buf[c]!=10 && buf[c]!=13 && d<77) desc[d++]=buf[c++];
            desc[d]=0;
         }
      }

      if(buf[0]!=0)
      {
         /* Don't add areas that exist locally */

         for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
            if(stricmp(buf,area->Tagname)==0) break;

         for(ss=(struct StatsNode *)SortList.First;ss;ss=ss->Next)
            if(stricmp(buf,ss->Tagname)==0) break;

         if(!area && !ss)
         {
            if(arealist->Flags & AREALIST_DESC)
               res=AddSortList(buf,desc,arealist->Group,FALSE,FALSE,-1);

            else
               res=AddSortList(buf,"",arealist->Group,FALSE,FALSE,-1);

            if(!res)
            {
               osClose(fh);
               return(FALSE);
            }
         }
      }
   }
   
   osClose(fh);
   return(TRUE);
}

void AddCommandReply(struct afReply *afr,uchar *cmd,uchar *reply)
{
	if(strlen(cmd) <= 30)
	{
   	afAddLine(afr,"%-30s %s",cmd,reply);
	}
	else
	{
   	afAddLine(afr,"%s",cmd);
   	afAddLine(afr,"%-30s %s","",reply);
	}	
}

void rawSendList(short type,struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode)
{
   uchar buf[50];
   struct TossNode *tn;
   struct Area *area;
   struct StatsNode *ss,*lastss;
   struct Arealist *arealist;
   short sendlisttotal,sendlistlinked;
   uchar ast;
   struct afReply *afr;
   /* Log action */

   switch(type)
   {
      case SENDLIST_QUERY:
         LogWrite(4,AREAFIX,"AreaFix: Sending query to %u:%u/%u.%u",
            cnode->Node.Zone,
            cnode->Node.Net,
            cnode->Node.Node,
            cnode->Node.Point);

         break;

      case SENDLIST_UNLINKED:
         LogWrite(4,AREAFIX,"AreaFix: Sending list of unlinked areas to %u:%u/%u.%u",
            cnode->Node.Zone,
            cnode->Node.Net,
            cnode->Node.Node,
            cnode->Node.Point);

         break;

      case SENDLIST_FULL:
         LogWrite(4,AREAFIX,"AreaFix: Sending list of areas to %u:%u/%u.%u",
            cnode->Node.Zone,
            cnode->Node.Net,
            cnode->Node.Node,
            cnode->Node.Point);

         break;
   }

   /* Start building reply message */

   if(!(afr=afInitReply(config.cfg_Sysop,from4d,toname,&cnode->Node,"AreaFix list of areas")))
		return;

   switch(type)
   {
      case SENDLIST_QUERY:
         afAddLine(afr,"This is a list of all connected areas at %u:%u/%u.%u:",
            from4d->Zone,
            from4d->Net,
            from4d->Node,
            from4d->Point);
         break;

      case SENDLIST_FULL:
         afAddLine(afr,"This is a list of all available areas at %u:%u/%u.%u:",
            from4d->Zone,
            from4d->Net,
            from4d->Node,
            from4d->Point);
         break;

      case SENDLIST_UNLINKED:
         afAddLine(afr,"This is a list of all unlinked areas at %u:%u/%u.%u:",
            from4d->Zone,
            from4d->Net,
            from4d->Node,
            from4d->Point);
         break;
   }

   afAddLine(afr,"");

   /* Init list */

	jbNewList(&SortList);

   /* Add local areas */

   for(area=(struct Area *)config.AreaList.First;area;area=area->Next)
      if(area->AreaType == AREATYPE_ECHOMAIL)
      {
         short add;
         bool attached,feed;
         
         for(tn=(struct TossNode *)area->TossNodes.First;tn;tn=tn->Next)
            if(tn->ConfigNode == cnode) break;

         add=FALSE;

         switch(type)
         {
            case SENDLIST_QUERY:
               if(tn) add=TRUE;
               break;

            case SENDLIST_UNLINKED:
               if(!tn && (CheckFlags(area->Group,cnode->Groups) || CheckFlags(area->Group,cnode->ReadOnlyGroups))) add=TRUE;
               break;

            case SENDLIST_FULL:
               if(tn || (CheckFlags(area->Group,cnode->Groups) || CheckFlags(area->Group,cnode->ReadOnlyGroups))) add=TRUE;
               break;
         }

         if(add)
         {
            attached=FALSE;
            feed=FALSE;
            
            if(tn) attached=TRUE;
            if(tn && tn->Flags & TOSSNODE_FEED) feed=TRUE;

            if(!AddSortList(area->Tagname,area->Description,area->Group,attached,feed,CalculateWeekKB(area)))
            {
               LogWrite(1,SYSTEMERR,"AreaFix: Out of memory when building list of areas");
               afAddLine(afr,"Failed to build list of areas, out of memory");
               afSendMessage(afr);
					afFreeReply(afr);
               FreeSortList();
               return;
            }
         }
      }

   /* Add forward-requestable areas */

   if(config.cfg_Flags & CFG_INCLUDEFORWARD && (type == SENDLIST_UNLINKED || type == SENDLIST_FULL) && cnode->Flags & NODE_FORWARDREQ)
   {
      for(arealist=(struct Arealist *)config.ArealistList.First;arealist;arealist=arealist->Next)
         if((arealist->Flags & AREALIST_FORWARD) && CheckFlags(arealist->Group,cnode->Groups))
         {
            if(!AddForwardList(arealist))
            {
               LogWrite(1,SYSTEMERR,"AreaFix: Out of memory when building list of areas");
               afAddLine(afr,"Failed to build list of areas, out of memory");
               afSendMessage(afr);
					afFreeReply(afr);
               FreeSortList();
               return;
            }
         }
   }

   /* Generate list */

   SortSortList();

   lastss=NULL;

   sendlisttotal=0;
   sendlistlinked=0;

   for(ss=(struct StatsNode *)SortList.First;ss;ss=ss->Next)
   {
      if(!lastss || lastss->Group!=ss->Group)
      {
         if(lastss)
            afAddLine(afr,"");

         if(ss->Group) afAddLine(afr," Group: %s",config.cfg_GroupNames[ss->Group-'A']);
         else          afAddLine(afr," Group: %s","<No group>");

         afAddLine(afr,"");

         afAddLine(afr," Tagname                       Description                         KB/week");
         afAddLine(afr," ----------------------------  ---------------------------------   -------");
      }

      ast=' ';

      if(type == SENDLIST_FULL && ss->Attached)
      {
         ast='*';
         sendlistlinked++;
      }

      if(ss->Feed)
         ast='%';

      sendlisttotal++;

      if(ss->WeekKB == -1)
         strcpy(buf,"?");

      else
         sprintf(buf,"%ld",ss->WeekKB);

      if(strlen(ss->Tagname)<=28)
      {
         afAddLine(afr,"%lc%-28.28s  %-33.33s  %8.8s",ast,ss->Tagname,ss->Desc,buf);
      }
      else
      {
         afAddLine(afr,"%lc%-70.70s",ast,ss->Tagname);
         afAddLine(afr,"%lc%-28.28s  %-33.33s  %8.8s",' ',"",ss->Desc,buf);
      }
      lastss=ss;
   }

   switch(type)
   {
      case SENDLIST_QUERY:
         afAddLine(afr,"\x0d%lu linked areas.",sendlisttotal);
         afAddLine(afr,"A '%%' means that you are the feed for the area.");
         break;

      case SENDLIST_UNLINKED:
         afAddLine(afr,"\x0d%lu unlinked areas.",sendlisttotal);
         break;

      case SENDLIST_FULL:
         afAddLine(afr,"\x0dTotally %lu areas, you are connected to %lu of them.",sendlisttotal,sendlistlinked);
         afAddLine(afr,"A '*' means that you are connected to the area.");
         afAddLine(afr,"A '%%' means that you are the feed for the area.");
         break;
   }

   afSendMessage(afr);
	afFreeReply(afr);
   FreeSortList();
}

void rawSendHelp(struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode)
{
   uchar helpbuf[100];
   osFile fh;
	struct afReply *afr;
	
   LogWrite(4,AREAFIX,"AreaFix: Sending help file to %u:%u/%u.%u",
      cnode->Node.Zone,
      cnode->Node.Net,
      cnode->Node.Node,
      cnode->Node.Point);

   if(!(afr=afInitReply(config.cfg_Sysop,from4d,toname,&cnode->Node,"AreaFix help")))
		return;

   if(!(fh=osOpen(config.cfg_AreaFixHelp,MODE_OLDFILE)))
   {
		ulong err=osError();
      LogWrite(1,SYSTEMERR,"AreaFix: Unable to open %s",config.cfg_AreaFixHelp);
		LogWrite(1,SYSTEMERR,"AreaFix: Error: %s",osErrorMsg(err));
      afAddLine(afr,"*** Error *** : Couldn't open help file");
   }
   else
   {
      while(osFGets(fh,helpbuf,100) && !nomem)
      {
         if(helpbuf[0]!=0)
            helpbuf[strlen(helpbuf)-1]=0;

         afAddLine(afr,"%s",helpbuf);
      }

      osClose(fh);
   }

   afSendMessage(afr);
	afFreeReply(afr);
}

void rawSendInfo(struct Node4D *from4d,uchar *toname,struct ConfigNode *cnode)
{
   int c;
	struct afReply *afr;
	
   LogWrite(4,AREAFIX,"AreaFix: Sending configuration info to %u:%u/%u.%u",
      cnode->Node.Zone,
      cnode->Node.Net,
      cnode->Node.Node,
      cnode->Node.Point);

   if(!(afr=afInitReply(config.cfg_Sysop,from4d,toname,&cnode->Node,"AreaFix configuration info")))
		return;

   afAddLine(afr,"Configuration for %u:%u/%u.%u:",
      cnode->Node.Zone,
      cnode->Node.Net,
      cnode->Node.Node,
      cnode->Node.Point);

   afAddLine(afr,"");

   afAddLine(afr,"           Sysop: %s",cnode->SysopName);
   afAddLine(afr,"Packet  password: %s",cnode->PacketPW);
   afAddLine(afr,"Areafix password: %s",cnode->AreafixPW);

   if(!cnode->Packer)
   {
      afAddLine(afr,"          Packer: No packer");
   }
   else
   {
      afAddLine(afr,"          Packer: %s",cnode->Packer->Name);
   }

   afAddLine(afr,"");

   if(cnode->Flags & NODE_PASSIVE)
      afAddLine(afr," * You are passive and will not receive any echomail messages");

   if(cnode->Flags & NODE_TINYSEENBY)
      afAddLine(afr," * You receive messages with tiny SEEN-BY lines");

   if(cnode->Flags & NODE_NOSEENBY)
      afAddLine(afr," * You receive messages without SEEN-BY lines");

   if(cnode->Flags & NODE_FORWARDREQ)
      afAddLine(afr," * You may do forward-requests");

   if(cnode->Flags & NODE_NOTIFY)
      afAddLine(afr," * You will receive notifications");

   if(cnode->Flags & NODE_PACKNETMAIL)
      afAddLine(afr," * Netmail to you will be packed");

   if(cnode->Flags & NODE_AUTOADD)
      afAddLine(afr," * New areas from you will be auto-added");

   afAddLine(afr,"");
   afAddLine(afr,"You have full access to these groups:");
   afAddLine(afr,"");

   for(c='A';c<='Z';c++)
      if(CheckFlags(c,cnode->Groups) && !CheckFlags(c,cnode->ReadOnlyGroups))
      {
         if(config.cfg_GroupNames[c-'A'][0]!=0)
            afAddLine(afr,"%lc: %s",c,config.cfg_GroupNames[c-'A']);

         else
            afAddLine(afr,"%lc",c);
      }

   afAddLine(afr,"");
   afAddLine(afr,"You have read-only access to these groups:");
   afAddLine(afr,"");

   for(c='A';c<='Z';c++)
      if(CheckFlags(c,cnode->ReadOnlyGroups))
      {
         if(config.cfg_GroupNames[c-'A'])
            afAddLine(afr,"%lc: %s",c,config.cfg_GroupNames[c-'A']);

         else
            afAddLine(afr,"%lc",c);
      }

   afSendMessage(afr);
	afFreeReply(afr);
}


void afRawPrepareMessage(void);

struct afReply *afInitReply(uchar *fromname,struct Node4D *from4d,uchar *toname,struct Node4D *to4d,uchar *subject)
{
	struct afReply *afr;
	
	if(!(afr=osAllocCleared(sizeof(struct afReply))))
	{
		nomem=TRUE;
		return(NULL);
	}
	
	if(!(afr->mm=mmAlloc()))
	{
		nomem=TRUE;
		osFree(afr);
		return(NULL);
	}
		
   strcpy(afr->mm->From,fromname);
   Copy4D(&afr->mm->OrigNode,from4d);
   strcpy(afr->mm->To,toname);
   Copy4D(&afr->mm->DestNode,to4d);
   strcpy(afr->subject,subject);
	afr->mm->Attr = FLAG_PVT;

	MakeFidoDate(time(NULL),afr->mm->DateTime);  
	afr->mm->Flags |= MMFLAG_AUTOGEN;
	MakeNetmailKludges(afr->mm);

   afr->lines=0;
   afr->part=1;

	return(afr);
}

void afFreeReply(struct afReply *afr)
{
	mmFree(afr->mm);
}

void afAddLine(struct afReply *afr,uchar *fmt,...)
{
   va_list args;
   uchar buf[200];

   if(afr->lines >= config.cfg_AreaFixMaxLines-2 && config.cfg_AreaFixMaxLines!=0)
   {
      strcpy(buf,"\x0d(Continued in next message)\x0d");
		mmAddLine(afr->mm,buf);

      sprintf(afr->mm->Subject,"%s (part %ld)",afr->subject,afr->part);
      afSendMessage(afr);
      jbFreeList(&afr->mm->TextChunks);

		MakeFidoDate(time(NULL),afr->mm->DateTime);  
	 	afr->mm->Flags |= MMFLAG_AUTOGEN;
		MakeNetmailKludges(afr->mm);

      strcpy(buf,"(Continued from previous message)\x0d\x0d");
      mmAddLine(afr->mm,buf);

      afr->lines=2;
      afr->part++;
   }

   va_start(args, fmt);
   vsprintf(buf,fmt,args);
   va_end(args);

   strcat(buf,"\x0d");
   mmAddLine(afr->mm,buf);

   afr->lines++;
}

void afSendMessage(struct afReply *afr)
{
   if(afr->part != 1) 
		sprintf(afr->mm->Subject,"%s (part %ld)",afr->subject,afr->part);

	else					
		strcpy(afr->mm->Subject,afr->subject);
		
   HandleMessage(afr->mm);
}


void RemoteAreafixAdd(uchar *area,struct ConfigNode *node)
{
   struct RemoteAFCommand *cmd;

   if(!(cmd=(struct RemoteAFCommand *)osAllocCleared(sizeof(struct RemoteAFCommand))))
   {
      nomem=TRUE;
      return;
   }

	if(node->Flags & NODE_AFNEEDSPLUS)
	   sprintf(cmd->Command,"+%.78s",area);

	else
	   sprintf(cmd->Command,"%.78s",area);

   cmd->Command[70]=0;
   jbAddNode(&node->RemoteAFList,(struct jbNode *)cmd);
}

void RemoteAreafixRemove(uchar *area,struct ConfigNode *node)
{
   struct RemoteAFCommand *cmd;

   if(!(cmd=(struct RemoteAFCommand *)osAllocCleared(sizeof(struct RemoteAFCommand))))
   {
      nomem=TRUE;
      return;
   }

   sprintf(cmd->Command,"-%.78s",area);

   cmd->Command[70]=0;
   jbAddNode(&node->RemoteAFList,(struct jbNode *)cmd);
}

void SendRemoteAreafix(void)
{
   struct Route *tmproute;
   struct ConfigNode *node;
   struct RemoteAFCommand *cmd;
   uchar buf[200];
   struct MemMessage *mm;

   for(node=(struct ConfigNode *)config.CNodeList.First;node;node=node->Next)
      if(node->RemoteAFList.First)
      {
			if(!(mm=mmAlloc()))
			{
				nomem=TRUE;
				return;
			}

         for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
            if(Compare4DPat(&tmproute->Pattern,&node->Node)==0) break;

         Copy4D(&mm->DestNode,&node->Node);
         Copy4D(&mm->OrigNode,&tmproute->Aka->Node);

         strcpy(mm->From,config.cfg_Sysop);
         strcpy(mm->To,node->RemoteAFName);
         strcpy(mm->Subject,node->RemoteAFPw);

         mm->Attr = FLAG_PVT;

 			MakeFidoDate(time(NULL),mm->DateTime);  
 
   		mm->Flags |= MMFLAG_AUTOGEN;

   		MakeNetmailKludges(mm);

		   if(config.cfg_Flags & CFG_ADDTID)
      		AddTID(mm);

         for(cmd=(struct RemoteAFCommand *)node->RemoteAFList.First;cmd;cmd=cmd->Next)
         {
            sprintf(buf,"%s\x0d",cmd->Command);
            mmAddLine(mm,buf);
         }

         mmAddLine(mm,"---\x0dGenerated by CrashMail "VERSION"\x0d");

         HandleMessage(mm);

         mmFree(mm);

         jbFreeList(&node->RemoteAFList);
      }
}

bool CheckFlags(uchar group,uchar *node)
{
   uchar c;

   for(c=0;c<strlen(node);c++)
   {
      if(group==node[c])
         return(TRUE);
    }

   return(FALSE);
}

struct Arealist *FindForward(uchar *tagname,uchar *flags)
{
   struct Arealist *arealist;
   uchar buf[200];
   ulong c;
   osFile fh;

   for(arealist=(struct Arealist *)config.ArealistList.First;arealist;arealist=arealist->Next)
   {
      if((arealist->Flags & AREALIST_FORWARD) && CheckFlags(arealist->Group,flags))
      {
         if((fh=osOpen(arealist->AreaFile,MODE_OLDFILE)))
         {
            while(osFGets(fh,buf,199))
            {
               for(c=0;buf[c]>32;c++);
               buf[c]=0;

               if(stricmp(buf,tagname)==0)
               {
                  osClose(fh);
                  return(arealist);
               }
            }
            osClose(fh);
         }
			else
			{
				ulong err=osError();
				LogWrite(1,SYSTEMERR,"Failed to open file %s",arealist->AreaFile);
				LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
			}
      }
   }
   return(NULL);
}

void DoSendAFList(short type,struct ConfigNode *cnode)
{
   struct Route *tmproute;

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,&cnode->Node)==0) break;

   if(!tmproute)
   {
      LogWrite(1,TOSSINGERR,"No route found for %d:%d/%d.%d",
         cnode->Node.Zone,
         cnode->Node.Net,
         cnode->Node.Node,
         cnode->Node.Point);

      return;
   }

   switch(type)
   {
      case SENDLIST_FULL:
         rawSendList(SENDLIST_FULL,&tmproute->Aka->Node,cnode->SysopName,cnode);
         break;

      case SENDLIST_QUERY:
         rawSendList(SENDLIST_QUERY,&tmproute->Aka->Node,cnode->SysopName,cnode);
         break;

      case SENDLIST_UNLINKED:
         rawSendList(SENDLIST_UNLINKED,&tmproute->Aka->Node,cnode->SysopName,cnode);
         break;

      case SENDLIST_INFO:
         rawSendInfo(&tmproute->Aka->Node,cnode->SysopName,cnode);
         break;

      case SENDLIST_HELP:
         rawSendHelp(&tmproute->Aka->Node,cnode->SysopName,cnode);
         break;
   }
}

