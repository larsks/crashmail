void ExpandPacker(uchar *cmd,uchar *dest,ulong destsize,uchar *archive,uchar *file);

bool SortFEList(struct jbList *list);
void BadFile(uchar *filename,uchar *comment);

bool IsArc(uchar *file);
bool IsPkt(uchar *file);
bool IsNewPkt(uchar *file);
bool IsPktTmp(uchar *file);
bool IsOrphan(uchar *file);
bool IsBad(uchar *file);

void strip(uchar *str);
bool MatchFlags(uchar group,uchar *node);

void ExpandRobot(uchar *cmd,uchar *dest,ulong destsize,
   uchar *rfc1,
   uchar *rfc2,
   uchar *msg,
   uchar *subj,
   uchar *time,
   uchar *from,
   uchar *to,
   uchar *orignode,
   uchar *destnode);
   

void MakeFidoDate(time_t tim,uchar *dest);
bool AddTID(struct MemMessage *mm);

bool MoveFile(uchar *file,uchar *newfile);
bool CopyFile(uchar *file,uchar *newfile);

uchar ChangeType(struct Node4D *dest,uchar pri);
bool MakeNetmailKludges(struct MemMessage *mm);
time_t FidoToTime(uchar *date);
bool ExtractAddress(uchar *origin, struct Node4D *n4d,uchar *domain);

unsigned long hextodec(char *hex);







