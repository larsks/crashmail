#ifndef NL_H
#define NL_H

#include <shared/node4d.h>

struct Nodelist
{
   char *name;
   char *desc;
   bool (*nlStart)(char *errbuf);
   void (*nlEnd)(void);
   bool (*nlCheckNode)(struct Node4D *node);
   long (*nlGetHub)(struct Node4D *node);
   long (*nlGetRegion)(struct Node4D *node);
};

#endif
