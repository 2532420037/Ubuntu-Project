#include "ramfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* modify this file freely */

typedef struct node { //节点
    enum { FILE_NODE, DIR_NODE } type;
    struct node *dirents; // if it's a dir, there's subentries
    void *content; // if it's a file, there's data content
    int nrde; // number of subentries for dir
    int size; // size of file
    char *name; // it's short name
    struct node *same_level;
} node;

node root; //根文件

node *all = &root; //全局指针
node *pre = NULL;

typedef struct FD { //文件描述符
    int offset;
    int flags;
    node *f;
} FD;

FD fd_[4097]; //文件描述符的集合

int JudgeOfPathname(const char *pathname) { //判断pathname的合法性
    if (pathname == NULL) {
        return 0;
    }
    if (strlen(pathname) > 1024) {
        return 0;
    } else if (pathname[0] != '/') {
        return 0;
    } else {
        for (int i = 0; i < strlen(pathname); i++) {
            if (pathname[i] != '/') {
                if (!(((pathname[i] >= '0') && (pathname[i] <= '9')) || ((pathname[i] >= 'a') && (pathname[i] <= 'z')) || ((pathname[i] >= 'A') && (pathname[i] <= 'Z')) || (pathname[i] == '.'))) {
                    return 0;
                }
            }
        }
        int num1 = 0, num2 = 0;
        int local1[100], local2[100];
        for (int i = 0; i < strlen(pathname) - 1; i++) {
            if ((pathname[i] == '/') && (pathname[i + 1] != '/')) {
                local1[num1] = i + 1;
                num1++;
            }
            if ((pathname[i] != '/') && (pathname[i + 1] == '/')) {
                local2[num2] = i;
                num2++;
            }
        }
        int minus[num1];
        if (num1 == num2) {
            for (int i = 0; i < num1; i++) {
                minus[i] = local2[i] - local1[i] + 1;
            }
        } else if (num1 == num2 + 1) {
            local2[num2] = strlen(pathname) - 1;
            num2++;
            for (int i = 0; i < num1; i++) {
                minus[i] = local2[i] - local1[i] + 1;
            }
        } else {
            return 0;
        }
        for (int i = 0; i < num1; i++) {
            if (minus[i] > 32) {
                return 0;
            }
        }
        char name[num1][33];
        for (int i = 0; i < num1; i++) {
            for (int j = 0; j < minus[i]; j++) {
                name[i][j] = pathname[local1[i] + j];
            }
            name[i][minus[i]] = '\0';
        }
        if (num1 == 0) {
            return 3;
        } else if (num1 == 1) {
            if (all->nrde == 0) {
                if (pathname[strlen(pathname) - 1] != '/') {
                    return -1;
                } else {
                    return -2;
                }
            }
            node *tp = all->dirents;
            for (int i = 0; i < all->nrde; i++) {
                if (strcmp(name[0], tp->name) == 0) {
                    if ((tp->type == FILE_NODE) && (pathname[strlen(pathname) - 1] != '/')) {
                        pre = all;
                        all = tp;
                        return 1;
                    } else if (tp->type == DIR_NODE) {
                        pre = all;
                        all = tp;
                        return 2;
                    } else {
                        return 0;
                    }
                } else {
                    tp = tp->same_level;
                }
            }
            if (pathname[strlen(pathname) - 1] != '/') {
                return -1;
            } else {
                return -2;
            }
        } else {
            for (int i = 0; i < num1; i++) {
                int aa = 0;
                if (all->nrde == 0) {
                    if (i == num1 - 1) {
                        if (pathname[strlen(pathname) - 1] != '/') {
                            return -1;
                        } else {
                            return -2;
                        }
                    } else {
                        return 0;
                    }
                }
                node *tp = all->dirents;
                for (int j = 0; j < all->nrde; j++) {
                    if (i < num1 - 1) {
                        if ((strcmp(name[i], tp->name) == 0) && (tp->type == DIR_NODE)) {
                            pre = all;
                            all = tp;
                            aa = 1;
                            break;
                        } else {
                            tp = tp->same_level;
                        }
                    } else {
                        if (strcmp(name[i], tp->name) == 0) {
                            pre = all;
                            all = tp;
                            aa = 1;
                            break;
                        } else {
                            tp = tp->same_level;
                        }
                    }
                }
                if (aa == 0) {
                    if (i == num1 - 1) {
                        if (pathname[strlen(pathname) - 1] != '/') {
                            return -1;
                        } else {
                            return -2;
                        }
                    } else {
                        return 0;
                    }
                }
            }
            if ((all->type == FILE_NODE) && (pathname[strlen(pathname) - 1] != '/')) {
                return 1;
            } else if (all->type == DIR_NODE) {
                return 2;
            } else {
                return 0;
            }
        }
    }
}

