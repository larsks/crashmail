#ifndef SHARED_PARSEARGS_H
#define SHARED_PARSEARGS_H

#include "shared/types.h"

#define ARGTYPE_END    0
#define ARGTYPE_STRING 1
#define ARGTYPE_BOOL   2

#define ARGFLAG_AUTO       1 /* Keyword does not have to be specified */
#define ARGFLAG_MANDATORY  2 /* Argument cannot be left out */

struct argument
{
   uint16_t type;
   char *name;
	uint16_t flags;
   void *data;
};

bool parseargs(struct argument *arg,int argc, char **argv);
void printargs(struct argument *arg);

#endif
