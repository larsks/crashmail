#define FLOW_NONE 0
#define FLOW_TRUNC 1
#define FLOW_DELETE 2

bool ArchiveOutbound(void);
void MakeBaseName(struct Node4D *n4d,char *basename);
bool AddFlow(char *filename,struct Node4D *n4d,char type,long mode);