int ropen(const char *pathname, int flags) {
  // TODO();
  int JudgeResult = JudgeOfPathname(pathname);
  if ((JudgeResult == 0) || (JudgeResult == -2)) {
      all = &root;
      pre = NULL;
      return -1;
  }
  if ((JudgeResult == 2) || (JudgeResult == 3)) {
      int bb = -1;
      for (int i = 0; i < 4097; i++) {
          if (fd_[i].f == NULL) {
              fd_[i].f = all;
              fd_[i].offset = -1;
              fd_[i].flags = 0;
              bb = i;
              break;
          }
      }
      all = &root;
      pre = NULL;
      return bb;
  }
  if (JudgeResult == -1) {
      if ((flags == O_CREAT) || (flags == (O_CREAT | O_RDONLY))) {
          (all->nrde)++;
          node *new = malloc(sizeof(*new));
          new->type = FILE_NODE;
          new->dirents = NULL;
          new->content = NULL;
          new->nrde = -1;
          new->size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          new->name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              new->name[j - cc] = pathname[j];
          }
          new->name[strlen(pathname) - cc] = '\0';
          new->same_level = all->dirents;
          all->dirents = new;
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = new;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 0;
          all = &root;
          pre = NULL;
          return dd;
      } else if (flags == (O_CREAT | O_WRONLY)) {
          (all->nrde)++;
          node *new = malloc(sizeof(*new));
          new->type = FILE_NODE;
          new->dirents = NULL;
          new->content = NULL;
          new->nrde = -1;
          new->size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          new->name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              new->name[j - cc] = pathname[j];
          }
          new->name[strlen(pathname) - cc] = '\0';
          new->same_level = all->dirents;
          all->dirents = new;
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = new;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 1;
          all = &root;
          pre = NULL;
          return dd;
      } else if (flags == (O_CREAT | O_RDWR)) {
          (all->nrde)++;
          node *new = malloc(sizeof(*new));
          new->type = FILE_NODE;
          new->dirents = NULL;
          new->content = NULL;
          new->nrde = -1;
          new->size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          new->name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              new->name[j - cc] = pathname[j];
          }
          new->name[strlen(pathname) - cc] = '\0';
          new->same_level = all->dirents;
          all->dirents = new;
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = new;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 2;
          all = &root;
          pre = NULL;
          return dd;
      } else {
          all = &root;
          pre = NULL;
          return -1;
      }
  }
  if (JudgeResult == 1) {
      if ((flags == O_APPEND) || (flags == (O_APPEND | O_RDONLY)) || (flags == (O_APPEND | O_CREAT))) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = (all->size);
          fd_[ee].flags = 0;
          all = &root;
          pre = NULL;
          return ee;
      }
      if (flags == (O_APPEND | O_WRONLY)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = (all->size);
          fd_[ee].flags = 1;
          all = &root;
          pre = NULL;
          return ee;
      }
      if (flags == (O_APPEND | O_RDWR)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = (all->size);
          fd_[ee].flags = 2;
          all = &root;
          pre = NULL;
          return ee;
      }
      if ((flags == O_CREAT) || (flags == O_RDONLY) || (flags == (O_CREAT | O_TRUNC)) || (flags == (O_CREAT | O_RDONLY)) || (flags == (O_TRUNC | O_RDONLY))) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = 0;
          fd_[ee].flags = 0;
          all = &root;
          pre = NULL;
          return ee;
      }
      if ((flags == (O_CREAT | O_WRONLY)) || (flags == (O_RDONLY | O_WRONLY)) || (flags == (O_RDWR | O_WRONLY)) || (flags == O_WRONLY)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = 0;
          fd_[ee].flags = 1;
          all = &root;
          pre = NULL;
          return ee;
      }
      if ((flags == (O_CREAT | O_RDWR)) || (flags == (O_RDONLY | O_RDWR)) || (flags == O_RDWR)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = 0;
          fd_[ee].flags = 2;
          all = &root;
          pre = NULL;
          return ee;
      }
      if (flags == (O_TRUNC | O_WRONLY)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = 0;
          fd_[ee].flags = 1;
          if (all->content != NULL) {
              free(all->content);
              all->content = NULL;
          }
          all = &root;
          pre = NULL;
          return ee;
      }
      if (flags == (O_TRUNC | O_RDWR)) {
          int ee = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  ee = i;
                  break;
              }
          }
          fd_[ee].offset = 0;
          fd_[ee].flags = 2;
          if (all->content != NULL) {
              free(all->content);
              all->content = NULL;
          }
          all = &root;
          pre = NULL;
          return ee;
      }
      all = &root;
      pre = NULL;
      return -1;
  }
}

