#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <shared/parseargs.h>
#include <shared/jbstrcpy.h>
#include <shared/path.h>
#include <shared/mystrncpy.h>
#include <shared/node4d.h>

#include <oslib/os.h>
#include <oslib/osfile.h>
#include <oslib/osmisc.h>

#include <shared/fidonet.h>

#define VERSION "1.1"

#define VERSION_MAJOR 1
#define VERSION_MINOR 1

#ifdef PLATFORM_AMIGA
char *ver="$VER: CrashWrite "VERSION" ("__COMMODORE_DATE__")";
#endif

#define DEFAULT_TONAME "All"
#define DEFAULT_FROMNAME "CrashWrite"
#define DEFAULT_SUBJECT "Information"
#define DEFAULT_ORIGIN "Another user of CrashMail II"

#define ARG_FROMNAME     0
#define ARG_FROMADDR     1
#define ARG_TONAME       2
#define ARG_TOADDR       3
#define ARG_SUBJECT      4
#define ARG_AREA         5
#define ARG_ORIGIN       6
#define ARG_DIR          7
#define ARG_TEXT         8
#define ARG_NOMSGID      9
#define ARG_FILEATTACH  10
#define ARG_PKTFROMADDR 11
#define ARG_PKTTOADDR   12
#define ARG_PASSWORD    13
#define ARG_FILENAME    14

struct argument args[] =
   { { ARGTYPE_STRING, "FROMNAME",      0,                 NULL },
     { ARGTYPE_STRING, "FROMADDR",      0,                 NULL },
     { ARGTYPE_STRING, "TONAME",        0,                 NULL },
     { ARGTYPE_STRING, "TOADDR",        0,                 NULL },
     { ARGTYPE_STRING, "SUBJECT",       0,                 NULL },
     { ARGTYPE_STRING, "AREA",          0,                 NULL },
     { ARGTYPE_STRING, "ORIGIN",        0,                 NULL },
     { ARGTYPE_STRING, "DIR",           ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "TEXT",          0,                 NULL },
	  { ARGTYPE_BOOL,   "NOMSGID",       0,                 NULL },
	  { ARGTYPE_BOOL,   "FILEATTACH",    0,                 NULL },
     { ARGTYPE_STRING, "PKTFROMADDR",   0,                 NULL },
     { ARGTYPE_STRING, "PKTTOADDR",     0,                 NULL },
     { ARGTYPE_STRING, "PASSWORD",      0,                 NULL },
     { ARGTYPE_STRING, "FILENAME",      0,                 NULL },
     { ARGTYPE_END,     NULL,           0,                 0    } };

char PktMsgHeader[SIZE_PKTMSGHEADER];
char PktHeader[SIZE_PKTHEADER];

bool nomem,diskfull;

uint16_t getuword(char *buf,uint32_t offset)
{
   return (uint16_t)(buf[offset]+256*buf[offset+1]);
}

void putuword(char *buf,uint32_t offset,uint16_t num)
{
   buf[offset]=num%256;
   buf[offset+1]=num/256;
}

void MakeFidoDate(time_t tim,char *dest)
{
   struct tm *tp;
   time_t t;
   char *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

   t=tim;
   tp=localtime(&t);

   sprintf(dest,"%02d %s %02d  %02d:%02d:%02d",
      tp->tm_mday,
      monthnames[tp->tm_mon],
      tp->tm_year % 100,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec);
}

void WriteNull(osFile ofh,char *str)
{
   osWrite(ofh,str,(uint32_t)(strlen(str)+1));
}

