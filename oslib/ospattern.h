#ifndef OS_OSPATTERN_H
#define OS_OSPATTERN_H

#include "shared/types.h"

bool osCheckPattern(char *pattern);
bool osMatchPattern(char *pattern,char *str);
bool osIsPattern(char *pattern);

#endif
