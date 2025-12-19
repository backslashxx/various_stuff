#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void print(char **s)
{
	syscall(SYS_write, 1, (long)*s, strlen(*s));
}

void print_int(int *a)
{
	printf("%d\n", *a);
}

// pptr is actually **
// we do this so any types can be passed
void free_byref(void *pptr)
{
	// we just cast inside so compiler wont hate it
	free(*(void **)pptr);
	*(void **)pptr = NULL;
}

int main(int argc, char *argv[], char *envp[])
{
	int b __attribute__((__cleanup__(print_int))) = 2;
	int a __attribute__((__cleanup__(print_int))) = 1;
	char *w __attribute__((__cleanup__(print))) = "world\n";

	char *m __attribute__((__cleanup__(free_byref))) = malloc(1000);

	
	char *h = "hello ";
	
	print(&h);
	
	strcpy(m, envp[0]);
	printf("%s\n", m);
	
	return 0;
}
