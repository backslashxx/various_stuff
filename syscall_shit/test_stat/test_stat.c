#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("usage: %s <file>\n", argv[0]);
		return 1;
	}

	struct stat st;
	int fd = open(argv[1], O_RDONLY);
	
	if (fd < 0)
		return 1;

	if (!fstat(fd, &st)) {
		printf("size:  %lld bytes\n", (long long)st.st_size);
		printf("uid:   %d\n", st.st_uid);
		printf("inode: %llu\n", (unsigned long long)st.st_ino);
	} else 
		return 1;

	close(fd);
	return 0;
}
