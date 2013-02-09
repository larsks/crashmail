#ifndef OS_OSMISC_H
#define OS_OSMISC_H

#include "shared/types.h"

void osSetComment(char *file,char *comment);
bool osExists(char *file);

int osChDirExecute(char *dir,char *cmd);
int osExecute(char *cmd);

bool osRename(char *oldfile,char *newfile);
bool osDelete(char *file);

bool osMkDir(char *dir);
void osSleep(int secs);

uint32_t osError(void);
char *osErrorMsg(uint32_t errnum);

#endif