int rclose(int fd) {
  // TODO();
  if (fd_[fd].f != NULL) {
      fd_[fd].offset = -1;
      fd_[fd].flags = -1;
      fd_[fd].f = NULL;
      return 0;
  } else {
      return -1;
  }
}

ssize_t rwrite(int fd, const void *buf, size_t count) {
  // TODO();
  if (buf == NULL) {
      return -1;
  }
  if ((fd_[fd].f != NULL) && (fd_[fd].f->type == FILE_NODE) && ((fd_[fd].flags == 1) || (fd_[fd].flags == 2))) {
      if (fd_[fd].offset + count > fd_[fd].f->size) {
          void *tem = realloc(fd_[fd].f->content, fd_[fd].offset + count);
          memcpy(tem + fd_[fd].offset, buf, count);
          if (fd_[fd].offset > fd_[fd].f->size) {
              for (int i = fd_[fd].f->size; i < fd_[fd].offset; i++) {
                  memcpy(tem + i, "\0", 1);
              }
          }
          fd_[fd].f->size = fd_[fd].offset + count;
          fd_[fd].f->content = tem;
          fd_[fd].offset = fd_[fd].f->size;
      } else {
          memcpy(fd_[fd].f->content + fd_[fd].offset, buf, count);
          fd_[fd].offset += count;
      }
      return count;
  } else {
      return -1;
  }
}

ssize_t rread(int fd, void *buf, size_t count) {
  // TODO();
  if (buf == NULL) {
      return -1;
  }
  if ((fd_[fd].f != NULL) && (fd_[fd].f->type == FILE_NODE) && ((fd_[fd].flags == 0) || (fd_[fd].flags == 2))) {
      int gg;
      if (fd_[fd].offset > fd_[fd].f->size) {
          return -1;
      }
      if (fd_[fd].offset + count > fd_[fd].f->size) {
          gg = fd_[fd].f->size - fd_[fd].offset;
      } else {
          gg = count;
      }
      if (gg <= 0) {
          return -1;
      }
      memcpy(buf, fd_[fd].f->content + fd_[fd].offset, gg);
      fd_[fd].offset += gg;
      return gg;
  } else {
      return -1;
  }
}

