#include "crashmail.h"

#include "nl_cmnl.h"

struct Nodelist AvailNodelists[] =
{
#ifdef NODELIST_CMNL
   { "CMNL",
     "CrashMail nodelist format",
     cmnl_nlStart,
     cmnl_nlEnd,
     cmnl_nlCheckNode,
     cmnl_nlGetHub,
     cmnl_nlGetRegion },
#endif
   { NULL, /* NULL here marks the end of the array */
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL }
};

