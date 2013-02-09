#ifndef NODE4D_H
#define NODE4D_H

#include <shared/types.h>

struct Node4D
{
   uint16_t Zone,Net,Node,Point;
};

bool Parse4D(char *buf, struct Node4D *node);
bool Parse4DTemplate(char *buf, struct Node4D *node,struct Node4D *tpl);
void Copy4D(struct Node4D *node1,struct Node4D *node2);
int Compare4D(struct Node4D *node1,struct Node4D *node2);
void Print4D(struct Node4D *n4d,char *dest);

#endif
