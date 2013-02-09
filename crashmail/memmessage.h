#ifndef MEMMESSAGE_H
#define MEMMESSAGE_H

#define PKT_CHUNKLEN  10000
#define PKT_NUM2D        50
#define PKT_NUMPATH      10

struct TextChunk
{
   struct TextChunk *Next;
   uint32_t Length;
   char Data[PKT_CHUNKLEN];
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
   char Path[PKT_NUMPATH][100];
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

   char Area[80];

   char To[36];
   char From[36];
   char Subject[72];
   char DateTime[20];

   char MSGID[80];
   char REPLY[80];

   uint16_t Attr;
   uint16_t Cost;

   uint8_t Type;
   uint16_t Flags;
   
   struct jbList TextChunks;
   struct jbList SeenBy;
   struct jbList Path;
};

bool mmAddNodes2DList(struct jbList *list,uint16_t net,uint16_t node);
void mmRemNodes2DList(struct jbList *list,uint16_t net,uint16_t node);
void mmRemNodes2DListPat(struct jbList *list,struct Node2DPat *pat);
bool mmAddPath(char *str,struct jbList *list);
bool mmAddBuf(struct jbList *chunklist,char *buf,uint32_t len);
bool mmAddLine(struct MemMessage *mm,char *buf);
struct MemMessage *mmAlloc(void);
void mmClear(struct MemMessage *mm);
void mmFree(struct MemMessage *mm);
bool mmSortNodes2D(struct jbList *list);
char *mmMakeSeenByBuf(struct jbList *list);

#endif

