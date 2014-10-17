#include "inc\FS_Config.h"
#include "inc\eml\FS_EmlFile.h"
#include "inc\util\FS_List.h"

void FS_EmlParseFile( FS_EmlFile *emlFile, FS_CHAR *file );

// to process a mime part, this will create a file in temp dir, use this to act LAZY copy
void FS_ProcessMimePart( FS_MimeEntry *entry );

void FS_EmlParseRcpt( FS_List *rcpt, FS_CHAR *file );

FS_BOOL FS_EmlParseTop( FS_EmlHead *emlHead, FS_CHAR ** data );

void FS_EmlParseAddrList( FS_List *head, FS_CHAR *str );

void FS_PackRfc822Message( FS_EmlFile *emlFile, FS_CHAR *file );

void FS_FreeEmlFile( FS_EmlFile *emlFile );

void FS_InitEmlFile( FS_EmlFile *emlFile );

void FS_EmlFreeAddrList( FS_List *head );