off_t rseek(int fd, off_t offset, int whence) {
  // TODO();
  if ((fd_[fd].f != NULL) && (fd_[fd].f->type == FILE_NODE)) {
      if (whence == SEEK_SET) {
          if (offset >= 0) {
              fd_[fd].offset = offset;
              return offset;
          } else {
              return -1;
          }
      }
      if (whence == SEEK_CUR) {
          if (offset + fd_[fd].offset >= 0) {
              fd_[fd].offset += offset;
              return fd_[fd].offset;
          } else {
              return -1;
          }
      }
      if (whence == SEEK_END) {
          if (offset + fd_[fd].f->size - 1 >= 0) {
              fd_[fd].offset = offset + fd_[fd].f->size;
              return fd_[fd].offset;
          } else {
              return -1;
          }
      }
      return -1;
  } else {
      return -1;
  }
}

int rmkdir(const char *pathname) {
  // TODO();
    int JudgeResult = JudgeOfPathname(pathname);
    if ((JudgeResult == -1) || (JudgeResult == -2)) {
        (all->nrde)++;
        node *new = malloc(sizeof (*new));
        new->type = DIR_NODE;
        new->dirents = NULL;
        new->content = NULL;
        new->nrde = 0;
        new->size = -1;
        int cc = -1;
        for (int j = 0; j < strlen(pathname) - 1; j++) {
            if (pathname[j] == '/' && pathname[j + 1] != '/') {
                cc = j + 1;
            }
        }
        int dd = -1;
        for (int j = strlen(pathname) - 1; j >= cc; j--) {
            if (pathname[j] != '/') {
                dd = j;
                break;
            }
        }
        new->name = malloc(dd - cc + 2);
        for (int j = cc; j <= dd; j++) {
            new->name[j - cc] = pathname[j];
        }
        new->name[dd - cc + 1] = '\0';
        new->same_level = all->dirents;
        all->dirents = new;
        all = &root;
        pre = NULL;
        return 0;
    } else {
        all = &root;
        pre = NULL;
        return -1;
    }
}

int rrmdir(const char *pathname) {
    // TODO();
    if (JudgeOfPathname(pathname) == 2) {
        if (all->nrde == 0) {
            int ss = 1;
            node *up = pre->dirents;
            if (strcmp(up->name, all->name) == 0) {
                up = pre;
                ss = 0;
            }
            if (ss == 1) {
                for (int i = 0; i < pre->nrde - 1; i++) {
                    if (strcmp(up->same_level->name, all->name) == 0) {
                        break;
                    } else {
                        up = up->same_level;
                    }
                }
            }
            if (ss == 0) {
                up->dirents = all->same_level;
            } else {
                up->same_level = all->same_level;
            }
            free(all->name);
            free(all);
            (pre->nrde)--;
            all = &root;
            pre = NULL;
            return 0;
        } else {
            all = &root;
            pre = NULL;
            return -1;
        }
    } else {
        all = &root;
        pre = NULL;
        return -1;
    }
}

int runlink(const char *pathname) {
      // TODO();
      if (JudgeOfPathname(pathname) == 1) {
          int ss = 1;
          node *up = pre->dirents;
          if (strcmp(up->name, all->name) == 0) {
              up = pre;
              ss = 0;
          }
          if (ss == 1) {
              for (int i = 0; i < pre->nrde - 1; i++) {
                  if (strcmp(up->same_level->name, all->name) == 0) {
                      break;
                  } else {
                      up = up->same_level;
                  }
              }
          }
          if (ss == 0) {
              up->dirents = all->same_level;
          } else {
              up->same_level = all->same_level;
          }
          if (all->size != 0) {
              free(all->content);
          }
          free(all);
          (pre->nrde)--;
          all = &root;
          pre = NULL;
          return 0;

      } else {
          all = &root;
          pre = NULL;
          return -1;
      }
  }

void init_ramfs() {
  // TODO();
  root.type = DIR_NODE;
  root.dirents = NULL;
  root.content = NULL;
  root.nrde = 0;
  root.size = -1;
  root.name = "/";
  root.same_level = NULL;
  for (int i = 0; i < 4097; i++) {
      fd_[i].offset = -1;
      fd_[i].flags = -1;
      fd_[i].f = NULL;
  }
}
