#ifndef NODE4DPAT_H
#define NODE4DPAT_H

#define PAT_PATTERN   0
#define PAT_ZONE      1
#define PAT_REGION    2
#define PAT_NET       3
#define PAT_HUB       4
#define PAT_NODE      5

struct Node4DPat
{
   char Type;
   char Zone[10];
   char Point[10];
   char Net[10];
   char Node[10];
};

struct Node2DPat
{
   char Net[10];
   char Node[10];
};

bool Parse4DPat(char *buf, struct Node4DPat *node);
bool Parse4DDestPat(char *buf, struct Node4DPat *node);
int Compare4DPat(struct Node4DPat *nodepat,struct Node4D *node);
void Print4DPat(struct Node4DPat *pat,char *dest);
void Print4DDestPat(struct Node4DPat *pat,char *dest);
bool Check4DPatNodelist(struct Node4DPat *pat);

bool Parse2DPat(char *buf, struct Node2DPat *node);
int Compare2DPat(struct Node2DPat *nodepat,uint16_t net,uint16_t node);
void ExpandNodePat(struct Node4DPat *temproute,struct Node4D *dest,struct Node4D *sendto);

#endif
