#include "shared/types.h"

struct cmnlIdx
{
   ushort zone,net,node,point,region,hub;
};
  
#define CMNLERR_NO_INDEX			1
#define CMNLERR_WRONG_TYPE 		2
#define CMNLERR_UNEXPECTED_EOF	3 				                                                              
#define CMNLERR_NODE_NOT_FOUND	4
#define CMNLERR_NO_NODELIST		5

osFile cmnlOpenNL(uchar *dir);
void cmnlCloseNL(osFile nl);
bool cmnlFindNL(osFile nl,uchar *dir,struct cmnlIdx *idx,uchar *line,ulong len);
uchar *cmnlLastError(void);

extern ulong cmnlerr;
