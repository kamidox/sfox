#ifndef _FS_UTIL_H_
#define _FS_UTIL_H_

#include "inc\FS_Config.h"

// list link structure
typedef struct FS_List_Tag
{
	struct FS_List_Tag *next;
	struct FS_List_Tag *prev;
}FS_List;

// calculate the entry address
#define FS_ListEntry(ptr, type, member)					\
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

// init a list
#define FS_ListInit(ptr) do {						\
	(ptr)->next = (ptr); (ptr)->prev = (ptr);		\
} while( 0 )

// empty test
#define FS_ListIsEmpty(list)		((list)->next == (list))

// for each entry in the list
#define FS_ListForEach(pos, head)					\
	for (pos = (head)->next; pos != (head); pos = pos->next)

void FS_ListAdd( FS_List *head, FS_List *node );

void FS_ListCon( FS_List *head, FS_List *list );

void FS_ListAddTail( FS_List *head, FS_List *node );

void FS_ListDel( FS_List *node );

FS_SINT4 FS_ListCount( FS_List *head );

#endif
