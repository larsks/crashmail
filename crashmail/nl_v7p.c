#include "crashmail.h"

/* Data records for V7+ nodelist. As far as I can understand, these should
   be stored on disk in native format. /JB */

#define V7P_NDXFILENAME "NODEX.ndx"
#define V7P_DATFILENAME "NODEX.dat"
#define V7P_DTPFILENAME "NODEX.dtp"
/* The Linux version of FastLst seems to use lower case for suffixes and
   upper case for the rest */

#define V7P_NDXBUFSIZE 512

struct v7p_ndxcontrol
{
   long indexstart;
   long rootstart;
   long lastblock;
   long firstleaf;
   long lastleaf;
   long freelist;
   uint16_t levels;
   uint16_t xor;
};

struct v7p_ndxindex
{
   long rectype;
   long prev;
   long next;
   short keycount;
   uint16_t keystart;
};

struct v7p_ndxindexkey
{
   uint16_t offset;
   uint16_t len;
   long value;
   long lower;
};

struct v7p_ndxleaf
{
   long rectype;
   long prev;
   long next;
   short keycount;
   uint16_t keystart;
};

struct v7p_ndxleafkey
{
   uint16_t offset;
   uint16_t len;
   long value;
};

struct v7p_datheader
{
   short Zone,Net,Node,HubNode;
   short CallCost,MsgFee,NodeFlags;

   uchar ModemType;
   uchar Phone_len;
   uchar Password_len;
   uchar Bname_len;
   uchar Sname_len;
   uchar Cname_len;
   uchar pack_len;
   uchar BaudRate;
};

osFile v7p_ndxfh;
osFile v7p_datfh;
osFile v7p_dtpfh;

uint16_t v7p_ndxrecsize;

struct v7p_ndxcontrol v7p_ndxcontrol;
struct v7p_datheader v7p_datheader;

uchar v7p_ndxbuf[V7P_NDXBUFSIZE];

int v7p_ndxcompare(uchar *d1,uchar *d2,int len)
{
   struct Node4D n1,n2;
   /* uchar b1[100],b2[100]; */

   if(len != 8 && len != 6)
      return(-1); /* Weird, but definitely not a match */

   memcpy(&n1,d1,len);
   memcpy(&n2,d2,sizeof(struct Node4D));

   if(len == 6)
      n1.Point=0;

   /*
   Print4D(&n1,b1);
   Print4D(&n2,b2);
   printf("compare %s and %s, %d\n",b1,b2,Compare4D(&n1,&n2));
   */

   return Compare4D(&n1,&n2);
}

bool v7p_findoffset(struct Node4D *node,uint32_t *offset)
{
   int i,res;
   long recnum;
   struct v7p_ndxindex *v7p_ndxindex;
   struct v7p_ndxindexkey *v7p_ndxindexkey;
   struct v7p_ndxleaf *v7p_ndxleaf;
   struct v7p_ndxleafkey *v7p_ndxleafkey;

   v7p_ndxindex=(struct v7p_ndxindex *)v7p_ndxbuf;
   v7p_ndxleaf=(struct v7p_ndxleaf *)v7p_ndxbuf;

   recnum=v7p_ndxcontrol.indexstart;

   osSeek(v7p_ndxfh,recnum*v7p_ndxrecsize,OFFSET_BEGINNING);

   if(osRead(v7p_ndxfh,v7p_ndxbuf,v7p_ndxrecsize) != v7p_ndxrecsize)
      return(FALSE);

   while(v7p_ndxindex->rectype != -1)
   {
      if(v7p_ndxindex->keycount < 0)
         return(FALSE);

      for(i=0;i < v7p_ndxindex->keycount;i++)
      {
         v7p_ndxindexkey=(struct v7p_ndxindexkey *)(v7p_ndxbuf+sizeof(struct v7p_ndxindex)+i*sizeof(struct v7p_ndxindexkey));

         if(v7p_ndxcompare(v7p_ndxbuf+v7p_ndxindexkey->offset,(uchar *)node,v7p_ndxindexkey->len) > 0)
            break;
      }

      if(i==0)
      {
         recnum=v7p_ndxindex->rectype;
      }
      else
      {
         v7p_ndxindexkey=(struct v7p_ndxindexkey *)(v7p_ndxbuf+sizeof(struct v7p_ndxindex)+(i-1)*sizeof(struct v7p_ndxindexkey));
         recnum=v7p_ndxindexkey->lower;
      }

      osSeek(v7p_ndxfh,recnum*v7p_ndxrecsize,OFFSET_BEGINNING);

      if(osRead(v7p_ndxfh,v7p_ndxbuf,v7p_ndxrecsize) != v7p_ndxrecsize)
         return(FALSE);
   }

   if(v7p_ndxleaf->keycount <= 0)
      return(FALSE);

   for(i=0;i < v7p_ndxleaf->keycount;i++)
   {
      v7p_ndxleafkey=(struct v7p_ndxleafkey *)(v7p_ndxbuf+sizeof(struct v7p_ndxleaf)+i*sizeof(struct v7p_ndxleafkey));

      res=v7p_ndxcompare(v7p_ndxbuf+v7p_ndxleafkey->offset,(uchar *)node,v7p_ndxleafkey->len);

      if(res > 0)
      {
         return(FALSE);
      }
      else if(res == 0)
      {
         *offset=v7p_ndxleafkey->value;
         break;
      }
   }

   if(i == v7p_ndxleaf->keycount)
      return(FALSE);

   return(TRUE);
}

