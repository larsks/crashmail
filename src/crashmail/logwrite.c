#include "crashmail.h"

#ifdef OS_HAS_SYSLOG
#include <syslog.h>
bool usesyslog;
#endif

osFile logfh;

bool OpenLogfile(uchar *logfile)
{
#ifdef OS_HAS_SYSLOG
	if(stricmp(logfile,"syslog")==0)
	{
		usesyslog=TRUE;
		openlog("CrashMail",0,LOG_USER);
		return(TRUE);
	}
#endif

   if(!(logfh=osOpen(logfile,MODE_READWRITE)))
      return(FALSE);

   osSeek(logfh,0,OFFSET_END);

   return(TRUE);
}

void CloseLogfile(void)
{
#ifdef OS_HAS_SYSLOG
	if(usesyslog)
	{
		closelog();
		usesyslog=FALSE;
		return;
	}
#endif

   osClose(logfh);
}

uchar *categoryletters="-%=!/D+^?";

void LogWrite(ulong level,ulong category,uchar *fmt,...)
{
   va_list args;
   time_t t;
   struct tm *tp;
   uchar *monthnames[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec","???"};

   if(level > config.cfg_LogLevel)
      return;

   if(level == 0)
      LogWrite(6,DEBUG,"*** Warning: Loglevel is 0!!! ***");

   if(fmt[0]==0)
   {
      printf("\n");
      return;
   }

#ifdef OS_HAS_SYSLOG
	if(usesyslog)
	{
		uchar buf[200];

	   va_start(args, fmt);
		vprintf(fmt,args);
	   printf("\n");
		vsprintf(buf,fmt,args);
		syslog(LOG_INFO,buf);
	   va_end(args);

		return;
	}
#endif

   va_start(args, fmt);

   vprintf(fmt,args);
   printf("\n");

   time(&t);
   tp=localtime(&t);

   fprintf(logfh,"%c %02d-%s-%02d %02d:%02d:%02d ",
      categoryletters[category],
      tp->tm_mday,
      monthnames[tp->tm_mon],
      tp->tm_year%100,
      tp->tm_hour,
      tp->tm_min,
      tp->tm_sec);

   vfprintf(logfh,fmt,args);
   fprintf(logfh,"\n");

   va_end(args);
}

