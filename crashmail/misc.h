void ExpandPacker(char *cmd,char *dest,uint32_t destsize,char *archive,char *file);

bool SortFEList(struct jbList *list);
void BadFile(char *filename,char *comment);

bool IsArc(char *file);
bool IsPkt(char *file);
bool IsNewPkt(char *file);
bool IsPktTmp(char *file);
bool IsOrphan(char *file);
bool IsBad(char *file);

void striptrail(char *str);
void striplead(char *str);
void stripleadtrail(char *str);

bool MatchFlags(char group,char *node);

void ExpandFilter(char *cmd,char *dest,uint32_t destsize,
   char *rfc1,
   char *rfc2,
   char *msg,
   char *area,
   char *subj,
   char *time,
   char *from,
   char *to,
   char *orignode,
   char *destnode);


void MakeFidoDate(time_t tim,char *dest);
bool AddTID(struct MemMessage *mm);

bool movefile(char *file,char *newfile);
bool copyfile(char *file,char *newfile);

char ChangeType(struct Node4D *dest,char pri);
bool MakeNetmailKludges(struct MemMessage *mm);
time_t FidoToTime(char *date);
bool Parse5D(char *buf, struct Node4D *n4d, char *domain);
bool ExtractAddress(char *origin, struct Node4D *n4d);

unsigned long hextodec(char *hex);

bool WriteMSG(struct MemMessage *mm,char *file);
bool WriteRFC(struct MemMessage *mm,char *name,bool rfcaddr);