uint32_t v7p_unpack(uchar *dest,uchar *pack,uint32_t size)
{
   uint32_t c,d;
   uint16_t w;
   uchar *table=" EANROSTILCHBDMUGPKYWFVJXZQ-'0123456789";

   d=0;

   for(c=0;c<size;c+=2)
   {
      w=pack[c]+pack[c+1]*256;

      dest[d+2]=table[w % 40];
      w/=40;

      dest[d+1]=table[w % 40];
      w/=40;

      dest[d]=table[w % 40];
      w/=40;

      d+=3;
   }

   return(d);
}

bool v7p_gethubregion(uint32_t datoffset,uint32_t *hub,uint32_t *region)
{
   uint32_t dtpoffset,sum;
   uchar *junk,*unpacked;
   int sz,d,c;

   osSeek(v7p_datfh,datoffset,OFFSET_BEGINNING);

   if(osRead(v7p_datfh,&v7p_datheader,sizeof(struct v7p_datheader)) != sizeof(struct v7p_datheader))
      return(FALSE);

   sz=v7p_datheader.Phone_len;

   if(v7p_datheader.Password_len > sz)
      sz=v7p_datheader.Password_len;

   if(v7p_datheader.pack_len > sz)
      sz=v7p_datheader.pack_len;

   if(!(junk=osAlloc(sz)))
   {
      return(FALSE);
   }

   if(!(unpacked=osAlloc(sz*3/2+3)))
   {
      osFree(junk);
      return(FALSE);
   }

   if(osRead(v7p_datfh,junk,v7p_datheader.Phone_len) != v7p_datheader.Phone_len)
   {
      osFree(junk);
      osFree(unpacked);
      return(FALSE);
   }

   if(osRead(v7p_datfh,junk,v7p_datheader.Password_len) != v7p_datheader.Password_len)
   {
      osFree(junk);
      osFree(unpacked);
      return(FALSE);
   }

   if(osRead(v7p_datfh,junk,v7p_datheader.pack_len) != v7p_datheader.pack_len)
   {
      osFree(junk);
      osFree(unpacked);
      return(FALSE);
   }

   d=v7p_unpack(unpacked,junk,v7p_datheader.pack_len);
   unpacked[d]=0;

   sum = v7p_datheader.Bname_len + v7p_datheader.Sname_len + v7p_datheader.Cname_len;

   if(d-sum < 8)
   {
      /* not v7+ */
      osFree(junk);
      osFree(unpacked);
      return(FALSE);
   }

   for(c=0;c<8;c++)
      if(!isxdigit(unpacked[sum+c]))
      {
         /* not v7+ */
         osFree(junk);
         osFree(unpacked);
         return(FALSE);
      }

   unpacked[sum+8]=0;
   dtpoffset=hextodec(&unpacked[sum]);

   osSeek(v7p_dtpfh,dtpoffset,OFFSET_BEGINNING);

   if(osRead(v7p_dtpfh,junk,4) != 4)
   {
      osFree(junk);
      osFree(unpacked);
      return(FALSE);
   }

   *region = junk[0]+junk[1]*256; /* Platform-independent */
   *hub    = junk[2]+junk[3]*256;

   osFree(junk);
   osFree(unpacked);

   return(TRUE);
}

