#ifndef MB_H
#define MB_H

#define MSGBASE_MSG
#define MSGBASE_JAM

#include "shared/types.h"
#include "memmessage.h"
#include "config.h"

struct Messagebase
{
   uchar *name;
   uchar *desc;
   bool active;
   bool (*beforefunc)(void);
   bool (*afterfunc)(bool success);
   bool (*importfunc)(struct MemMessage *mm,struct Area *area);
   bool (*exportfunc)(struct Area *area,bool (*handlefunc)(struct MemMessage *mm));
   bool (*rescanfunc)(struct Area *area,ulong max,bool (*handlefunc)(struct MemMessage *mm));
};

#endif
