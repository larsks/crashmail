#include "shared/types.h"

void FreeDupebuf(void);
bool AllocDupebuf(void);
bool ReadDupes(uchar *file);
bool WriteDupes(uchar *file);
bool CheckDupe(struct MemMessage *mm);



