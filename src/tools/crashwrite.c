#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include <shared/parseargs.h>
#include <shared/jbstrcpy.h>
#include <shared/path.h>
#include <shared/mystrncpy.h>

#include <oslib/os.h>
#include <oslib/osfile.h>
#include <oslib/osmisc.h>

#include <shared/fidonet.h>

#define VERSION "1.0"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#ifdef PLATFORM_AMIGA
uchar *ver="$VER: CrashWrite "VERSION" ("__COMMODORE_DATE__")";
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

struct argument args[] =
   { { ARGTYPE_STRING, "FROMNAME",     0,                 NULL },
     { ARGTYPE_STRING, "FROMADDR",     0,                 NULL },
     { ARGTYPE_STRING, "TONAME",       0,                 NULL },
     { ARGTYPE_STRING, "TOADDR",       0,                 NULL },
     { ARGTYPE_STRING, "SUBJECT",      0,                 NULL },
     { ARGTYPE_STRING, "AREA",         0,                 NULL },
     { ARGTYPE_STRING, "ORIGIN",       0,                 NULL },
     { ARGTYPE_STRING, "DIR",          ARGFLAG_MANDATORY, NULL },
     { ARGTYPE_STRING, "TEXT",         0,                 NULL },
	  { ARGTYPE_BOOL,   "NOMSGID",      0,                 NULL },
	  { ARGTYPE_BOOL,   "FILEATTACH",   0,                 NULL },
     { ARGTYPE_END,     NULL,          0,                 0    } };

struct Node4D
{
   ushort Zone,Net,Node,Point;
};

uchar PktMsgHeader[SIZE_PKTMSGHEADER];
uchar PktHeader[SIZE_PKTHEADER];

bool nomem,diskfull;

ushort getuword(uchar *buf,ulong offset)
{
   return (ushort)(buf[offset]+256*buf[offset+1]);
}

void putuword(uchar *buf,ulong offset,ushort num)
{
   buf[offset]=num%256;
   buf[offset+1]=num/256;
}

bool Parse4D(uchar *buf, struct Node4D *node)
{
   ulong c=0,val=0;
   bool GotZone=FALSE,GotNet=FALSE,GotNode=FALSE;

   node->Zone=0;
   node->Net=0;
   node->Node=0;
   node->Point=0;

	for(c=0;c<strlen(buf);c++)
	{
		if(buf[c]==':')
		{
         if(GotZone || GotNet || GotNode) return(FALSE);
			node->Zone=val;
         GotZone=TRUE;
			val=0;
	   }
		else if(buf[c]=='/')
		{
         if(GotNet || GotNode) return(FALSE);
         node->Net=val;
         GotNet=TRUE;
			val=0;
		}
		else if(buf[c]=='.')
		{
         if(GotNode) return(FALSE);
         node->Node=val;
         GotNode=TRUE;
			val=0;
		}
		else if(buf[c]>='0' && buf[c]<='9')
		{
         val*=10;
         val+=buf[c]-'0';
		}
		else return(FALSE);
	}
   if(GotZone && !GotNet)  node->Net=val;
   else if(GotNode)        node->Point=val;
   else                    node->Node=val;

   return(TRUE);
}

