/*
   基本想法。开26个文件，写成：
   abcdefg...
   bcdefgh...
   cdefghi...
   这种东西。顺带就把 seek 和 O_APPEND 测了。
*/
#include "ramfs.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
int notin(int fd, int *fds, int n);

int genfd(int *fds, int n);


char alphabet[53] = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
int fd[26];
char buf[26][52];

void openall_succ(int flag);

void openall_fail(int flag);
void readall(int len, int expect);

void closeall(int expect);

void writeall(int off);
void writeall_fail(int off);

void seekall(int off, int whence, int expect);

int main() {
    init_ramfs();
    openall_succ(O_CREAT | O_RDWR | O_WRONLY);  // can't read
    readall(5, -1);
    for (int i = 0; i < 26; i++) {
        seekall(i, SEEK_SET, i);
        for (int j = 0; j < 26; j++) {
            writeall(j);
        }
        seekall(0, SEEK_END, 26 + i);
    }
    seekall(0, SEEK_END, 51);
    assert(rclose(genfd(fd, 26)) == -1);
    closeall(0);
    closeall(-1);
    openall_succ(O_RDONLY);
    readall(54, 51);
    for (int i = 0; i < 26; i++) {
        assert(memcmp(buf[i] + 25, alphabet + i, 26) == 0);
        for (int j = 0; j < 25; j++) {
            assert(buf[i][j] == alphabet[i]);
        }
    }
    closeall(0);
    for (int i = 0; i < 26; i++) {
        openall_succ(O_APPEND | O_RDWR);
        readall(5, 0);  // at rear
        writeall(i);
        closeall(0);
    }

    memset(buf, 0, sizeof(buf));
    openall_succ(O_RDONLY);
    seekall(0, SEEK_END, 77);
    seekall(51, SEEK_SET, 51);
    readall(52, 26);
    for (int i = 0; i < 26; i++) {
        assert(memcmp(buf[i], alphabet + i, 26) == 0);
    }
    closeall(0);
    return 0;
}

int notin(int fd, int *fds, int n) {
    for (int i = 0; i < n; i++) {
        if (fds[i] == fd) return 0;
    }
    return 1;
}
int genfd(int *fds, int n) {
    for (int i = 0; i < 4096; i++) {
        if (notin(i, fds, n))
            return i;
    }
    return -1;
}

void openall_succ(int flag) {
    char name[32];
    for (int i = 0; i < 26; i++) {
        sprintf(name, "/thisisthe%dthfile", i);
        assert((fd[i] = ropen(name, flag)) >= 0);
    }
}
void openall_fail(int flag) {
    char name[32];
    for (int i = 0; i < 26; i++) {
        sprintf(name, "/thisisthe%dthfile", i);
        assert((fd[i] = ropen(name, flag)) == -1);
    }
}
void readall(int len, int expect) {
    for (int i = 0; i < 26; i++) {
        assert(rread(fd[i], buf[i], len) == expect);
    }
}
void closeall(int expect) {
    for (int i = 0; i < 26; i++) {
        assert(rclose(fd[i]) == expect);
    }
}
void writeall(int off) {
    for (int i = 0; i < 26; i++) {
        assert(rwrite(fd[i], alphabet + ((i + off) % 26), 1) == 1);
    }
}
void writeall_fail(int off) {
    for (int i = 0; i < 26; i++) {
        assert(rwrite(fd[i], alphabet + ((i + off) % 26), 1) == -1);
    }
}
void seekall(int off, int whence, int expect) {
    for (int i = 0; i < 26; i++) {
        assert(rseek(fd[i], off, whence) == expect);
    }
}


/*#include "ramfs.h"
#include <assert.h>
#include <string.h>

int main() {
    init_ramfs();
    assert(rmkdir("/dir") == 0);
    assert(rmkdir("//dir") == -1);
    assert(rmkdir("/a/b") == -1);
    int fd;
    assert((fd = ropen("//dir///////1.txt", O_CREAT | O_RDWR)) >= 0);
    assert(rwrite(fd, "hello", 5) == 5);
    assert(rseek(fd, 0, SEEK_CUR) == 5);
    assert(rseek(fd, 0, SEEK_SET) == 0);
    char buf[8];
    assert(rread(fd, buf, 7) == 5);
    assert(memcmp(buf, "hello", 5) == 0);
    assert(rseek(fd, 3, SEEK_END) == 8);
    assert(rwrite(fd, "world", 5) == 5);
    assert(rseek(fd, 5, SEEK_SET) == 5);
    assert(rread(fd, buf, 8) == 8);
    assert(memcmp(buf, "\0\0\0world", 8) == 0);
    assert(rclose(fd) == 0);
    assert(rclose(fd + 1) == -1);
    return 0;
}*/
