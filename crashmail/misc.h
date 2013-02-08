void ExpandPacker(uchar *cmd,uchar *dest,uint32_t destsize,uchar *archive,uchar *file);

bool SortFEList(struct jbList *list);
void BadFile(uchar *filename,uchar *comment);

bool IsArc(uchar *file);
bool IsPkt(uchar *file);
bool IsNewPkt(uchar *file);
bool IsPktTmp(uchar *file);
bool IsOrphan(uchar *file);
bool IsBad(uchar *file);

void striptrail(uchar *str);
void striplead(uchar *str);
void stripleadtrail(uchar *str);

bool MatchFlags(uchar group,uchar *node);

void ExpandFilter(uchar *cmd,uchar *dest,uint32_t destsize,
   uchar *rfc1,
   uchar *rfc2,
   uchar *msg,
   uchar *area,
   uchar *subj,
   uchar *time,
   uchar *from,
   uchar *to,
   uchar *orignode,
   uchar *destnode);


void MakeFidoDate(time_t tim,uchar *dest);
bool AddTID(struct MemMessage *mm);

bool movefile(uchar *file,uchar *newfile);
bool copyfile(uchar *file,uchar *newfile);

uchar ChangeType(struct Node4D *dest,uchar pri);
bool MakeNetmailKludges(struct MemMessage *mm);
time_t FidoToTime(uchar *date);
bool Parse5D(uchar *buf, struct Node4D *n4d, uchar *domain);
bool ExtractAddress(uchar *origin, struct Node4D *n4d);

unsigned long hextodec(char *hex);

bool WriteMSG(struct MemMessage *mm,uchar *file);
bool WriteRFC(struct MemMessage *mm,uchar *name,bool rfcaddr);






