#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#include <oslib/os.h>
#include <oslib/osfile.h>
#include <oslib/osmisc.h>

#include <shared/types.h>
#include <shared/jbstrcpy.h>
#include <shared/parseargs.h>
#include <shared/node4d.h>

#define VERSION "1.0"

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashExport " VERSION " " __AMIGADATE__;
#endif

bool diskfull;

uchar cfgbuf[4000];

#define AREATYPE_NETMAIL	1
#define AREATYPE_ECHOMAIL  2
#define AREATYPE_DEFAULT   3
#define AREATYPE_BAD       4
#define AREATYPE_LOCAL     5

uchar tagname[100],desc[100],msgbase[10],path[100],export[1000],aka[50];
uchar group,areatype;
bool unconfirmed;

#define ARG_PREFSFILE    0
#define ARG_OUTFILE      1
#define ARG_FORMAT       2
#define ARG_GROUP  	    3

struct argument args[] =
   { { ARGTYPE_STRING, "PREFSFILE", ARGFLAG_AUTO | ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "OUTFILE",   ARGFLAG_AUTO | ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "FORMAT",    ARGFLAG_AUTO | ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "GROUP",     0,                                NULL },
     { ARGTYPE_END,     NULL,       0,                                0    } };

#define FORMAT_AREASBBS 			0
#define FORMAT_FORWARD				1
#define FORMAT_FORWARDNODESC		2
#define FORMAT_GOLDED				3
#define FORMAT_TIMED             4

int format;

bool CheckFlags(uchar group,uchar *node)
{
   int c;

   for(c=0;c<strlen(node);c++)
   {
      if(toupper(group)==toupper(node[c]))
         return(TRUE);
    }

   return(FALSE);
}

void writearea(osFile fh)
{
	int c;
   uchar escdesc[100];

	uchar *gedmsgbase,*gedtype,*gedflags;
   uchar gedgroupbuf[10];

   uchar *timflags,*timkeyword;

	/* Never write unconfirmed areas to output file */
	
	if(unconfirmed)
		return;

	/* Never write default areas to output file */

   if(areatype == AREATYPE_DEFAULT)
      return;

	/* Only write if in the right group */

   if(args[ARG_GROUP].data && !CheckFlags(group,args[ARG_GROUP].data))
		return;

   /* Escape description */

   strcpy(escdesc,desc);
         
   for(c=0;escdesc[c];c++) /* Desc can't contain " */
      if(escdesc[c] == '\"') escdesc[c]='\'';

	switch(format)
	{
		case FORMAT_AREASBBS:
			if(areatype != AREATYPE_NETMAIL && areatype != AREATYPE_LOCAL) /* Don't write netmail areas */
			{
	         if(stricmp(msgbase,"MSG")==0)      osFPrintf(fh,"%s %s %s\n",path,tagname,export);            
            else if(stricmp(msgbase,"JAM")==0) osFPrintf(fh,"!%s %s %s\n",path,tagname,export);
	         else if(stricmp(msgbase,"")==0)    osFPrintf(fh,"#%s %s %s\n",tagname,tagname,export);
	         else                               osFPrintf(fh,"%s:%s %s %s\n",msgbase,path,tagname,export);
			}
			break;
			
		case FORMAT_FORWARD: /* Don't write netmail, local or BAD areas */
			if(areatype == AREATYPE_ECHOMAIL)
			{
				if(desc[0]) osFPrintf(fh,"%s %s\n",tagname,desc);
				else 			osFPrintf(fh,"%s\n",tagname);
			}
			break;
			
		case FORMAT_FORWARDNODESC: /* Don't write netmail or BAD areas */
			if(areatype == AREATYPE_ECHOMAIL)
			{
				osFPrintf(fh,"%s\n",tagname);
			}
			break;

		case FORMAT_GOLDED:
			if(path[0]) /* Don't write pass-through areas */
			{
				if(stricmp(msgbase,"MSG")==0)      gedmsgbase="FTS1";
				else if(stricmp(msgbase,"JAM")==0) gedmsgbase="JAM";
				else return;

				if(areatype == AREATYPE_NETMAIL)    gedtype="NET";
				else if(areatype == AREATYPE_LOCAL) gedtype="LOCAL";
				else			   							gedtype="ECHO";
	
				if(areatype == AREATYPE_NETMAIL)    gedflags="(Loc Pvt)";
				else			                        gedflags="(Loc)";

				if(group) sprintf(gedgroupbuf,"%c",group);
				else		 strcpy(gedgroupbuf,"0");
			
            osFPrintf(fh,"AREADEF %s \"%s\" %s %s %s %s %s %s\n",tagname,escdesc,gedgroupbuf,gedtype,gedmsgbase,path,aka,gedflags);
         }
         break;

      case FORMAT_TIMED:
			if(path[0]) /* Don't write pass-through areas */
			{
            if(stricmp(msgbase,"MSG")==0)      timflags="";
            else if(stricmp(msgbase,"JAM")==0) timflags=" -J";
				else return;

            if(areatype == AREATYPE_NETMAIL)     timkeyword="NetArea";
            else if(areatype == AREATYPE_LOCAL)  timkeyword="LocalArea";
            else           							 timkeyword="EchoArea";
	
            osFPrintf(fh,"%s \"%s\" %s %s -P%s%s\n",timkeyword,escdesc,tagname,path,aka,timflags);
			}
         break;

	}
}

