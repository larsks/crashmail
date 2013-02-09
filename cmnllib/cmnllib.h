#include "shared/types.h"

struct cmnlIdx
{
   uint16_t zone,net,node,point,region,hub;
};
  
#define CMNLERR_NO_INDEX			1
#define CMNLERR_WRONG_TYPE 		2
#define CMNLERR_UNEXPECTED_EOF	3 				                                                              
#define CMNLERR_NODE_NOT_FOUND	4
#define CMNLERR_NO_NODELIST		5

osFile cmnlOpenNL(char *dir);
void cmnlCloseNL(osFile nl);
bool cmnlFindNL(osFile nl,char *dir,struct cmnlIdx *idx,char *line,uint32_t len);
char *cmnlLastError(void);

extern uint32_t cmnlerr;
