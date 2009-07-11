#include <stddef.h>
#include <stdint.h>

typedef uint16_t UINT16; /* Unsigned 16-bit integer */

#define OS_EXIT_ERROR   10
#define OS_EXIT_OK      0

#define OS_PLATFORM_NAME "Linux"
#define OS_PATH_CHARS "/"
#define OS_CURRENT_DIR "."

#define OS_CONFIG_NAME "crashmail.prefs"
#define OS_CONFIG_VAR "CMCONFIGFILE"

#define OS_HAS_SYSLOG

/*
   OS_PATH_CHARS is used by MakeFullPath. If path doesn't end with one of these characters,
   the first character will be appended to it.

   Example:

   OS_PATH_CHARS = "/:"

   "inbound" + "file"  --> "inbound/file"
   "inbound/" + "file" --> "inbound/file"
   "inbound:" + "file" --> "inbound/file"
*/

#define stricmp strcasecmp
#define strnicmp strncasecmp

