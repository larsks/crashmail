#ifndef SHARED_PARSEARGS_H
#define SHARED_PARSEARGS_H

#include "shared/types.h"

#define ARGTYPE_END    0
#define ARGTYPE_STRING 1
#define ARGTYPE_BOOL   2

struct argument
{
   ushort type;
   uchar *name;
   void *data;
};

bool parseargs(struct argument *arg,int argc, char **argv);
void printargs(struct argument *arg);

#endif
