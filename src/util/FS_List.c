#include "inc\FS_Config.h"
#include "inc\util\FS_List.h"
#include "inc\util\FS_MemDebug.h"

//---------------------------------------------------------------------------
// add an entry to list
static void __list_add(FS_List * node, FS_List * prev, FS_List * next)
{
	next->prev = node;
	node->next = next;
	node->prev = prev;
	prev->next = node;
}

void FS_ListCon( FS_List *head, FS_List *list )
{
	FS_List *first = list->next;
	FS_List *last = list->prev;
	FS_List *hlast = head->prev;
	if( first != list )
	{
		hlast->next = first;
		first->prev = hlast;
		last->next = head;
		head->prev = last;
	}
}

//---------------------------------------------------------------------------
// add an entry to list next to head
void FS_ListAdd( FS_List *head, FS_List *node )
{
	__list_add( node, head, head->next );
}
//---------------------------------------------------------------------------
// add an entry to list tail
void FS_ListAddTail( FS_List *head, FS_List *node )
{
	__list_add( node, head->prev, head );
}

void FS_ListDel( FS_List *node )
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
}

FS_SINT4 FS_ListCount( FS_List *head )
{
	FS_SINT4 ret = 0;
	FS_List *node = head->next;
	while( node != head )
	{
		node = node->next;
		ret ++;
	}
	return ret;
}
