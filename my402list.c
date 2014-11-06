#include<stdio.h>
#include<stdlib.h>

#include"my402list.h"


int My402ListLength(My402List* list)
{
	return list->num_members;
}
int My402ListEmpty(My402List* list)
{
	if(list->num_members == 0) return TRUE;
	else return FALSE;	
}

int My402ListAppend(My402List* list, void *new_obj)
{
	My402ListElem* elem = (My402ListElem*) malloc(sizeof(My402ListElem));
	if( !elem ) return FALSE;

	My402ListElem *last = My402ListLast(list);
	// elem update
	elem->prev = last;
	elem->next = &(list->anchor);
	elem->obj  = new_obj;
	// list update
	last->next = elem;
	list->anchor.prev = elem;
	// list length update	
	list->num_members ++;
	return TRUE;
}

int  My402ListPrepend(My402List* list, void* new_obj)
{
	My402ListElem* elem = (My402ListElem*) malloc (sizeof(My402ListElem));
	if(!elem) return FALSE;

	My402ListElem *first = My402ListFirst(list);
	// elem update	
	elem->next = first;
	elem->prev = &(list->anchor);
	elem->obj = new_obj;
	// list update
	first->prev = elem;
	list->anchor.next = elem;
	// list length update
	list->num_members ++;	
	return TRUE;
}

void My402ListUnlink(My402List* list, My402ListElem* elem)
{
	(elem->next)->prev = elem->prev;	
	(elem->prev)->next = elem->next;
	free((void*)elem);
	list->num_members --;
}

void My402ListUnlinkAll(My402List* list)
{
	My402ListElem *curr_elem, *next_elem;
	for (curr_elem = My402ListFirst(list), next_elem = curr_elem; curr_elem != 0; curr_elem = My402ListNext(list, next_elem), next_elem = curr_elem) {
		free((void*)curr_elem);
	}
	list->num_members = 0;
}

int  My402ListInsertAfter(My402List* list, void* new_obj, My402ListElem* elem)
{
	// snaity check
	if (!elem) return My402ListAppend(list, new_obj);	
	My402ListElem* temp_elem = (My402ListElem*) malloc (sizeof(My402ListElem));
	if (!temp_elem) return FALSE;
	temp_elem->prev = elem;
	temp_elem->next = elem->next;
	temp_elem->obj = new_obj;	
	(elem->next)->prev = temp_elem;
	elem->next = temp_elem;
	list->num_members ++;
	return TRUE;
}

int  My402ListInsertBefore(My402List* list, void* new_obj, My402ListElem* elem)
{
	// snaity check
	if (!elem) return My402ListPrepend(list, new_obj);
	My402ListElem* temp_elem = (My402ListElem*) malloc (sizeof(My402ListElem));
	if (!temp_elem) return FALSE;
	temp_elem->next = elem;
	temp_elem->prev = elem->prev;
	temp_elem->obj = new_obj;		
	(elem->prev)->next = temp_elem;	
	elem->prev = temp_elem;
	list->num_members ++;
	return TRUE;
}

My402ListElem *My402ListFirst(My402List* list)
{
		return list->anchor.next;
}

My402ListElem *My402ListLast(My402List* list)
{
		return list->anchor.prev;
}

My402ListElem *My402ListNext(My402List* list, My402ListElem* elem)
{
	if(elem->next == &(list->anchor))
		return NULL;
	else
		return elem->next;
}

My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem)
{
	if(elem->prev == &(list->anchor))
		return NULL;
	else
		return elem->prev;
}

My402ListElem *My402ListFind(My402List* list, void* obj)
{
	My402ListElem* temp_elem;
	for( temp_elem = My402ListFirst(list); temp_elem != 0; temp_elem = My402ListNext(list, temp_elem)) {
		if(temp_elem->obj == obj) break;
	}
	return temp_elem;
}

int My402ListInit(My402List* list)
{	
	if (list)
	{ 	
		list->anchor.prev = &(list->anchor);
		list->anchor.next = &(list->anchor);
		list->num_members = 0;
		return TRUE;	
	}	
	return FALSE;
}