int main(int argc, char **argv)
{
   struct Node4D from4d,to4d,pktfrom4d,pktto4d;
   osFile ifh,ofh;
	time_t t;
	struct tm *tp;
	uint32_t pktnum,c,serial;
	uint16_t attr;
	char fromname[36],toname[36],subject[72],datetime[20],origin[80];
	char pktname[30],fullname[200],readbuf[100];
	int i;
	
	from4d.Zone=0;
	from4d.Net=0;
	from4d.Node=0;
	from4d.Point=0;
	
	to4d.Zone=0;
	to4d.Net=0;
	to4d.Node=0;
	to4d.Point=0;

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

   if(args[ARG_FROMADDR].data)
   {
      if(!(Parse4D((char *)args[ARG_FROMADDR].data,&from4d)))
      {
         printf("Invalid address \"%s\"\n",(char *)args[ARG_FROMADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

   if(args[ARG_TOADDR].data)
   {
      if(!(Parse4D((char *)args[ARG_TOADDR].data,&to4d)))
      {
         printf("Invalid address \"%s\"\n",(char *)args[ARG_TOADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

	Copy4D(&pktfrom4d,&from4d);
	Copy4D(&pktto4d,&to4d);

   if(args[ARG_PKTFROMADDR].data)
   {
      if(!(Parse4D((char *)args[ARG_PKTFROMADDR].data,&pktfrom4d)))
      {
         printf("Invalid address \"%s\"\n",(char *)args[ARG_PKTFROMADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

   if(args[ARG_PKTTOADDR].data)
   {
      if(!(Parse4D((char *)args[ARG_PKTTOADDR].data,&pktto4d)))
      {
         printf("Invalid address \"%s\"\n",(char *)args[ARG_PKTTOADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

   time(&t);
   tp=localtime(&t);

	/* Create packet header */

   putuword(PktHeader,PKTHEADER_ORIGNODE,pktfrom4d.Node);
   putuword(PktHeader,PKTHEADER_DESTNODE,pktto4d.Node);
   putuword(PktHeader,PKTHEADER_DAY,tp->tm_mday);
   putuword(PktHeader,PKTHEADER_MONTH,tp->tm_mon);
   putuword(PktHeader,PKTHEADER_YEAR,tp->tm_year+1900);
   putuword(PktHeader,PKTHEADER_HOUR,tp->tm_hour);
   putuword(PktHeader,PKTHEADER_MINUTE,tp->tm_min);
   putuword(PktHeader,PKTHEADER_SECOND,tp->tm_sec);
   putuword(PktHeader,PKTHEADER_BAUD,0);
   putuword(PktHeader,PKTHEADER_PKTTYPE,2);
   putuword(PktHeader,PKTHEADER_ORIGNET,pktfrom4d.Net);
   putuword(PktHeader,PKTHEADER_DESTNET,pktto4d.Net);
   PktHeader[PKTHEADER_PRODCODELOW]=0xfe;
   PktHeader[PKTHEADER_REVMAJOR]=VERSION_MAJOR;
   putuword(PktHeader,PKTHEADER_QORIGZONE,pktfrom4d.Zone);
   putuword(PktHeader,PKTHEADER_QDESTZONE,pktto4d.Zone);
   putuword(PktHeader,PKTHEADER_AUXNET,0);
   putuword(PktHeader,PKTHEADER_CWVALIDCOPY,0x0100);
   PktHeader[PKTHEADER_PRODCODEHIGH]=0;
   PktHeader[PKTHEADER_REVMINOR]=VERSION_MINOR;
   putuword(PktHeader,PKTHEADER_CAPABILWORD,0x0001);
   putuword(PktHeader,PKTHEADER_ORIGZONE,pktfrom4d.Zone);
   putuword(PktHeader,PKTHEADER_DESTZONE,pktto4d.Zone);
   putuword(PktHeader,PKTHEADER_ORIGPOINT,pktfrom4d.Point);
   putuword(PktHeader,PKTHEADER_DESTPOINT,pktto4d.Point);
   PktHeader[PKTHEADER_PRODDATA]=0;
   PktHeader[PKTHEADER_PRODDATA+1]=0;
   PktHeader[PKTHEADER_PRODDATA+2]=0;
   PktHeader[PKTHEADER_PRODDATA+3]=0;

   for(c=0;c<8;c++)
      PktHeader[PKTHEADER_PASSWORD+c]=0;

   if(args[ARG_PASSWORD].data)
      strncpy(&PktHeader[PKTHEADER_PASSWORD],args[ARG_PASSWORD].data,8);

	/* Create message header */

	attr=0;

	if(!args[ARG_AREA].data)
		attr|=FLAG_PVT;
		
	if(args[ARG_FILEATTACH].data)
		attr|=FLAG_FILEATTACH;

   putuword(PktMsgHeader,PKTMSGHEADER_PKTTYPE,0x0002);
   putuword(PktMsgHeader,PKTMSGHEADER_ORIGNODE,from4d.Node);
   putuword(PktMsgHeader,PKTMSGHEADER_DESTNODE,to4d.Node);
   putuword(PktMsgHeader,PKTMSGHEADER_ORIGNET,from4d.Net);
   putuword(PktMsgHeader,PKTMSGHEADER_DESTNET,to4d.Net);
   putuword(PktMsgHeader,PKTMSGHEADER_ATTR,attr);
   putuword(PktMsgHeader,PKTMSGHEADER_COST,0);

	mystrncpy(fromname,DEFAULT_FROMNAME,36);
	mystrncpy(toname,DEFAULT_TONAME,36);
	mystrncpy(subject,DEFAULT_SUBJECT,72);
	mystrncpy(origin,DEFAULT_ORIGIN,80);
	
	if(args[ARG_FROMNAME].data) mystrncpy(fromname,(char *)args[ARG_FROMNAME].data,36);
	if(args[ARG_TONAME].data)   mystrncpy(toname,(char *)args[ARG_TONAME].data,36);
	if(args[ARG_SUBJECT].data)  mystrncpy(subject,(char *)args[ARG_SUBJECT].data,72);
	if(args[ARG_ORIGIN].data)   mystrncpy(origin,(char *)args[ARG_ORIGIN].data,80);

	MakeFidoDate(t,datetime);

	/* Create pkt file */

	serial=0;


	if (args[ARG_FILENAME].data) {
		MakeFullPath(args[ARG_DIR].data,args[ARG_FILENAME].data,fullname,200);
		t=time(NULL);
		pktnum = (t<<8) + serial;
	} else {
		do
		{
			t=time(NULL);
			pktnum = (t<<8) + serial;
			serial++;
			sprintf(pktname,"%08x.pkt",pktnum);
			MakeFullPath(args[ARG_DIR].data,pktname,fullname,200);
		} while(osExists(fullname));
	}

   if(!(ofh=osOpen(fullname,MODE_NEWFILE)))
   {
		uint32_t err=osError();
      printf("Unable to create packet %s\n",fullname);
		printf("Error: %s\n",osErrorMsg(err));		
		osEnd();
      exit(OS_EXIT_ERROR);
   }

   printf("Writing...\n");
   printf(" From: %-36s (%u:%u/%u.%u)\n",fromname,from4d.Zone,from4d.Net,from4d.Node,from4d.Point);
   printf("   To: %-36s (%u:%u/%u.%u)\n",toname,to4d.Zone,to4d.Net,to4d.Node,to4d.Point);
   printf(" Subj: %s\n",subject);
   printf(" Date: %s\n",datetime);

   osWrite(ofh,PktHeader,SIZE_PKTHEADER);
   osWrite(ofh,PktMsgHeader,SIZE_PKTMSGHEADER);

   WriteNull(ofh,datetime);
   WriteNull(ofh,toname);
   WriteNull(ofh,fromname);
   WriteNull(ofh,subject);

   if(args[ARG_AREA].data)
   {
		osFPrintf(ofh,"AREA:%s\x0d",args[ARG_AREA].data);
   }
   else
   {
      if(from4d.Point)
         osFPrintf(ofh,"\x01" "FMPT %d\x0d",from4d.Point);

      if(to4d.Point)
         osFPrintf(ofh,"\x01" "TOPT %d\x0d",to4d.Point);

      osFPrintf(ofh,"\x01" "INTL %u:%u/%u %u:%u/%u\x0d",to4d.Zone,to4d.Net,to4d.Node,
																			  from4d.Zone,from4d.Net,from4d.Node);
   }

   if(!args[ARG_NOMSGID].data)
   {
      osFPrintf(ofh,"\x01" "MSGID: %u:%u/%u.%u %08x\x0d",
         from4d.Zone,from4d.Net,from4d.Node,from4d.Point,pktnum);
	}

	if(args[ARG_TEXT].data)
	{
		printf("Appending %s...\n",(char *)args[ARG_TEXT].data);
		
	   if(!(ifh=osOpen((char *)args[ARG_TEXT].data,MODE_OLDFILE)))
   	{
			uint32_t err=osError();
      	printf("Unable to open \"%s\" for reading\n",(char *)args[ARG_TEXT].data);
			printf("Error: %s\n",osErrorMsg(err));		
			osClose(ofh);
			osDelete(fullname);
			exit(OS_EXIT_ERROR);
      }

      while(osFGets(ifh,readbuf,100))
      {
			for(i=0;readbuf[i];i++)
				if(readbuf[i] == '\n') readbuf[i]=0x0d;

         osFPrintf(ofh,"%s",readbuf);
      }

		osClose(ifh);
	}
	
   if(args[ARG_AREA].data)
   {
		osFPrintf(ofh,"--- CrashWrite II/" OS_PLATFORM_NAME " " VERSION "\x0d");
		osFPrintf(ofh," * Origin: %s (%u:%u/%u.%u)\x0d",origin,from4d.Zone,from4d.Net,from4d.Node,from4d.Point);
   }

   osWrite(ofh,"",1);
   osWrite(ofh,"",1);
   osWrite(ofh,"",1);

   osClose(ofh);

	osEnd();

   exit(OS_EXIT_OK);
}
