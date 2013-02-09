#ifndef CONFIG_H
#define CONFIG_H

#define READCONFIG_NOT_FOUND  1
#define READCONFIG_INVALID    2
#define READCONFIG_NO_MEM     3

#define CFG_CHECKSEENBY            (1L<<0)
#define CFG_CHECKPKTDEST           (1L<<1)
#define CFG_STRIPRE                (1L<<2)
#define CFG_FORCEINTL              (1L<<3)
#define CFG_NOROUTE                (1L<<4)
#define CFG_PATH3D                 (1L<<5)
#define CFG_IMPORTSEENBY           (1L<<6)
#define CFG_IMPORTEMPTYNETMAIL     (1L<<7)
#define CFG_BOUNCEPOINTS           (1L<<8)
#define CFG_ANSWERRECEIPT          (1L<<9)
#define CFG_ANSWERAUDIT            (1L<<10)
#define CFG_NODIRECTATTACH         (1L<<11)
#define CFG_IMPORTAREAFIX          (1L<<12)
#define CFG_AREAFIXREMOVE          (1L<<13)
#define CFG_WEEKDAYNAMING          (1L<<14)
#define CFG_ADDTID                 (1L<<16)
#define CFG_ALLOWRESCAN            (1L<<18)
#define CFG_FORWARDPASSTHRU        (1L<<19)
#define CFG_BOUNCEHEADERONLY       (1L<<21)
#define CFG_REMOVEWHENFEED         (1L<<22)
#define CFG_INCLUDEFORWARD         (1L<<23)
#define CFG_NOMAXOUTBOUNDZONE      (1L<<24)
#define CFG_ALLOWKILLSENT          (1L<<25)
#define CFG_FLOWCRLF               (1L<<26)
#define CFG_NOEXPORTNETMAIL        (1L<<27)

#ifdef MSGBASE_MSG
#define CFG_MSG_HIGHWATER         (1L<<1)
#define CFG_MSG_WRITEBACK         (1L<<2)
#endif

#ifdef MSGBASE_UMS
#define CFG_UMS_MAUSGATE               (1L<<0)
#define CFG_UMS_KEEPORIGIN             (1L<<1)
#define CFG_UMS_CHANGEMSGID            (1L<<2)
#define CFG_UMS_IGNOREORIGINDOMAIN     (1L<<3)
#define CFG_UMS_EMPTYTOALL             (1L<<4)
#endif

#ifdef MSGBASE_JAM
#define CFG_JAM_HIGHWATER     (1L<<0)
#define CFG_JAM_LINK          (1L<<1)
#define CFG_JAM_QUICKLINK     (1L<<2)
#endif

#ifdef PLATFORM_AMIGA
#define CFG_AMIGA_UNATTENDED        (1L<<0)
#endif

#define AREA_UNCONFIRMED    1
#define AREA_MANDATORY      2
#define AREA_DEFREADONLY    4
#define AREA_IGNOREDUPES    8
#define AREA_IGNORESEENBY  16

#define AREATYPE_NETMAIL   1
#define AREATYPE_ECHOMAIL  2
#define AREATYPE_DEFAULT   3
#define AREATYPE_BAD       4
#define AREATYPE_LOCAL     5
#define AREATYPE_DELETED   6

struct Area
{
   struct Area *Next;
   bool changed;
   struct Aka  *Aka;
   char Flags;
   char AreaType;
   char Tagname[80];
   char Description[80];
   struct Messagebase *Messagebase;
   char Path[80];
   char Group;

   struct jbList TossNodes;
   struct jbList BannedNodes;

   /* Maint */

   uint32_t KeepNum,KeepDays;

   /* Stats */

   uint32_t Texts;
   uint32_t NewTexts;
   uint32_t Dupes;
   uint32_t NewDupes;
   uint16_t Last8Days[8];
   time_t FirstTime;
   time_t LastTime;

   /* Other */

   bool scanned;
};

struct Aka
{
   struct Aka *Next;
   struct Node4D Node;
   char Domain[20];
   struct jbList AddList;
   struct jbList RemList;
};

#define TOSSNODE_READONLY  1
#define TOSSNODE_WRITEONLY 2
#define TOSSNODE_FEED      4

struct TossNode
{
   struct TossNode *Next;
   struct ConfigNode *ConfigNode;
   uint16_t Flags;
};

struct ImportNode
{
   struct ImportNode *Next;
   struct Node4D Node;
};

struct BannedNode
{
   struct BannedNode *Next;
   struct ConfigNode *ConfigNode;
};

#define NODE_AUTOADD       1
#define NODE_PASSIVE        2
#define NODE_TINYSEENBY     4
#define NODE_NOSEENBY       8
#define NODE_FORWARDREQ    16
#define NODE_NOTIFY        32
#define NODE_PACKNETMAIL   64
#define NODE_SENDAREAFIX  128
#define NODE_SENDTEXT     256
#define NODE_PKTFROM	  512
#define NODE_AFNEEDSPLUS 1024
struct RemoteAFCommand
{
   struct RemoteAFCommand *Next;
   char Command[80];
};

struct ConfigNode
{
   struct ConfigNode *Next;
   bool changed;
   struct Node4D Node;
   char PacketPW[9];
   struct Packer *Packer;
   bool IsInSeenBy;
   char AreafixPW[40];
   char Groups[30];
   char ReadOnlyGroups[30];
   char AddGroups[30];
   char DefaultGroup;
   uint32_t Flags;
   char SysopName[36];
   char EchomailPri;
   struct Node4D PktFrom;
	
