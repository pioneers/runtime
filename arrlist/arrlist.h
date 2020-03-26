#ifndef ARRLIST_H
#define ARRLIST_H

#include <stdlib.h>

typedef struct {
	int *arr;
	int size;
	int arrlen;
} arrlist_t;

//construct a new empy arraylist of ints
//don't make copies of the pointer returned to prevent mem leaks
arrlist_t *arrlist ();

//append "elem" to the end of "alist"
void alist_append (int elem, arrlist_t *alist);

//insert "elem" at index "i" into "alist"; shifts all elements after "i" down one index
void alist_insert (int i, int elem, arrlist_t *alist);

//removes element at index "i" in "alist"; shift all elements after "i" up one index
void alist_remove (int i, arrlist_t *alist);

//return element at index "i" in "alist"
int alist_get (int i, arrlist_t *alist);

//returns size of "alist"
int alist_size (arrlist_t *alist);

//clears "alist" and resets its size to 0
void alist_clear (arrlist_t *alist);

//destroys (deallocates its memory and the "alist" pointer)
//do not try to use the pointer after calling this function
void alist_destroy (arrlist_t *alist);

#endif