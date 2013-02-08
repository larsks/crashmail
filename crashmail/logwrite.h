#include <shared/types.h>

#define SYSTEMINFO   0
#define SYSTEMERR    1
#define TOSSINGINFO  2
#define TOSSINGERR   3
#define MISCINFO     4
#define DEBUG        5
#define AREAFIX      6
#define ACTIONINFO   7
#define USERERR      8

bool OpenLogfile(uchar *filename);
void CloseLogfile(void);
void LogWrite(uint32_t level,uint32_t category,uchar *fmt,...);