void MakeFidoDate(time_t tim,uchar *dest)
{
   struct tm *tp;
   time_t t;
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

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

void WriteNull(osFile ofh,uchar *str)
{
   osWrite(ofh,str,(ulong)(strlen(str)+1));
}

int main(int argc, char **argv)
{
   struct Node4D from4d,to4d;
   osFile ifh,ofh;
	time_t t;
	struct tm *tp;
	ulong pktnum,c,serial;
	ushort attr;
	uchar fromname[36],toname[36],subject[72],datetime[20],origin[80];
	uchar pktname[30],fullname[200],readbuf[100];
	
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

   if(argc == 2 && strcmp(argv[1],"?")==0)
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
      if(!(Parse4D((uchar *)args[ARG_FROMADDR].data,&from4d)))
      {
         printf("Invalid address \"%s\"\n",(uchar *)args[ARG_FROMADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

   if(args[ARG_TOADDR].data)
   {
      if(!(Parse4D((uchar *)args[ARG_TOADDR].data,&to4d)))
      {
         printf("Invalid address \"%s\"\n",(uchar *)args[ARG_FROMADDR].data);
			osEnd();
			exit(OS_EXIT_ERROR);
      }
	}

   time(&t);
   tp=localtime(&t);

	/* Create packet header */

   putuword(PktHeader,PKTHEADER_ORIGNODE,from4d.Node);
   putuword(PktHeader,PKTHEADER_DESTNODE,to4d.Node);
   putuword(PktHeader,PKTHEADER_DAY,tp->tm_mday);
   putuword(PktHeader,PKTHEADER_MONTH,tp->tm_mon);
   putuword(PktHeader,PKTHEADER_YEAR,tp->tm_year+1900);
   putuword(PktHeader,PKTHEADER_HOUR,tp->tm_hour);
   putuword(PktHeader,PKTHEADER_MINUTE,tp->tm_min);
   putuword(PktHeader,PKTHEADER_SECOND,tp->tm_sec);
   putuword(PktHeader,PKTHEADER_BAUD,0);
   putuword(PktHeader,PKTHEADER_PKTTYPE,2);
   putuword(PktHeader,PKTHEADER_ORIGNET,from4d.Net);
   putuword(PktHeader,PKTHEADER_DESTNET,to4d.Net);
   PktHeader[PKTHEADER_PRODCODELOW]=0xfe;
   PktHeader[PKTHEADER_REVMAJOR]=VERSION_MAJOR;
   putuword(PktHeader,PKTHEADER_QORIGZONE,from4d.Zone);
   putuword(PktHeader,PKTHEADER_QDESTZONE,to4d.Zone);
   putuword(PktHeader,PKTHEADER_AUXNET,0);
   putuword(PktHeader,PKTHEADER_CWVALIDCOPY,0x0100);
   PktHeader[PKTHEADER_PRODCODEHIGH]=0;
   PktHeader[PKTHEADER_REVMINOR]=VERSION_MINOR;
   putuword(PktHeader,PKTHEADER_CAPABILWORD,0x0001);
   putuword(PktHeader,PKTHEADER_ORIGZONE,from4d.Zone);
   putuword(PktHeader,PKTHEADER_DESTZONE,to4d.Zone);
   putuword(PktHeader,PKTHEADER_ORIGPOINT,from4d.Point);
   putuword(PktHeader,PKTHEADER_DESTPOINT,to4d.Point);
   PktHeader[PKTHEADER_PRODDATA]=0;
   PktHeader[PKTHEADER_PRODDATA+1]=0;
   PktHeader[PKTHEADER_PRODDATA+2]=0;
   PktHeader[PKTHEADER_PRODDATA+3]=0;

   for(c=0;c<8;c++)
      PktHeader[PKTHEADER_PASSWORD+c]=0;

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
	
	if(args[ARG_FROMNAME].data) mystrncpy(fromname,(uchar *)args[ARG_FROMNAME].data,36);
	if(args[ARG_TONAME].data)   mystrncpy(toname,(uchar *)args[ARG_TONAME].data,36);
	if(args[ARG_SUBJECT].data)  mystrncpy(subject,(uchar *)args[ARG_SUBJECT].data,72);
	if(args[ARG_ORIGIN].data)   mystrncpy(origin,(uchar *)args[ARG_ORIGIN].data,80);

	MakeFidoDate(t,datetime);

	/* Create pkt file */

	serial=0;

   do
   {
      t=time(NULL);
      pktnum = (t<<8) + serial;
		serial++;
      sprintf(pktname,"%08lx.pkt",pktnum);
      MakeFullPath(args[ARG_DIR].data,pktname,fullname,200);
   } while(osExists(fullname));

   if(!(ofh=osOpen(fullname,MODE_NEWFILE)))
   {
      printf("Unable to create packet %s",fullname);
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
         osFPrintf(ofh,"\x01" "FMPT %ld\x0d",from4d.Point);

      if(to4d.Point)
         osFPrintf(ofh,"\x01" "TOPT %ld\x0d",to4d.Point);

      osFPrintf(ofh,"\x01" "INTL %lu:%lu/%lu %lu:%lu/%lu\x0d",to4d.Zone,to4d.Net,to4d.Node,
																			  from4d.Zone,from4d.Net,from4d.Node);
   }

   if(!args[ARG_NOMSGID].data)
   {
      osFPrintf(ofh,"\x01" "MSGID: %u:%u/%u.%u %08lx\x0d",
         from4d.Zone,from4d.Net,from4d.Node,from4d.Point,pktnum);
	}

	if(args[ARG_TEXT].data)
	{
		printf("Appending %s...\n",(uchar *)args[ARG_TEXT].data);
		
	   if(!(ifh=osOpen((uchar *)args[ARG_TEXT].data,MODE_OLDFILE)))
   	{
      	printf("Unable to open \"%s\" for reading\n",(uchar *)args[ARG_TEXT].data);
			osClose(ofh);
			osDelete(fullname);
			exit(OS_EXIT_ERROR);
      }

      while(osFGets(ifh,readbuf,100))
      {
         if(readbuf[0]!=0)
            readbuf[strlen(readbuf)-1]=0;

			osFPrintf(ofh,"%s\x0d",readbuf);
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
