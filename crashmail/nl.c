#include "crashmail.h"

#ifdef NODELIST_CMNL
#include "nl_cmnl.h"
#endif

#ifdef NODELIST_V7P
#include "nl_v7p.h"
#endif

struct Nodelist AvailNodelists[] =
{
#ifdef NODELIST_CMNL
   { "CMNL",
     "CrashMail nodelist index",
     cmnl_nlStart,
     cmnl_nlEnd,
     cmnl_nlCheckNode,
     cmnl_nlGetHub,
     cmnl_nlGetRegion },
#endif
#ifdef NODELIST_V7P
   { "V7+",
     "Version 7+ format",
     v7p_nlStart,
     v7p_nlEnd,
     v7p_nlCheckNode,
     v7p_nlGetHub,
     v7p_nlGetRegion },
#endif
   { NULL, /* NULL here marks the end of the array */
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL }
};