   char RemoteAFName[36];
   char RemoteAFPw[72];

   struct jbList RemoteAFList;

   char LastArcName[12];
	
   /* Stats */

   uint32_t GotNetmails;
   uint32_t GotNetmailBytes;
   uint32_t SentNetmails;
   uint32_t SentNetmailBytes;

   uint32_t GotEchomails;
   uint32_t GotEchomailBytes;
   uint32_t SentEchomails;
   uint32_t SentEchomailBytes;

   uint32_t Dupes;

   time_t FirstTime;
};

struct Packer
{
   struct Packer *Next;
   char Name[10];
   char Packer[80];
   char Unpacker[80];
   char Recog[80];
};

struct AddNode
{
   struct AddNode *Next;
   struct Node4D Node;
};

struct RemNode
{
   struct RemNode *Next;
   struct Node2DPat NodePat;
};

struct Route
{
   struct Route      *Next;
   struct Node4DPat Pattern;
   struct Node4DPat DestPat;
   struct Aka        *Aka;
};

struct PatternNode
{
   struct PatternNode *Next;
   struct Node4DPat Pattern;
};

#define PKTS_ECHOMAIL  0
#define PKTS_HOLD      1
#define PKTS_NORMAL    2
#define PKTS_DIRECT    3
#define PKTS_CRASH     4

struct Change
{
   struct Change *Next;
   struct Node4DPat Pattern;
   bool ChangeNormal;
   bool ChangeCrash;
   bool ChangeDirect;
   bool ChangeHold;
   char DestPri;
};

struct AreaFixName
{
   struct AreaFixName *Next;
   char Name[36];
};

#define AREALIST_DESC     1
#define AREALIST_FORWARD  2

struct Arealist
{
   struct Arealist *Next;
   struct ConfigNode *Node;
   char AreaFile[80];
   uint16_t Flags;
   char Group;
};

#define COMMAND_KILL 		  1
#define COMMAND_TWIT 		  2
#define COMMAND_COPY 		  3
#define COMMAND_EXECUTE	     4
#define COMMAND_WRITELOG     5
#define COMMAND_WRITEBAD     6
#define COMMAND_BOUNCEMSG    7
#define COMMAND_BOUNCEHEADER 8
#define COMMAND_REMAPMSG     9

struct Command
{
	struct Command *Next;
	uint16_t Cmd;
	char *string;
	struct Node4D n4d;
	struct Node4DPat n4ddestpat;
};

struct Filter
{
   struct Filter *Next;
   char Filter[400];
	struct jbList CommandList;
};

#define DUPE_IGNORE 0
#define DUPE_BAD    1
#define DUPE_KILL   2

#define LOOP_IGNORE  0
#define LOOP_LOG     1
#define LOOP_LOGBAD  2

struct Config
{
   bool changed;
   char filename[100];
   
   char cfg_Sysop[36];
   char cfg_Inbound[100];
   char cfg_Outbound[100];
   char cfg_TempDir[100];
   char cfg_PacketCreate[100];
   char cfg_PacketDir[100];
   char cfg_LogFile[100];
   char cfg_StatsFile[100];
   char cfg_DupeFile[100];
   char cfg_BeforeToss[80];
   char cfg_BeforePack[80];
   uint32_t cfg_LogLevel;
   uint32_t cfg_DupeSize;
   uint32_t cfg_MaxPktSize;
   uint32_t cfg_MaxBundleSize;
   char cfg_AreaFixHelp[100];
   char cfg_Nodelist[100];
   struct Nodelist *cfg_NodelistType;
   uint32_t cfg_AreaFixMaxLines;
   char cfg_GroupNames[30][80];
   uint32_t cfg_Flags;
   uint16_t cfg_DupeMode;
   uint16_t cfg_LoopMode;
   uint32_t cfg_DefaultZone;
   struct jbList AkaList;
   struct jbList AreaList;
   struct jbList CNodeList;
   struct jbList PackerList;
   struct jbList RouteList;
   struct jbList FileAttachList;
   struct jbList BounceList;
   struct jbList ChangeList;
   struct jbList AreaFixList;
   struct jbList ArealistList;
   struct jbList FilterList;

#ifdef PLATFORM_AMIGA
   uint32_t cfg_amiga_LogBufferLines;
   uint32_t cfg_amiga_LogBufferSecs;
   uint32_t cfg_amiga_Flags;
#endif

#ifdef MSGBASE_UMS
   char cfg_ums_RFCGatewayName[40];
   struct Node4D cfg_ums_RFCGatewayNode;
   char cfg_ums_LoginName[80];
   char cfg_ums_LoginPassword[80];
   char cfg_ums_LoginServer[80];
   char cfg_ums_GatewayName[36];
   uint32_t cfg_ums_Flags;
#endif

#ifdef MSGBASE_MSG
   uint32_t cfg_msg_Flags;
#endif

#ifdef MSGBASE_JAM
   uint32_t cfg_jam_MaxOpen;
   uint32_t cfg_jam_Flags;
#endif
};

bool ReadConfig(char *filename,struct Config *cfg,short *seconderr,uint32_t *cfgline,char *cfgerr);
bool UpdateConfig(struct Config *cfg,char *cfgerr);
void InitConfig(struct Config *cfg);
void FreeConfig(struct Config *cfg);

bool CheckConfig(struct Config *cfg,char *cfgerr);
/* Should not be called in prefs */

#endif


