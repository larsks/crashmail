/* PktHeader */

#define PKTHEADER_ORIGNODE       0
#define PKTHEADER_DESTNODE       2
#define PKTHEADER_YEAR           4
#define PKTHEADER_MONTH          6
#define PKTHEADER_DAY            8
#define PKTHEADER_HOUR           10
#define PKTHEADER_MINUTE         12
#define PKTHEADER_SECOND         14

#define PKTHEADER_BAUD           16
#define PKTHEADER_PKTTYPE        18

#define PKTHEADER_ORIGNET        20
#define PKTHEADER_DESTNET        22
#define PKTHEADER_PRODCODELOW    24
#define PKTHEADER_REVMAJOR       25
#define PKTHEADER_PASSWORD       26

#define PKTHEADER_QORIGZONE      34
#define PKTHEADER_QDESTZONE      36

#define PKTHEADER_AUXNET         38
#define PKTHEADER_CWVALIDCOPY    40
#define PKTHEADER_PRODCODEHIGH   42
#define PKTHEADER_REVMINOR       43
#define PKTHEADER_CAPABILWORD    44

#define PKTHEADER_ORIGZONE       46
#define PKTHEADER_DESTZONE       48
#define PKTHEADER_ORIGPOINT      50
#define PKTHEADER_DESTPOINT      52
#define PKTHEADER_PRODDATA       54

#define SIZE_PKTHEADER           58

/* PktHeader FSC-0045 */

#define PKTHEADER45_ORIGNODE       0
#define PKTHEADER45_DESTNODE       2
#define PKTHEADER45_ORIGPOINT      4
#define PKTHEADER45_DESTPOINT      6
#define PKTHEADER45_RESERVED       8
#define PKTHEADER45_SUBVERSION    16
#define PKTHEADER45_VERSION       18

#define PKTHEADER45_ORIGNET       20
#define PKTHEADER45_DESTNET       22
#define PKTHEADER45_PRODCODE      24
#define PKTHEADER45_REVISION      25
#define PKTHEADER45_PASSWORD      26

#define PKTHEADER45_ORIGZONE      34
#define PKTHEADER45_DESTZONE      36

#define PKTHEADER45_ORIGDOMAIN    38
#define PKTHEADER45_DESTDOMAIN    46
#define PKTHEADER45_PRODDATA      54

#define SIZE_PKTHEADER45          58

/* PktMsgHeader */                 

#define PKTMSGHEADER_PKTTYPE       0
#define PKTMSGHEADER_ORIGNODE      2
#define PKTMSGHEADER_DESTNODE      4
#define PKTMSGHEADER_ORIGNET       6
#define PKTMSGHEADER_DESTNET       8
#define PKTMSGHEADER_ATTR         10
#define PKTMSGHEADER_COST         12
/* plus header strings */

#define SIZE_PKTMSGHEADER         14

/* Header flags */
#define FLAG_PVT             1
#define FLAG_CRASH           2
#define FLAG_RECD            4
#define FLAG_SENT            8
#define FLAG_FILEATTACH     16
#define FLAG_INTRANSIT      32
#define FLAG_ORPHAN         64
#define FLAG_KILLSENT      128
#define FLAG_LOCAL         256
#define FLAG_HOLD          512
/*      FLAG_UNUSED       1024 */
#define FLAG_FILEREQ      2048
#define FLAG_RREQ         4096
#define FLAG_IRRR         8192
#define FLAG_AUDIT       16384
#define FLAG_UPDATEREQ   32768
