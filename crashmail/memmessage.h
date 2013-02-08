#ifndef MEMMESSAGE_H
#define MEMMESSAGE_H

#define PKT_CHUNKLEN  10000
#define PKT_NUM2D        50
#define PKT_NUMPATH      10

struct TextChunk
{
   struct TextChunk *Next;
   uint32_t Length;
   uchar Data[PKT_CHUNKLEN];
};

struct Nodes2D
{
   struct Nodes2D *Next;
   uint16_t Nodes;
   uint16_t Net[PKT_NUM2D];
   uint16_t Node[PKT_NUM2D];
};

struct Path
{
   struct Path *Next;
   uint16_t Paths;
   uchar Path[PKT_NUMPATH][100];
};

#define MMFLAG_RESCANNED   1
#define MMFLAG_EXPORTED    2
#define MMFLAG_TOSSED      4
#define MMFLAG_NOSECURITY  8
#define MMFLAG_AUTOGEN	  16
#define MMFLAG_TWIT		  32
#define MMFLAG_KILL		  64

struct MemMessage
{
   uint32_t msgnum;

   struct Node4D OrigNode;
   struct Node4D DestNode;
   struct Node4D PktOrig;
   struct Node4D PktDest;

   struct Node4D Origin4D;

   uchar Area[80];

   uchar To[36];
   uchar From[36];
   uchar Subject[72];
   uchar DateTime[20];

   uchar MSGID[80];
   uchar REPLY[80];

   uint16_t Attr;
   uint16_t Cost;

   uchar Type;
   uint16_t Flags;
   
   struct jbList TextChunks;
   struct jbList SeenBy;
   struct jbList Path;
};

bool mmAddNodes2DList(struct jbList *list,uint16_t net,uint16_t node);
void mmRemNodes2DList(struct jbList *list,uint16_t net,uint16_t node);
void mmRemNodes2DListPat(struct jbList *list,struct Node2DPat *pat);
bool mmAddPath(uchar *str,struct jbList *list);
bool mmAddBuf(struct jbList *chunklist,uchar *buf,uint32_t len);
bool mmAddLine(struct MemMessage *mm,uchar *buf);
struct MemMessage *mmAlloc(void);
void mmClear(struct MemMessage *mm);
void mmFree(struct MemMessage *mm);
bool mmSortNodes2D(struct jbList *list);
uchar *mmMakeSeenByBuf(struct jbList *list);

#endif

