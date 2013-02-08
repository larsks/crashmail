#ifndef SHARED_JBLIST_H
#define SHARED_JBLIST_H

struct jbList
{
   struct jbNode *First;
   struct jbNode *Last;
};

struct jbNode
{
   struct jbNode *Next;
};

void jbAddNode(struct jbList *list, struct jbNode *node);
void jbNewList(struct jbList *list);
void jbFreeList(struct jbList *list);
void jbFreeNum(struct jbList *list,uint32_t num);
void jbFreeNode(struct jbList *list,struct jbNode *node);

#endif