bool v7p_nlStart(uchar *errbuf)
{
   uchar ndxname[120],datname[120],dtpname[120];

	MakeFullPath(config.cfg_Nodelist,V7P_NDXFILENAME,ndxname,120);
	MakeFullPath(config.cfg_Nodelist,V7P_DATFILENAME,datname,120);
	MakeFullPath(config.cfg_Nodelist,V7P_DTPFILENAME,dtpname,120);

   if(!(v7p_ndxfh=osOpen(ndxname,MODE_OLDFILE)))
   {
      sprintf(errbuf,"Failed to open V7+ index file \"%s\"",ndxname);
      return(FALSE);
   }

   if(!(v7p_datfh=osOpen(datname,MODE_OLDFILE)))
   {
      sprintf(errbuf,"Failed to open V7+ data file \"%s\"",datname);
      osClose(v7p_ndxfh);
      return(FALSE);
   }

   if(!(v7p_dtpfh=osOpen(dtpname,MODE_OLDFILE)))
   {
      sprintf(errbuf,"Failed to open V7+ dtp file \"%s\" (not a V7+ nodelist?)",dtpname);
      osClose(v7p_ndxfh);
      osClose(v7p_datfh);
      return(FALSE);
   }

   if(osRead(v7p_ndxfh,&v7p_ndxrecsize,sizeof(uint16_t))!=sizeof(uint16_t))
   {
      sprintf(errbuf,"V7+ nodelist \"%s\" appears to be corrupt",config.cfg_Nodelist);
      osClose(v7p_ndxfh);
      osClose(v7p_datfh);
      osClose(v7p_dtpfh);
      return(FALSE);
   }

   if(v7p_ndxrecsize > V7P_NDXBUFSIZE)
   {
      sprintf(errbuf,"Record size of V7+ nodelist is too big (%d uchars, max is %d uchars)",v7p_ndxrecsize,V7P_NDXBUFSIZE);
      osClose(v7p_ndxfh);
      osClose(v7p_datfh);
      osClose(v7p_dtpfh);
      return(FALSE);
   }

   if(osRead(v7p_ndxfh,&v7p_ndxcontrol,sizeof(struct v7p_ndxcontrol))!=sizeof(struct v7p_ndxcontrol))
   {
      sprintf(errbuf,"V7+ nodelist \"%s\" appears to be corrupt",config.cfg_Nodelist);
      osClose(v7p_ndxfh);
      osClose(v7p_datfh);
      osClose(v7p_dtpfh);
      return(FALSE);
   }

   return(TRUE);
}

void v7p_nlEnd(void)
{
   osClose(v7p_ndxfh);
   osClose(v7p_datfh);
   osClose(v7p_dtpfh);
}

bool v7p_nlCheckNode(struct Node4D *node)
{
   uint32_t junk;

   if(v7p_findoffset(node,&junk))
      return(TRUE);

   return(FALSE);
}

long v7p_nlGetHub(struct Node4D *node)
{
   struct Node4D t4d;
   uint32_t hub,region,datoffset;

   Copy4D(&t4d,node);
   t4d.Point=0;

   if(!v7p_findoffset(&t4d,&datoffset))
      return(-1);

   if(!v7p_gethubregion(datoffset,&hub,&region))
      return(-1);

   return(hub);
}

long v7p_nlGetRegion(struct Node4D *node)
{
   struct Node4D t4d;
   uint32_t hub,region,datoffset;

   Copy4D(&t4d,node);
   t4d.Point=0;

   if(!v7p_findoffset(&t4d,&datoffset))
      return(-1);

   if(!v7p_gethubregion(datoffset,&hub,&region))
      return(-1);

   return(region);
}

/* for testing
int main(int argc, char **argv)
{
   uchar err[200];
   struct Node4D n;

   nlname=argv[1];

   if(!v7p_nlStart(err))
   {
      printf("err: %s\n",err);
      exit(10);
   }

   Parse4D(argv[2],&n);

   printf(" check: %ld\n",v7p_nlCheckNode(&n));
   printf("   hub: %ld\n",v7p_nlGetHub(&n));
   printf("region: %ld\n",v7p_nlGetRegion(&n));

   v7p_nlEnd();

   exit(0);
}
*/
