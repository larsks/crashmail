#include "crashmail.h"

struct jbList ArcList;

bool doAddFlow(uchar *filename,uchar *basename,uchar type,long mode);

bool LockBasename(uchar *basename)
{
	uchar buf[200];
	osFile fp;

	strcpy(buf,basename);
	strcat(buf,".bsy");

	if(osExists(buf))
		return(FALSE);
	
	if(!(fp=osOpen(buf,MODE_NEWFILE)))
	{
		uint32_t err=osError();
		LogWrite(1,SYSTEMERR,"Failed to create busy file %s\n",buf);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));		
		return(FALSE);
	}
	
	osClose(fp);

	return(TRUE);
}

void UnlockBasename(uchar *basename)
{
	uchar buf[200];
	
	strcpy(buf,basename);
	strcat(buf,".bsy");
	
	osDelete(buf);
}

void MakeBaseName(struct Node4D *n4d,uchar *basename)
{
   struct Aka *firstaka;
   struct Route *tmproute;
   bool samedomain;
   uchar *ospathchars;
   uint32_t num,c;
   uchar buf[50];

   ospathchars=OS_PATH_CHARS;

   for(tmproute=(struct Route *)config.RouteList.First;tmproute;tmproute=tmproute->Next)
      if(Compare4DPat(&tmproute->Pattern,n4d)==0) break;

   firstaka=(struct Aka *)config.AkaList.First;

   samedomain=FALSE;

   if(!tmproute)
      samedomain=TRUE;

   else if(tmproute->Aka->Domain[0]==0 || firstaka->Domain[0]==0 || stricmp(tmproute->Aka->Domain,firstaka->Domain)==0)
      samedomain=TRUE;

   if(samedomain)
   {
      /* Main domain */

      strcpy(basename,config.cfg_Outbound);

      if(basename[0])
      {
         if(strchr(ospathchars,basename[strlen(basename)-1]))
            basename[strlen(basename)-1]=0; /* Strip / */
      }

      if(n4d->Zone != firstaka->Node.Zone)
      {
         /* Not in main zone */

         num=n4d->Zone;

			if(!(config.cfg_Flags & CFG_NOMAXOUTBOUNDZONE))
			{
         	if(num > 0xfff)
					num=0xfff;
			}

         sprintf(buf,".%03lx",num);
         strcat(basename,buf);
      }
   }
   else
   {
      /* Other domain */

      strcpy(basename,config.cfg_Outbound);

      if(basename[0])
      {
         if(strchr(ospathchars,basename[strlen(basename)-1]))
            basename[strlen(basename)-1]=0; /* Strip / */
      }

      *GetFilePart(basename)=0; /* Use domain as last component in path */
      strcat(basename,tmproute->Aka->Domain);

      num=n4d->Zone;

		if(!(config.cfg_Flags & CFG_NOMAXOUTBOUNDZONE))
		{
         if(num > 0xfff)
				num=0xfff;
		}

      sprintf(buf,".%03lx",num);
      strcat(basename,buf);
   }

   if(!osExists(basename))
      osMkDir(basename);

   /* Add slash */

   c=strlen(basename);
   basename[c++]=ospathchars[0];
   basename[c++]=0;

   /* Add net/node */

   sprintf(buf,"%04x%04x",n4d->Net,n4d->Node);
   strcat(basename,buf);

   if(n4d->Point)
   {
      strcat(basename,".pnt");

      if(!osExists(basename))
         osMkDir(basename);

      /* Add slash */

      c=strlen(basename);
      basename[c++]=ospathchars[0];
      basename[c++]=0;

      /* Add point */

      sprintf(buf,"%08x",n4d->Point);
      strcat(basename,buf);
   }
}


void WriteIndex(void)
{
	osFile fh;
	uchar buf[200];
	struct ConfigNode *cnode;

	MakeFullPath(config.cfg_PacketDir,"cmindex",buf,200);

	/* Get basenum */

	if(!(fh=osOpen(buf,MODE_NEWFILE)))
		return;

	for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
		if(cnode->LastArcName[0])
		{
			Print4D(&cnode->Node,buf);
			osFPrintf(fh,"%s %s\n",buf,cnode->LastArcName);
		}

	osClose(fh);
}

