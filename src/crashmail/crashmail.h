#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

#include <shared/types.h>

#include <shared/parseargs.h>
#include <shared/jblist.h>
#include <shared/mystrncpy.h>
#include <shared/jbstrcpy.h>
#include <shared/path.h>
#include <shared/node4d.h>

#include <oslib/os.h>
#include <oslib/osmem.h>
#include <oslib/osfile.h>
#include <oslib/osdir.h>
#include <oslib/ospattern.h>
#include <oslib/osmisc.h>

#include <shared/fidonet.h>
#include <shared/storedmsg.h>        

#include "node4dpat.h"
#include "nl.h"
#include "mb.h"
#include "memmessage.h"
#include "config.h"
#include "memmessage.h"
#include "logwrite.h"
#include "dupe.h"
#include "stats.h"
#include "misc.h"
#include "safedel.h"
#include "toss.h"
#include "scan.h"
#include "pkt.h"
#include "handle.h"
#include "outbound.h"
#include "areafix.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 61

#define VERSION "0.61"

#define TID_VERSION "0.61"

extern struct jbList PktList;
extern struct jbList DeleteList;

extern bool nomem;
extern bool ioerror;
extern bool exitprg;

extern ulong ioerrornum;

extern ulong toss_total;
extern ulong toss_bad;
extern ulong toss_route;
extern ulong toss_import;
extern ulong toss_written;
extern ulong toss_dupes;

extern ulong scan_total;
extern ulong rescan_total;

extern bool istossing;
extern bool isscanning;
extern bool isrescanning;

extern bool no_security;

extern struct ConfigNode *RescanNode;

extern ulong DayStatsWritten;

extern struct Nodelist AvailNodelists[];
extern struct Messagebase AvailMessagebases[];

extern struct Config config;

extern bool ctrlc;

bool BeforeScanToss(void);
void AfterScanToss(bool success);

extern uchar *prinames[];


