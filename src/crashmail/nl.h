#ifndef NL_H
#define NL_H

#include "node4d.h"

struct Nodelist
{
   uchar *name;
   uchar *desc;
   bool (*nlStart)(uchar *errbuf);
   void (*nlEnd)(void);
   bool (*nlCheckNode)(struct Node4D *node);
   long (*nlGetHub)(struct Node4D *node);
   long (*nlGetRegion)(struct Node4D *node);
};

#endif
