#ifndef OS_OSPATTERN_H
#define OS_OSPATTERN_H

#include "shared/types.h"

bool osCheckPattern(uchar *pattern);
bool osMatchPattern(uchar *pattern,uchar *str);
bool osIsPattern(uchar *pattern);

#endif
