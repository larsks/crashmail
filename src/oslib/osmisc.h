#ifndef OS_OSMISC_H
#define OS_OSMISC_H

#include "shared/types.h"

void osSetComment(uchar *file,uchar *comment);
bool osExists(uchar *file);

int osChDirExecute(uchar *dir,uchar *cmd);
bool osMkDir(uchar *dir);

void osSleep(int secs);

#endif