int main(int argc, char **argv)
{
   osFile ifh,ofh;
   uchar sysopname[100],cfgword[30],buf[100];
   uint32_t jbcpos;
   time_t t;

   if(!osInit())
      exit(OS_EXIT_ERROR);

   if(argc > 1 &&
	  (strcmp(argv[1],"?")==0      ||
		strcmp(argv[1],"-h")==0     ||
		strcmp(argv[1],"--help")==0 ||
		strcmp(argv[1],"help")==0 ||
		strcmp(argv[1],"/h")==0     ||
		strcmp(argv[1],"/?")==0 ))
   {
      printargs(args);
      osEnd();
      exit(OS_EXIT_OK);
   }

   if(!parseargs(args,argc,argv))
   {
      osEnd();
      exit(OS_EXIT_ERROR);
   }
   	
	if(stricmp((uchar *)args[ARG_FORMAT].data,"areasbbs")==0)
	{
		format=FORMAT_AREASBBS;
	}
	else if(stricmp((uchar *)args[ARG_FORMAT].data,"forward")==0)
	{
		format=FORMAT_FORWARD;
	}
	else if(stricmp((uchar *)args[ARG_FORMAT].data,"forwardnodesc")==0)
	{
		format=FORMAT_FORWARDNODESC;
	}
	else if(stricmp((uchar *)args[ARG_FORMAT].data,"golded")==0)
	{
		format=FORMAT_GOLDED;
	}
   else if(stricmp((uchar *)args[ARG_FORMAT].data,"timed")==0)
	{
      format=FORMAT_TIMED;
	}
	else
   {
      printf("Unknown format \"%s\"\n",(uchar *)args[ARG_FORMAT].data);
      osEnd();
      exit(OS_EXIT_ERROR);
   }
	
	if(!(ifh=osOpen(args[ARG_PREFSFILE].data,MODE_OLDFILE)))
   {
		uint32_t err=osError();
      printf("Failed to open %s for reading\n",(char *)args[ARG_PREFSFILE].data);
		printf("Error: %s",osErrorMsg(err));		
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   if(!(ofh=osOpen(args[ARG_OUTFILE].data,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      printf("Failed to open %s for writing\n",(char *)args[ARG_OUTFILE].data);
		printf("Error: %s",osErrorMsg(err));		
      osClose(ifh);
      osEnd();
      exit(OS_EXIT_ERROR);
   }

   time(&t);
   osFPrintf(ofh,"; Generated by CrashExport %s\n; %s\n",VERSION,ctime(&t));

	if(format == FORMAT_AREASBBS)
	{
	   /* Get default origin and sysop name for areas.bbs */

	   strcpy(sysopname,"Sysop");

	   while(osFGets(ifh,cfgbuf,4000))
   	{
      	jbcpos=0;
	      jbstrcpy(cfgword,cfgbuf,30,&jbcpos);
	
   	   if(stricmp(cfgword,"SYSOP")==0)
      	   jbstrcpy(sysopname,cfgbuf,100,&jbcpos);
	   }

	   osFPrintf(ofh,"%s ! %s\n","Default origin",sysopname);

	   osSeek(ifh,0,OFFSET_BEGINNING);
	}
	
   while(osFGets(ifh,cfgbuf,4000))
   {
      jbcpos=0;
      jbstrcpy(cfgword,cfgbuf,30,&jbcpos);

      if(stricmp(cfgword,"AREA")==0 || stricmp(cfgword,"NETMAIL")==0 || stricmp(cfgword,"LOCALAREA")==0)
      {
			if(tagname[0])
            writearea(ofh);

			group=0;
			unconfirmed=FALSE;
									
			export[0]=0;
			desc[0]=0;
	
         jbstrcpy(tagname,cfgbuf,100,&jbcpos);
         jbstrcpy(aka,cfgbuf,50,&jbcpos);
         jbstrcpy(msgbase,cfgbuf,10,&jbcpos);
         jbstrcpy(path,cfgbuf,100,&jbcpos);

			if(stricmp(cfgword,"NETMAIL")==0)
				areatype=AREATYPE_NETMAIL;
				
			else if(stricmp(cfgword,"LOCALAREA")==0)
				areatype=AREATYPE_LOCAL;
	
			else if(stricmp(tagname,"BAD")==0)
				areatype=AREATYPE_BAD;
			
			else if(stricmp(tagname,"DEFAULT")==0 || strnicmp(tagname,"DEFAULT_",8)==0)
				areatype=AREATYPE_DEFAULT;
			
			else
				areatype=AREATYPE_ECHOMAIL;				
      }

      if(stricmp(cfgword,"EXPORT")==0)
      {
			struct Node4D tpl4d,tmp4d;
			
			tpl4d.Zone=0;
			tpl4d.Net=0;
			tpl4d.Node=0;
			tpl4d.Point=0;

         while(jbstrcpy(buf,cfgbuf,100,&jbcpos))
         {
            if(buf[0]=='!' || buf[0]=='%' || buf[0]=='@')
               strcpy(buf,&buf[1]);

            if(Parse4DTemplate(buf,&tmp4d,&tpl4d))
            {
					Copy4D(&tpl4d,&tmp4d);
					tpl4d.Point=0;
				
					Print4D(&tmp4d,buf);
					
					if(strlen(export) < sizeof(export)-20)
					{
						if(export[0]) strcat(export," ");
      		      strcat(export,buf);
					}
				}
         }
      }   

      if(stricmp(cfgword,"UNCONFIRMED")==0)
      {
			unconfirmed=TRUE;
      }   

      if(stricmp(cfgword,"DESCRIPTION")==0)
      {
         jbstrcpy(desc,cfgbuf,100,&jbcpos);
      }   

      if(stricmp(cfgword,"GROUP")==0)
      {
         if(jbstrcpy(buf,cfgbuf,100,&jbcpos))
            group=buf[0];
      }   
   }

   if(tagname[0])
      writearea(ofh);

   osClose(ofh);
   osClose(ifh);
   osEnd();
   
   exit(OS_EXIT_OK);
}

