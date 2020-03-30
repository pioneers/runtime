#include <stdio.h>

#define SNAME "hi\n"

struct s1 {
	int i1;
	int i2;
};

struct s2 {
	int i;
	char c;
};

struct s3 {
	int i;
	char *cp;
};

struct s4 {
	int i;
	char c_a[8];
};

int main()
{
	printf("sizeof(int) = %lu\n", sizeof(int));
	printf("sizeof(char) = %lu\n", sizeof(char));
	
	printf("sizeof(struct s1) = %lu\n", sizeof(struct s1));
	printf("sizeof(struct s2) = %lu\n", sizeof(struct s2));
	printf("sizeof(struct s3) = %lu\n", sizeof(struct s3));
	printf("sizeof(struct s4) = %lu\n", sizeof(struct s4));
	
	printf(SNAME);
	
	return 0;
}