void ReadIndex(void)
{
	osFile fh;
	uchar buf[200],buf2[200];
	uint32_t jbcpos;
	struct ConfigNode *cnode,*c1,*c2;
	struct Node4D n4d;

	MakeFullPath(config.cfg_PacketDir,"cmindex",buf,200);

	/* Get basenum */

	if(!(fh=osOpen(buf,MODE_OLDFILE)))
		return;

	while(osFGets(fh,buf,200))
	{
		striptrail(buf);

		jbcpos=0;

		jbstrcpy(buf2,buf,200,&jbcpos);

		if(Parse4D(buf2,&n4d))
		{
			jbstrcpy(buf2,buf,200,&jbcpos);

			for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
				if(Compare4D(&cnode->Node,&n4d)==0) mystrncpy(cnode->LastArcName,buf2,13);
		}
	}

	osClose(fh);

	/* Check for duplicates */

   for(c1=(struct ConfigNode *)config.CNodeList.First;c1;c1=c1->Next)
      for(c2=c1->Next;c2;c2=c2->Next)
         if(c1->LastArcName[0] && hextodec(c1->LastArcName) == hextodec(c2->LastArcName))
         {
				LogWrite(1,TOSSINGINFO,"Warning: The same bundle name is used for %u:%u/%u.%u and %u:%u/%u.%u",
					c1->Node.Zone,
					c1->Node.Net,
					c1->Node.Node,
					c1->Node.Point,
					c2->Node.Zone,
					c2->Node.Net,
					c2->Node.Node,
					c2->Node.Point);

				LogWrite(1,TOSSINGINFO,"Cleared bundle name for %u:%u/%u.%u",
					c2->Node.Zone,
					c2->Node.Net,
					c2->Node.Node,
					c2->Node.Point);

				c2->LastArcName[0]=0;
				WriteIndex();
         }
}

bool ExistsBasenum(uint32_t num)
{
	uchar name[20];
	struct osFileEntry *fe;
	struct ConfigNode *cnode;

	sprintf(name,"%08lx.",num);

	for(fe=(struct osFileEntry *)ArcList.First;fe;fe=fe->Next)
		if(IsArc(fe->Name) && hextodec(fe->Name) == num) return(TRUE);

	for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
		if(cnode->LastArcName[0] && hextodec(cnode->LastArcName) == num) return(TRUE);

	return(FALSE);
}

bool ExistsBundle(uint32_t basenum,uint32_t num)
{
	uchar name[20];
	struct osFileEntry *fe;
	uchar *daynames[]={"su","mo","tu","we","th","fr","sa"};
	
	sprintf(name,"%08lx.%s%ld",basenum,daynames[num/10],num%10);

	for(fe=(struct osFileEntry *)ArcList.First;fe;fe=fe->Next)
		if(stricmp(fe->Name,name)==0) return(TRUE);
		
	return(FALSE);
}

void MakeArcName(struct ConfigNode *cnode,uchar *dest)
{
	struct osFileEntry *fe,*foundfe;
	uchar ext[10];
	uint32_t basenum;
	long suffix,newsuffix,day,i;
	uchar *daynames[]={"su","mo","tu","we","th","fr","sa"};
	time_t t;
	struct tm *tp;

	time(&t);
	tp=localtime(&t);

	day=tp->tm_wday;

	/* Get basenum and suffix of latest bundle */

   suffix=-1;

   if(!cnode->LastArcName[0])
	{
		basenum=time(NULL);

		while(ExistsBasenum(basenum))
			basenum++;
   }
	else
	{
		basenum=hextodec(cnode->LastArcName);

      strncpy(ext,&cnode->LastArcName[strlen(cnode->LastArcName)-3],3);
      ext[2]=0;

		for(i=0;i<7;i++)
		{
         if(stricmp(ext,daynames[i])==0)
			{
				suffix=i*10;
				suffix+=cnode->LastArcName[strlen(cnode->LastArcName)-1]-'0';
			}
		}
   }

	/* Does LastArcName still exist in directory? */

	foundfe=NULL;

	if(cnode->LastArcName[0])
	{
	   for(fe=(struct osFileEntry *)ArcList.First;fe;fe=fe->Next)
   	   if(stricmp(cnode->LastArcName,fe->Name)==0) foundfe=fe;
	}

	if(suffix == -1)
	{
      if((config.cfg_Flags & CFG_WEEKDAYNAMING))
			newsuffix=day*10;

		else
			newsuffix=0;
	}
	else
	{
		newsuffix=suffix;

      if(!foundfe)
      {
         newsuffix=-1;
      }
      else
      {
         if(foundfe->Size == 0)
	   		newsuffix=-1;

   		if(foundfe->Size > config.cfg_MaxBundleSize)
	   		newsuffix=-1;
      }

		if((config.cfg_Flags & CFG_WEEKDAYNAMING) && suffix/10 != day)
			newsuffix=-1;

		if(newsuffix == -1)
		{
			newsuffix=suffix+1;
			if(newsuffix == 70) newsuffix=0;

         if((config.cfg_Flags & CFG_WEEKDAYNAMING) && newsuffix/10 != day)
				newsuffix=day*10;

			if(ExistsBundle(basenum,newsuffix))
				newsuffix=suffix;
		}
	}

	sprintf(dest,"%08lx.%s%ld",basenum,daynames[newsuffix/10],newsuffix%10);

	if(stricmp(cnode->LastArcName,dest)!=0)
	{
		mystrncpy(cnode->LastArcName,dest,13);
		WriteIndex();
	}
}

