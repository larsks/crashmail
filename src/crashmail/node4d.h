#ifndef NODE4D_H
#define NODE4D_H

struct Node4D
{
   ushort Zone,Net,Node,Point;
};

#define PAT_PATTERN   0
#define PAT_ZONE      1
#define PAT_REGION    2
#define PAT_NET       3
#define PAT_HUB       4
#define PAT_NODE      5

struct Node4DPat
{
   uchar Type;
   uchar Zone[10];
   uchar Point[10];
   uchar Net[10];
   uchar Node[10];
};

struct Node2DPat
{
   uchar Net[10];
   uchar Node[10];
};

bool Parse4D(uchar *buf, struct Node4D *node);
void Copy4D(struct Node4D *node1,struct Node4D *node2);
int Compare4D(struct Node4D *node1,struct Node4D *node2);
void Print4D(struct Node4D *n4d,uchar *dest);

bool Parse4DPat(uchar *buf, struct Node4DPat *node);
bool Parse4DDestPat(uchar *buf, struct Node4DPat *node);
int Compare4DPat(struct Node4DPat *nodepat,struct Node4D *node);
void Print4DPat(struct Node4DPat *pat,uchar *dest);
void Print4DDestPat(struct Node4DPat *pat,uchar *dest);
bool Check4DPatNodelist(struct Node4DPat *pat);

bool Parse2DPat(uchar *buf, struct Node2DPat *node);
int Compare2DPat(struct Node2DPat *nodepat,ushort net,ushort node);

bool Parse5D(uchar *buf, struct Node4D *n4d, uchar *domain);

#endif
