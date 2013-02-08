bool HandleMessage(struct MemMessage *mm);
bool HandleRescan(struct MemMessage *mm);
struct Area *AddArea(uchar *name,struct Node4D *node,struct Node4D *mynode,uint32_t active,uint32_t forcepassthru);
bool AddTossNode(struct Area *area,struct ConfigNode *cnode,uint16_t flags);
bool WriteBad(struct MemMessage *mm,uchar *reason);
bool Bounce(struct MemMessage *mm,uchar *reason,bool headeronly);

