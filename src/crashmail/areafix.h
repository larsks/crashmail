
#define SENDLIST_FULL      1
#define SENDLIST_QUERY     2
#define SENDLIST_UNLINKED  3
#define SENDLIST_HELP		4
#define SENDLIST_INFO		5

void DoSendAFList(short type,struct ConfigNode *cnode);
bool AreaFix(struct MemMessage *mm);

void SendRemoveMessages(struct Area *area);
void RemoveDeletedAreas(void);


