#define FLOW_NONE 0
#define FLOW_TRUNC 1
#define FLOW_DELETE 2

bool ArchiveOutbound(void);
void MakeBaseName(struct Node4D *n4d,uchar *basename);
bool AddFlow(uchar *filename,struct Node4D *n4d,uchar type,long mode);
