bool jam_beforefunc(void);
bool jam_afterfunc(bool success);
bool jam_importfunc(struct MemMessage *mm,struct Area *area);
bool jam_exportfunc(struct Area *area,bool (*handlefunc)(struct MemMessage *mm));
bool jam_rescanfunc(struct Area *area,uint32_t max,bool (*handlefunc)(struct MemMessage *mm));
