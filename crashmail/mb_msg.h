bool msg_beforefunc(void);
bool msg_afterfunc(bool success);
bool msg_importfunc(struct MemMessage *mm,struct Area *area);
bool msg_exportfunc(struct Area *area,bool (*handlefunc)(struct MemMessage *mm));
bool msg_rescanfunc(struct Area *area,uint32_t max,bool (*handlefunc)(struct MemMessage *mm));