void DeleteZero(uchar *dir,struct jbList *arclist)
{
   struct osFileEntry *fe,*fe2;
   uchar buf[200];

	/* Delete zero length bundles for this node */

   fe=(struct osFileEntry *)arclist->First;

	while(fe)
	{
		fe2=fe->Next;

      if(fe->Size == 0)
      {
         MakeFullPath(dir,fe->Name,buf,200);

         LogWrite(2,TOSSINGINFO,"Deleting zero length bundle %s",buf);

         osDelete(buf);
			jbFreeNode(&ArcList,(struct jbNode *)fe);
		}

		fe=fe2;
	}
}

void HandleOrphan(uchar *name)
{
   osFile fh;
   uchar buf[200],buf2[200];
   char type;
   bool mode;
   uint32_t jbcpos;
   struct Node4D n4d;
	uchar basename[200];

   if(!(fh=osOpen(name,MODE_OLDFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to open orphan file \"%s\"",name);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return;
   }

   if(!osFGets(fh,buf,100))
   {
      LogWrite(1,SYSTEMERR,"Orphan file \"%s\" contains no information",name);
      osClose(fh);
      return;
   }

   osClose(fh);

   jbcpos=0;

   jbstrcpy(buf2,buf,100,&jbcpos);

   if(stricmp(buf2,"Normal")==0)
      type=PKTS_NORMAL;

   else if(stricmp(buf2,"Hold")==0)
      type=PKTS_HOLD;

   else if(stricmp(buf2,"Direct")==0)
      type=PKTS_DIRECT;

   else if(stricmp(buf2,"Crash")==0)
      type=PKTS_CRASH;

   else
   {
      LogWrite(1,SYSTEMERR,"Unknown flavour \"%s\" in \"%s\"",buf2,name);
      return;
   }

   jbstrcpy(buf2,buf,100,&jbcpos);

   if(!Parse4D(buf2,&n4d))
   {
      LogWrite(1,SYSTEMERR,"Invalid node \"%s\" in \"%s\"",buf2,name);
      return;
   }

   mode=FLOW_NONE;

   jbstrcpy(buf2,buf,100,&jbcpos);

   if(stricmp(buf2,"Truncate")==0)
      mode=FLOW_TRUNC;

   if(stricmp(buf2,"Delete")==0)
      mode=FLOW_DELETE;

   mystrncpy(buf,name,200);
   buf[strlen(buf)-7]=0; /* Remove .orphan */

   MakeBaseName(&n4d,basename);

	if(!LockBasename(basename))
	{
		printf("Cannot add to %s, node is busy...\n",GetFilePart(basename));
      return;
	}

   if(doAddFlow(buf,basename,type,mode))
   	osDelete(name); /* Orphan file no longer needed */

	UnlockBasename(basename);
}

void MakeOrphan(uchar *file,struct Node4D *n4d,char type,long mode)
{
   uchar buf[200];
   osFile fh;

   strcpy(buf,file);
   strcat(buf,".orphan");

   if(!(fh=osOpen(buf,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to open \"%s\", cannot make .orphan file",buf);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return;
   }

   sprintf(buf,"%s %d:%d/%d.%d",prinames[(int)type],n4d->Zone,n4d->Net,n4d->Node,n4d->Point);
   if(mode==FLOW_TRUNC) strcat(buf," Truncate");
   else if(mode==FLOW_DELETE) strcat(buf," Delete");

   strcat(buf,"\n");

   osPuts(fh,buf);
   osClose(fh);
}

/* Only call if file is already locked */
/* MakeOrphan() should be called if necessary */
bool doAddFlow(uchar *filename,uchar *basename,uchar type,long mode)
{
   uchar buf[200],letter,*prefix;
   osFile fh;

   switch(type)
   {
      case PKTS_NORMAL:
      case PKTS_ECHOMAIL: letter='f';
                          break;
      case PKTS_HOLD:     letter='h';
                          break;
      case PKTS_DIRECT:   letter='d';
                          break;
      case PKTS_CRASH:    letter='c';
                          break;
      default:            letter='f';
   }

   sprintf(buf,"%s.%clo",basename,letter);

   if(!(fh=osOpen(buf,MODE_READWRITE)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to open \"%s\"",buf);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   while(osFGets(fh,buf,200))
   {
      striptrail(buf);

      if(buf[0]=='#') strcpy(buf,&buf[1]);
      if(buf[0]=='~') strcpy(buf,&buf[1]);
      if(buf[0]=='^') strcpy(buf,&buf[1]);
      if(buf[0]=='-') strcpy(buf,&buf[1]);

      if(stricmp(buf,filename)==0)
      {
         osClose(fh);
         return(TRUE); /* Was already in flow file */
      }
   }

   osSeek(fh,0,OFFSET_END);

   prefix="";

   if(mode == FLOW_TRUNC)
      prefix="#";

   if(mode == FLOW_DELETE)
      prefix="^";

   if(config.cfg_Flags & CFG_FLOWCRLF) osFPrintf(fh,"%s%s\r\n",prefix,filename);
   else                         osFPrintf(fh,"%s%s\n",prefix,filename);

   osClose(fh);

   return(TRUE);
}

/* Handles locking and MakeOrphan() */
bool AddFlow(uchar *filename,struct Node4D *n4d,uchar type,long mode)
{
	uchar basename[200];

   MakeBaseName(n4d,basename);

	if(!LockBasename(basename))
	{
		printf("Cannot add to %s, node is busy...\n",GetFilePart(basename));
		MakeOrphan(filename,n4d,type,mode);
		return(FALSE);
	}

	if(!doAddFlow(filename,basename,type,mode))
		MakeOrphan(filename,n4d,type,mode);

	UnlockBasename(basename);

	return(TRUE);
}

bool MakePktTmp(uchar *name)
{
	uchar buf[200];

	MakeFullPath(config.cfg_PacketDir,GetFilePart(name),buf,200);
   strcpy(&buf[strlen(buf)-6],"pkttmp"); /* Change suffix */

   if(!movefile(name,buf))
	{
   	LogWrite(1,SYSTEMERR,"Failed to move file \"%s\" to \"%s\"",name,buf);
		return(FALSE);
	}

	return(TRUE);
}

void UpdateFile(uchar *name)
{
	struct osFileEntry *newfe,*fe;

	if(!(newfe=osGetFileEntry(name)))
		return;

	for(fe=(struct osFileEntry *)ArcList.First;fe;fe=fe->Next)
		if(stricmp(fe->Name,name)==0) break;

	if(fe)
	{
		fe->Date=newfe->Date;
		fe->Size=newfe->Size;
		osFree(newfe);
	}
	else
	{
		jbAddNode(&ArcList,(struct jbNode *)newfe);
	}
}

#define COPYBUFSIZE 5000

bool PackFile(char *file)
{
   uchar basename[200],arcname[200],pktname[200],buf[200],buf2[200],*copybuf;
   uint32_t jbcpos,readlen;
   int c,res;
   struct Node4D n4d;
   char type;
	uchar letter;
	osFile ifh,ofh;

   /* Parse filename */

   mystrncpy(buf,GetFilePart(file),200);

   for(c=0;buf[c];c++)
      if(buf[c]=='_') buf[c]=' ';

   jbcpos=0;

   jbstrcpy(buf2,buf,100,&jbcpos);

   jbstrcpy(buf2,buf,100,&jbcpos);

   if(stricmp(buf2,"Normal")==0)
      type=PKTS_NORMAL;

   else if(stricmp(buf2,"Hold")==0)
      type=PKTS_HOLD;

   else if(stricmp(buf2,"Direct")==0)
      type=PKTS_DIRECT;

   else if(stricmp(buf2,"Crash")==0)
      type=PKTS_CRASH;

   else if(stricmp(buf2,"Echomail")==0)
      type=PKTS_ECHOMAIL;

   else
   {
      LogWrite(1,TOSSINGERR,"Unknown flavour \"%s\" for  \"%s\"",buf2,file);
      return(FALSE);
   }

   jbstrcpy(buf2,buf,100,&jbcpos);
   n4d.Zone=atol(buf2);

   jbstrcpy(buf2,buf,100,&jbcpos);
   n4d.Net=atol(buf2);

   jbstrcpy(buf2,buf,100,&jbcpos);
   n4d.Node=atol(buf2);

   jbstrcpy(buf2,buf,100,&jbcpos);
   n4d.Point=atol(buf2);

   /* Make basename for this node */

   MakeBaseName(&n4d,basename);

	if(!LockBasename(basename))
   {
      LogWrite(1,TOSSINGINFO,"Cannot add \"%s\" to outbound, node is busy...",GetFilePart(file));
      return(FALSE);
   }

   /* Handle echomail packet */

   if(type == PKTS_ECHOMAIL)
   {
	   struct ConfigNode *cnode;

		for(cnode=(struct ConfigNode *)config.CNodeList.First;cnode;cnode=cnode->Next)
			if(Compare4D(&cnode->Node,&n4d)==0) break;

      if(cnode && cnode->Packer)
      {
         /* Pack echomail */

			MakeArcName(cnode,buf);
			MakeFullPath(config.cfg_PacketDir,buf,arcname,200);

         mystrncpy(pktname,file,200);
         GetFilePart(pktname)[8]=0;
         strcat(pktname,".pkt");

			LogWrite(4,TOSSINGINFO,"Packing %s for %d:%d/%d.%d with %s",
				GetFilePart(pktname),
				cnode->Node.Zone,
				cnode->Node.Net,
				cnode->Node.Node,
				cnode->Node.Point,
				cnode->Packer->Name);

         osRename(file,pktname);

         if(config.cfg_BeforePack[0])
         {
            ExpandPacker(config.cfg_BeforePack,buf,200,arcname,pktname);
            res=osExecute(buf);

            if(res != 0)
            {
	            osRename(pktname,file);
               LogWrite(1,SYSTEMERR,"BEFOREPACK command failed: %lu",res);
	   			UnlockBasename(basename);
   				return(FALSE);
            }
         }

         ExpandPacker(cnode->Packer->Packer,buf,200,arcname,pktname);
         res=osExecute(buf);

         if(res == 0)
         {
				UpdateFile(arcname);

            osDelete(pktname);

            if(!doAddFlow(arcname,basename,cnode->EchomailPri,FLOW_DELETE))
               MakeOrphan(arcname,&n4d,cnode->EchomailPri,FLOW_DELETE);
         }
         else
         {
	         osRename(pktname,file);
            LogWrite(1,SYSTEMERR,"Packer failed: %lu",res);
				UnlockBasename(basename);
				return(FALSE);
         }
      }
      else
      {
         /* Send unpacked echomail */

         MakeFullPath(config.cfg_PacketDir,GetFilePart(file),pktname,200);
         GetFilePart(pktname)[8]=0;
         strcat(pktname,".pkt");

			LogWrite(4,TOSSINGINFO,"Sending %s unpacked to %d:%d/%d.%d",
				GetFilePart(pktname),
				cnode->Node.Zone,
				cnode->Node.Net,
				cnode->Node.Node,
				cnode->Node.Point);

         if(!movefile(file,pktname))
         {
            LogWrite(1,SYSTEMERR,"Failed to move file \"%s\" to \"%s\"",file,pktname);
				UnlockBasename(basename);
				return(FALSE);
         }
         else
         {
            if(!doAddFlow(pktname,basename,cnode->EchomailPri,FLOW_DELETE))
               MakeOrphan(pktname,&n4d,cnode->EchomailPri,FLOW_DELETE);
         }
      }
   }
   else
   {
      /* Netmail */          

	   switch(type)
   	{
      	case PKTS_NORMAL:
      	case PKTS_ECHOMAIL: letter='o';
         	                 break;
      	case PKTS_HOLD:     letter='h';
            	              break;
      	case PKTS_DIRECT:   letter='d';
         	                 break;
	      case PKTS_CRASH:    letter='c';
   	                       break;
      	default:            letter='f';
		}
		
		sprintf(buf2,".%cut",letter);
		strcpy(buf,basename);
		strcat(buf,buf2);

		LogWrite(4,TOSSINGINFO,"Sending unpacked netmail to %d:%d/%d.%d (%s)",
			n4d.Zone,
			n4d.Net,
			n4d.Node,
			n4d.Point,
			prinames[(int)type]);

		if(!(copybuf=(uchar *)osAlloc(COPYBUFSIZE)))
		{
			nomem=TRUE;
			UnlockBasename(basename);
			return(FALSE);
		}
		
		if(!(ifh=osOpen(file,MODE_OLDFILE)))
		{
			uint32_t err=osError();
			LogWrite(1,SYSTEMERR,"Failed to open \"%s\"",file);
			LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));		
			osFree(copybuf);
			UnlockBasename(basename);
			return(FALSE);
		}
		
		if(osExists(buf))
		{
			if(!(ofh=osOpen(buf,MODE_READWRITE)))
			{
				uint32_t err=osError();
				LogWrite(1,SYSTEMERR,"Failed to open \"%s\"",file);
				LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));		
				osClose(ifh);
				osFree(copybuf);
				UnlockBasename(basename);
				return(FALSE);
			}

			osSeek(ifh,SIZE_PKTHEADER,OFFSET_BEGINNING);
			osSeek(ofh,-2,OFFSET_END);
		}
		else
		{
			if(!(ofh=osOpen(buf,MODE_NEWFILE)))
			{
				uint32_t err=osError();
				LogWrite(1,SYSTEMERR,"Failed to open \"%s\"",file);
				LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));		
				osClose(ifh);
				osFree(copybuf);
				UnlockBasename(basename);
				return(FALSE);
			}
		}

		while((readlen=osRead(ifh,copybuf,COPYBUFSIZE)))
		{
			if(!osWrite(ofh,copybuf,readlen))
				{ ioerror=TRUE; ioerrornum=osError(); }
		}
				
		osClose(ifh);
		osClose(ofh);
		osFree(copybuf);

		osDelete(file);
   }

	UnlockBasename(basename);
	
	if(ioerror)
		return(FALSE);
	
	return(TRUE);
}

