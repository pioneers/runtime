#include "arrlist.h"

#define STARTSIZE 8
#define RESIZEFACTOR 2

#define BIGGER 1
#define SMALLER 0

//************************************** HELPER FUNCTIONS *****************************//

//return 1 if array needs to be resized if we want to add or remove an item
int need_resize (arrlist_t *alist)
{
	if (alist->size + 1 == alist->arrlen) {
		return 1;
	} else if (alist ->size - 1 == alist->arrlen / RESIZEFACTOR && alist->arrlen > STARTSIZE) {
		return 1;
	} else {
		return 0;
	}
}

//resize the array bigger or smaller; updates arrlen
void resize (arrlist_t *alist, int direction)
{
	int *new_arr;
	
	if (direction == BIGGER) {
		new_arr = (int *) malloc(sizeof(int) * (alist->arrlen * RESIZEFACTOR));
		alist->arrlen = alist->arrlen * RESIZEFACTOR;
	} else {
		new_arr = (int *) malloc(sizeof(int) * (alist->arrlen / RESIZEFACTOR));
		alist->arrlen = alist->arrlen / RESIZEFACTOR;
	}
	
	for (int i = 0; i < alist->size; i++) {
		new_arr[i] = alist->arr[i];
	}
	
	free(alist->arr);
	alist->arr = new_arr;
}


//************************************* PUBLIC FUNCTIONS *****************************//

arrlist_t *arrlist ()
{
	arrlist_t *ret = (arrlist_t *) malloc (sizeof(arrlist_t));
	ret->arr = (int *) malloc(sizeof(int) * STARTSIZE);
	ret->size = 0;
	ret->arrlen = STARTSIZE;
	return ret;
}

void alist_append (int elem, arrlist_t *alist)
{
	if (need_resize(alist)) {
		resize(alist, BIGGER);
	}
	alist->arr[alist->size] = elem;
	alist->size++;
}

void alist_insert (int i, int elem, arrlist_t *alist)
{
	if (need_resize(alist)) {
		resize(alist, BIGGER);
	}
	
	for (int j = alist->size; j > i; j--) {
		alist->arr[j] = alist->arr[j - 1];
	}
	alist->arr[i] = elem;
	alist->size++;
}

void alist_remove (int i, arrlist_t *alist)
{
	for (int j = i; j < alist->size; j++) {
		alist->arr[j] = alist->arr[j + 1];
	}
	alist->size--;
	
	if (need_resize(alist)) {
		resize(alist, SMALLER);
	}
}

int alist_get (int i, arrlist_t *alist)
{
	return alist->arr[i];
}

int alist_size (arrlist_t *alist)
{
	return alist->size;
}

void alist_clear (arrlist_t *alist)
{
	free(alist->arr);
	alist->arr = (int *) malloc(sizeof(int) * STARTSIZE);
	alist->size = 0;
	alist->arrlen = STARTSIZE;
}

void alist_destroy (arrlist_t *alist)
{
	free(alist->arr);
	free(alist);
}