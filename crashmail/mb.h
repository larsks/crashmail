#ifndef MB_H
#define MB_H

#include "shared/types.h"
#include "memmessage.h"
#include "config.h"

struct Messagebase
{
   char *name;
   char *desc;
   bool active;
   bool (*beforefunc)(void);
   bool (*afterfunc)(bool success);
   bool (*importfunc)(struct MemMessage *mm,struct Area *area);
   bool (*exportfunc)(struct Area *area,bool (*handlefunc)(struct MemMessage *mm));
   bool (*rescanfunc)(struct Area *area,uint32_t max,bool (*handlefunc)(struct MemMessage *mm));
};

#endif
