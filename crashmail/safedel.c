#include "crashmail.h"

bool SafeDelete(char *file)
{
   struct osFileEntry *fe;

   if(!(fe=osAllocCleared(sizeof(struct osFileEntry))))
      return(FALSE);

   mystrncpy(fe->Name,file,100);
   jbAddNode(&DeleteList,(struct jbNode *)fe);

	return(TRUE);
}

void ProcessSafeDelete(void)
{
   struct osFileEntry *fe;

   for(fe=(struct osFileEntry *)DeleteList.First;fe;fe=fe->Next)
      osDelete(fe->Name);

   jbFreeList(&DeleteList);
}
