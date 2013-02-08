#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#include "oslib/osmem.h"
#include "shared/jblist.h"

void jbNewList(struct jbList *list)
{
   list->First=NULL;
   list->Last=NULL;
}

void jbAddNode(struct jbList *list, struct jbNode *node)
{
   if(!list->First)
      list->First=node;

   else
      list->Last->Next=node;

   list->Last=node;
   node->Next=NULL;
}

void jbFreeList(struct jbList *list)
{
   struct jbNode *tmp,*tmp2;

   for(tmp=list->First;tmp;)
   {
      tmp2=tmp->Next;
      osFree(tmp);
      tmp=tmp2;
   }

   list->First=NULL;
   list->Last=NULL;
}

void jbFreeNum(struct jbList *list,uint32_t num)
{
   struct jbNode *old=NULL,*tmp;
   uint32_t c;

   tmp=list->First;

   for(c=0;c<num && tmp;c++)
   {
      if(c==num-1)
         old=tmp;

     tmp=tmp->Next;
   }

   if(c==num)
   {
      if(old)
         old->Next=tmp->Next;

      if(num==0)
         list->First=tmp->Next;

      if(tmp->Next == NULL)
         list->Last=old;

      osFree(tmp);
   }
}

void jbFreeNode(struct jbList *list,struct jbNode *node)
{
   struct jbNode *old=NULL,*tmp;

   for(tmp=list->First;tmp;tmp=tmp->Next)
   {
      if(tmp->Next == node)
         old=tmp;

      if(tmp == node) break;
   }

   if(tmp == node)
   {
      if(old)
         old->Next=tmp->Next;

      if(node == list->First)
         list->First=tmp->Next;

      if(tmp->Next == NULL)
         list->Last=old;

      osFree(tmp);
   }
}
