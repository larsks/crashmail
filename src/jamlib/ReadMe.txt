The contents of this directory is the JAMLIB library by Björn Stenberg with 
some minor modifications made by myself. Thank you Björn! You can find the 
documentation for JAMLIB in the file jamlib.doc.

These are the modifications done by me:

 * Support for Win32 and Linux added

 * JAM_ReadMsgHeader() now gives the error JAM_NO_MESSAGE if the
   message has been removed from the messagebase (both offset and
   crc are -1 in the index file).

 * The function JAM_AddEmptyMessage() has been added

 * Improved ANSI-C compatibilty by changing the header files 
   included and using feof() to check for EOF.

 * Some other bugfixes

 / Johan Billing
