#include <stdio.h>
#include <time.h>
#include <libusb.h>

#include "arrlist.h"

int main()
{
	arrlist_t *alist = arrlist();
	clock_t start, end;
	
	start = clock();	
	for (int i = 0; i < 1000000; i++) {
		alist_append(i, alist);
	}
	end = clock();
	
	printf("time is %f sec\n", (double) (end - start) / CLOCKS_PER_SEC);
	
	/*
	printf("size = %d\n", alist_size(alist));
	printf("get(279) = %d\n", alist_get(279, alist));
	printf("get(0) = %d\n", alist_get(0, alist));
	printf("get(999) = %d\n", alist_get(999, alist));
	
	alist_clear(alist);
	
	for (int i =0 ; i < 10; i++) {
		alist_append(i, alist);
	}
	alist_remove(6, alist);
	printf("size = %d\n", alist_size(alist));
	printf("get(5) = %d\n", alist_get(5, alist));
	printf("get(6) = %d\n", alist_get(6, alist));
	*/
	
	alist_destroy(alist);
	
	return 0;
}