#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
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
#include <shared/expr.h>

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
#include "filter.h"

#include "version.h"

extern struct jbList PktList;
extern struct jbList DeleteList;

extern bool nomem;
extern bool ioerror;

extern uint32_t ioerrornum;

extern uint32_t toss_read;
extern uint32_t toss_bad;
extern uint32_t toss_route;
extern uint32_t toss_import;
extern uint32_t toss_written;
extern uint32_t toss_dupes;

extern uint32_t scan_total;
extern uint32_t rescan_total;

extern bool no_security;

extern int handle_nesting;

extern struct ConfigNode *RescanNode;

extern uint32_t DayStatsWritten;

extern struct Nodelist AvailNodelists[];
extern struct Messagebase AvailMessagebases[];

extern struct Config config;

extern bool ctrlc;
extern bool nodelistopen;

bool BeforeScanToss(void);
void AfterScanToss(bool success);

extern char *prinames[];


