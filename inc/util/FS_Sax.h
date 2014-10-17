#ifndef _FS_SAX_H_
#define _FS_SAX_H_

#include "inc/FS_Config.h"

typedef void * FS_SaxHandle;

typedef void (* FS_SaxDataRequest)( void * userData, FS_SaxHandle hsax );

typedef void (* FS_SaxTextHandler)( void * userData, FS_CHAR *str );

typedef void (* FS_SaxElementTextHandler)( void * userData,  FS_CHAR *element, FS_CHAR *str, FS_SINT4 slen );

typedef void (* FS_SaxAttrHandler)( void * userData, FS_CHAR *element, FS_CHAR *name, FS_CHAR *value );

typedef void (* FS_SaxXmlNoteHandler)( void * userData, FS_CHAR *version, FS_CHAR *encoding );

FS_SaxHandle FS_CreateSaxHandler( void *userData );

void FS_FreeSaxHandler( FS_SaxHandle hsax );

void FS_SaxDataFeed( FS_SaxHandle hsax, FS_BYTE *data, FS_SINT4 dlen, FS_BOOL finish );

void FS_SaxSetDataRequest( FS_SaxHandle hsax, FS_SaxDataRequest handler );

void FS_SaxSetStartElementHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler );

void FS_SaxSetStartElementEndHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler );

void FS_SaxSetEndElementHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler );

void FS_SaxSetElementTextHandler( FS_SaxHandle hsax, FS_SaxElementTextHandler handler );

void FS_SaxSetAttributeHandler( FS_SaxHandle hsax, FS_SaxAttrHandler handler );

void FS_SaxSetXmlNoteHandler( FS_SaxHandle hsax, FS_SaxXmlNoteHandler handler );

void FS_SaxSetXmlInstructionHandler( FS_SaxHandle hsax, FS_SaxTextHandler handler );

void FS_SaxSetCommentHandler( FS_SaxHandle hsax, FS_SaxElementTextHandler handler );

void FS_SaxSetCompleteHandler( FS_SaxHandle hsax, FS_SaxDataRequest handler );

void FS_SaxProcXmlDoc( FS_SaxHandle hsax );

#endif
