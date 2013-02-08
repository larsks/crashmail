#ifndef OS_OSMISC_H
#define OS_OSMISC_H

#include "shared/types.h"

void osSetComment(uchar *file,uchar *comment);
bool osExists(uchar *file);

int osChDirExecute(uchar *dir,uchar *cmd);
int osExecute(uchar *cmd);

bool osRename(uchar *oldfile,uchar *newfile);
bool osDelete(uchar *file);

bool osMkDir(uchar *dir);
void osSleep(int secs);

uint32_t osError(void);
uchar *osErrorMsg(uint32_t errnum);

#endif
