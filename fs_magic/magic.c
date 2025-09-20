#include <stdio.h>
#include <sys/vfs.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct statfs fsinfo;

    if (statfs(argv[1], &fsinfo) != 0) {
        printf("statfs err\n");
        return 1;
    }

    printf("magic number: 0x%lX\n", (unsigned long)fsinfo.f_type);

    return 0;
}