bool ArchiveOutbound(void)
{
   struct jbList PktList;
   struct osFileEntry *fe;
   uchar buf[200];

	/* Orphan files */

   LogWrite(3,ACTIONINFO,"Scanning for orphan files");

   if(!(osReadDir(config.cfg_PacketDir,&ArcList,IsOrphan)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_PacketDir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   SortFEList(&ArcList);

   for(fe=(struct osFileEntry *)ArcList.First;fe && !ctrlc;fe=fe->Next)
   {
      LogWrite(1,SYSTEMINFO,"Found orphan file \"%s\", retrying...",fe->Name);

      MakeFullPath(config.cfg_PacketDir,fe->Name,buf,200);
      HandleOrphan(buf);
   }

   jbFreeList(&ArcList);

	/* Read ArcList */

   if(!(osReadDir(config.cfg_PacketDir,&ArcList,IsArc)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_PacketDir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      return(FALSE);
   }

   /* Delete old zero-length files */

   DeleteZero(config.cfg_PacketDir,&ArcList);

   /* Read index */

	ReadIndex();

	/* Old packets */

   LogWrite(3,ACTIONINFO,"Scanning for old packets");

   if(!(osReadDir(config.cfg_PacketDir,&PktList,IsPktTmp)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_PacketDir);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      jbFreeList(&ArcList);
      return(FALSE);
   }

   SortFEList(&PktList);

   for(fe=(struct osFileEntry *)PktList.First;fe;fe=fe->Next)
   {
      LogWrite(1,SYSTEMINFO,"Found old packet file \"%s\", retrying...",fe->Name);

      MakeFullPath(config.cfg_PacketDir,fe->Name,buf,200);
      PackFile(buf);
   }

   jbFreeList(&PktList);

	/* New packets */

   LogWrite(3,ACTIONINFO,"Scanning for new files to pack");

   if(!(osReadDir(config.cfg_PacketCreate,&PktList,IsNewPkt)))
   {
		uint32_t err=osError();
      LogWrite(1,SYSTEMERR,"Failed to read directory \"%s\"",config.cfg_PacketCreate);
		LogWrite(1,SYSTEMERR,"Error: %s",osErrorMsg(err));
      jbFreeList(&ArcList);
      return(FALSE);
   }

   SortFEList(&PktList);

   for(fe=(struct osFileEntry *)PktList.First;fe;fe=fe->Next)
   {
      MakeFullPath(config.cfg_PacketCreate,fe->Name,buf,200);

      if(!PackFile(buf))
			if(!MakePktTmp(buf))
			{
      		jbFreeList(&PktList);
      		jbFreeList(&ArcList);
		      return(FALSE);
			}
   }

   jbFreeList(&PktList);
   jbFreeList(&ArcList);

   return(TRUE);
}

