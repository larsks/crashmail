bool HandleMessage(struct MemMessage *mm);
bool HandleNetmail(struct MemMessage *mm);
bool HandleRescan(struct MemMessage *mm);
struct Area *AddArea(uchar *name,struct Node4D *node,struct Node4D *mynode,ulong active,ulong forcepassthru);
bool AddTossNode(struct Area *area,struct ConfigNode *cnode,ushort flags);

