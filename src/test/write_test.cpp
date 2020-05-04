#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>

#include <string.h>
#include <stdio.h>

int main() {
    int writer_fd = open("output.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (writer_fd < 0) {
        printf("writer fd wrong\n");
        return 0;
    }
    write(writer_fd, "abc\n", 4);
    char* buffer = (char*)mmap(NULL, sizeof(char) * 1100000, PROT_READ | PROT_WRITE, MAP_SHARED, writer_fd, 0);
    printf("mmap over %d\n", buffer); fflush(stdout);
    if (buffer == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno)); fflush(stdout);
        return 0;
    }
    for (int i=0; i<100000; ++i) {
        memcpy(buffer + i*11, "0123456789\n", 11 * sizeof(char));
    }
        
    printf("memcpy over\n"); fflush(stdout);
    munmap(buffer, sizeof(char) * 1100000);
    close(writer_fd);
    printf("write over\n"); fflush(stdout);
    return 0;
}