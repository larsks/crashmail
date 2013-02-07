#include <shared/types.h>

#include <stddef.h>
#include <string.h>

bool osInit(void);
void osEnd(void);

typedef unsigned short UINT16; /* Unsigned 16-bit integer */

#define OS_EXIT_ERROR   10
#define OS_EXIT_OK      0

#define OS_PLATFORM_NAME "2"
#define OS_CURRENT_DIR "."
#define OS_PATH_CHARS "\\"

#define OS_CONFIG_NAME "crashmail.prefs"
#define OS_CONFIG_VAR "CMCONFIGFILE"

/*
   Used by MakeFullPath. If path doesn't end with one of these characters,
   the first character will be appended to it.

   Example:

   OS_PATH_CHARS = "/:"

   "inbound" + "file"  --> "inbound/file"
   "inbound/" + "file" --> "inbound/file"
   "inbound:" + "file" --> "inbound/file"
*/
