bool ReadPkt(char *pkt,struct osFileEntry *fe,bool bundled,bool (*handlefunc)(struct MemMessage *mm));
bool WriteEchoMail(struct MemMessage *mm,struct ConfigNode *cnode, struct Aka *aka);
bool WriteNetMail(struct MemMessage *mm,struct Node4D *dest,struct Aka *aka);
void ClosePackets(void);


