#include "ramfs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* modify this file freely */

typedef struct node { //节点
    enum { FILE_NODE, DIR_NODE } type;
    struct node *dirents; // if it's a dir, there's subentries
    char *content; // if it's a file, there's data content
    int nrde; // number of subentries for dir
    int size; // size of file
    char *name; // it's short name
} node;

node root; //根文件

node *all = &root; //全局指针
node *pre;

typedef struct FD { //文件描述符
    int offset;
    int flags;
    node *f;
} FD;

FD fd_[4097]; //文件描述符的集合

int JudgeOfPathname(const char *pathname) { //判断pathname的合法性
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
            for (int i = 0; i < all->nrde; i++) {
                if (strcmp(name[0], all->dirents[i].name) == 0) {
                    if ((all->dirents[i].type == FILE_NODE) && (pathname[strlen(pathname) - 1] != '/')) {
                        pre = all;
                        all = &(all->dirents[i]);
                        return 1;
                    } else if (all->dirents[i].type == DIR_NODE) {
                        pre = all;
                        all = &(all->dirents[i]);
                        return 2;
                    } else {
                        return 0;
                    }
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
                for (int j = 0; j < all->nrde; j++) {
                    if (strcmp(name[i], all->dirents[j].name) == 0) {
                        pre = all;
                        all = &(all->dirents[j]);
                        aa = 1;
                        break;
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
      return bb;
  }
  /*
   * for (int i = 0; i < all->nrde; i++) {
              if (all->dirents[i] == NULL) {
                  node *new = malloc(sizeof *new);
                  if (new == NULL) {
                      printf("Error: malloc failed in Append\n");
                      all = &root;
                      return -1;
                  }
                  new->type = FILE_NODE;
                  for (int j = 0; j < 100; j++) {
                      new->dirents[j] = NULL;
                  }
                  new->content = malloc(sizeof new->content);
                  strcpy(new->content, "\0");
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
                  all->dirents[i] = new;
                  all = new;
                  break;
              }
          }
   */
  if (JudgeResult == -1) {
      if ((flags == O_CREAT) || (flags == (O_CREAT | O_RDONLY))) {
          (all->nrde)++;
          node *tem = realloc(all->dirents, (all->nrde) * sizeof(*tem));
          all->dirents = tem;
          all->dirents[all->nrde - 1].type = FILE_NODE;
          all->dirents[all->nrde - 1].dirents = NULL;
          all->dirents[all->nrde - 1].content = NULL;
          all->dirents[all->nrde - 1].nrde = -1;
          all->dirents[all->nrde - 1].size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          all->dirents[all->nrde - 1].name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              all->dirents[all->nrde - 1].name[j - cc] = pathname[j];
          }
          all->dirents[all->nrde - 1].name[strlen(pathname) - cc] = '\0';
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 0;
          all = &root;
          return dd;
      } else if (flags == (O_CREAT | O_WRONLY)) {
          (all->nrde)++;
          node *tem = realloc(all->dirents, (all->nrde) * sizeof(*tem));
          all->dirents = tem;
          all->dirents[all->nrde - 1].type = FILE_NODE;
          all->dirents[all->nrde - 1].dirents = NULL;
          all->dirents[all->nrde - 1].content = NULL;
          all->dirents[all->nrde - 1].nrde = -1;
          all->dirents[all->nrde - 1].size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          all->dirents[all->nrde - 1].name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              all->dirents[all->nrde - 1].name[j - cc] = pathname[j];
          }
          all->dirents[all->nrde - 1].name[strlen(pathname) - cc] = '\0';
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 1;
          all = &root;
          return dd;
      } else if (flags == (O_CREAT | O_RDWR)) {
          (all->nrde)++;
          node *tem = realloc(all->dirents, (all->nrde) * sizeof(*tem));
          all->dirents = tem;
          all->dirents[all->nrde - 1].type = FILE_NODE;
          all->dirents[all->nrde - 1].dirents = NULL;
          all->dirents[all->nrde - 1].content = NULL;
          all->dirents[all->nrde - 1].nrde = -1;
          all->dirents[all->nrde - 1].size = 0;
          int cc = -1;
          for (int j = 0; j < strlen(pathname) - 1; j++) {
              if (pathname[j] == '/' && pathname[j + 1] != '/') {
                  cc = j + 1;
              }
          }
          all->dirents[all->nrde - 1].name = malloc(strlen(pathname) - cc + 1);
          for (int j = cc; j < strlen(pathname); j++) {
              all->dirents[all->nrde - 1].name[j - cc] = pathname[j];
          }
          all->dirents[all->nrde - 1].name[strlen(pathname) - cc] = '\0';
          int dd = -1;
          for (int i = 0; i < 4097; i++) {
              if (fd_[i].f == NULL) {
                  fd_[i].f = all;
                  dd = i;
                  break;
              }
          }
          fd_[dd].offset = 0;
          fd_[dd].flags = 2;
          all = &root;
          return dd;
      } else {
          all = &root;
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
          fd_[ee].offset = (all->size) - 1;
          fd_[ee].flags = 0;
          all = &root;
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
          fd_[ee].offset = (all->size) - 1;
          fd_[ee].flags = 1;
          all = &root;
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
          fd_[ee].offset = (all->size) - 1;
          fd_[ee].flags = 2;
          all = &root;
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
          free(all->content);
          all->content = NULL;
          all = &root;
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
          free(all->content);
          all->content = NULL;
          all = &root;
          return ee;
      }
      all = &root;
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
  if ((fd_[fd].f != NULL) && (fd_[fd].f->type == FILE_NODE) && ((fd_[fd].flags == 1) || (fd_[fd].flags == 2))) {
      char *temp = malloc(count);
      if (count <= strlen(buf)) {
          memcpy(temp, buf, count);
      } else {
          memcpy(temp, buf, strlen(buf));
          for (int i = strlen(buf); i < count; i++) {
              temp[i] = '\0';
          }
      }
      if (fd_[fd].offset + count > fd_[fd].f->size) {
          char *tem = realloc(fd_[fd].f->content, fd_[fd].offset + count);
          for (int i = fd_[fd].offset; i < fd_[fd].offset + count; i++) {
              tem[i] = temp[i - fd_[fd].offset];
          }
          if (fd_[fd].offset > fd_[fd].f->size) {
              for (int i = fd_[fd].f->size; i < fd_[fd].offset; i++) {
                  tem[i] = '\0';
              }
          }
          fd_[fd].f->size = fd_[fd].offset + count;
          fd_[fd].f->content = tem;
          fd_[fd].offset = fd_[fd].f->size;
      } else {
          for (int i = fd_[fd].offset; i < fd_[fd].offset + count; i++) {
              fd_[fd].f->content[i] = temp[i - fd_[fd].offset];
          }
          fd_[fd].offset += count;
      }
      free(temp);
      return count;
  } else {
      return -1;
  }
}

ssize_t rread(int fd, void *buf, size_t count) {
  // TODO();
  if ((fd_[fd].f != NULL) && (fd_[fd].f->type == FILE_NODE) && ((fd_[fd].flags == 0) || (fd_[fd].flags == 2))) {
      int gg;
      if (fd_[fd].offset + count > fd_[fd].f->size) {
          gg = fd_[fd].f->size - fd_[fd].offset;
      } else {
          gg = count;
      }
      char *tem = malloc(gg * sizeof (char));
      for (int i = fd_[fd].offset; i < fd_[fd].offset + gg; i++) {
          tem[i - fd_[fd].offset] = fd_[fd].f->content[i];
      }
      memcpy(buf, tem, gg);
      fd_[fd].offset += gg;
      free(tem);
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
              fd_[fd].offset = offset + fd_[fd].f->size - 1;
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
        node *tem = realloc(all->dirents, (all->nrde) * sizeof(*tem));
        all->dirents = tem;
        all->dirents[all->nrde - 1].type = DIR_NODE;
        all->dirents[all->nrde - 1].dirents = NULL;
        all->dirents[all->nrde - 1].content = NULL;
        all->dirents[all->nrde - 1].nrde = 0;
        all->dirents[all->nrde - 1].size = -1;
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
        all->dirents[all->nrde - 1].name = malloc(dd - cc + 2);
        for (int j = cc; j <= dd; j++) {
            all->dirents[all->nrde - 1].name[j - cc] = pathname[j];
        }
        all->dirents[all->nrde - 1].name[dd - cc + 1] = '\0';
        all = &root;
        return 0;
    } else {
        all = &root;
        return -1;
    }
}

int rrmdir(const char *pathname) {
  // TODO();
  if (JudgeOfPathname(pathname) == 2) {
      if (all->nrde == 0) {
          int gg = -1;
          for (int i = 0; i < pre->nrde; i++) {
              if (strcmp(pre->dirents[i].name, all->name) == 0) {
                  gg = i;
                  break;
              }
          }
          if (pre->nrde > 1) {
              node *temp = malloc((pre->nrde - 1) * sizeof(node));
              for (int i = 0; i < gg; i++) {
                  temp[i].type = pre->dirents[i].type;
                  if (temp[i].type == DIR_NODE) {
                      if (pre->dirents[i].nrde != 0) {
                          temp[i].dirents = malloc(pre->dirents[i].nrde * sizeof(node));
                          memcpy(temp[i].dirents, pre->dirents[i].dirents, pre->dirents[i].nrde * sizeof(node));
                      } else {
                          temp[i].dirents = NULL;
                      }
                      temp[i].content = NULL;
                  } else if (temp[i].type == FILE_NODE) {
                      if (pre->dirents[i].size != 0) {
                          temp[i].content = malloc(pre->dirents[i].size * sizeof(char));
                          memcpy(temp[i].content, pre->dirents[i].content, pre->dirents[i].size * sizeof(char));
                      } else {
                          temp[i].content = NULL;
                      }
                      temp[i].dirents = NULL;
                  }
                  temp[i].nrde = pre->dirents[i].nrde;
                  temp[i].size = pre->dirents[i].size;
                  temp[i].name = malloc(strlen(pre->dirents[i].name) + 1);
                  memcpy(temp[i].name, pre->dirents[i].name, strlen(pre->dirents[i].name) + 1);
              }
              for (int i = gg + 1; i < pre->nrde; i++) {
                  temp[i - 1].type = pre->dirents[i].type;
                  if (temp[i - 1].type == DIR_NODE) {
                      if (pre->dirents[i].nrde != 0) {
                          temp[i - 1].dirents = malloc(pre->dirents[i].nrde * sizeof(node));
                          memcpy(temp[i - 1].dirents, pre->dirents[i].dirents, pre->dirents[i].nrde * sizeof(node));
                      } else {
                          temp[i - 1].dirents = NULL;
                      }
                      temp[i - 1].content = NULL;
                  } else if (temp[i - 1].type == FILE_NODE) {
                      if (pre->dirents[i].size != 0) {
                          temp[i - 1].content = malloc(pre->dirents[i].size * sizeof(char));
                          memcpy(temp[i - 1].content, pre->dirents[i].content, pre->dirents[i].size * sizeof(char));
                      } else {
                          temp[i - 1].content = NULL;
                      }
                      temp[i - 1].dirents = NULL;
                  }
                  temp[i - 1].nrde = pre->dirents[i].nrde;
                  temp[i - 1].size = pre->dirents[i].size;
                  temp[i - 1].name = malloc(strlen(pre->dirents[i].name) + 1);
                  memcpy(temp[i - 1].name, pre->dirents[i].name, strlen(pre->dirents[i].name) + 1);
              }
              for (int i = 0; i < pre->nrde; i++) {
                  if ((pre->dirents[i].type == FILE_NODE) && (pre->dirents[i].size != 0)) {
                      free(pre->dirents[i].content);
                  } else if ((pre->dirents[i].type == DIR_NODE) && (pre->dirents[i].nrde != 0)) {
                      free(pre->dirents[i].dirents);
                  }
                  free(pre->dirents[i].name);
              }
              free(pre->dirents);
              pre->dirents = malloc((pre->nrde - 1) * sizeof(node));
              memcpy(pre->dirents, temp, (pre->nrde - 1) * sizeof(node));
              for (int i = 0; i < pre->nrde - 1; i++) {
                  if ((temp[i].type == FILE_NODE) && (temp[i].size != 0)) {
                          free(temp[i].content);
                      } else if ((temp[i].type == DIR_NODE) && (temp[i].nrde != 0)) {
                      free(temp[i].dirents);
                  }
                  free(temp[i].name);
              }
              free(temp);
              (pre->nrde)--;
              all = &root;
              pre = NULL;
              return 0;
          } else if (pre->nrde == 1) {
              free(all->name);
              free(pre->dirents);
              pre->dirents = NULL;
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
  } else {
      all = &root;
      pre = NULL;
      return -1;
  }
}

int runlink(const char *pathname) {
  // TODO();
    if (JudgeOfPathname(pathname) == 1) {
        int gg = -1;
        for (int i = 0; i < pre->nrde; i++) {
            if (strcmp(pre->dirents[i].name, all->name) == 0) {
                gg = i;
                break;
            }
        }
        if (pre->nrde > 1) {
            node *temp = malloc((pre->nrde - 1) * sizeof(node));
            for (int i = 0; i < gg; i++) {
                temp[i].type = pre->dirents[i].type;
                if (temp[i].type == DIR_NODE) {
                    if (pre->dirents[i].nrde != 0) {
                        temp[i].dirents = malloc(pre->dirents[i].nrde * sizeof(node));
                        memcpy(temp[i].dirents, pre->dirents[i].dirents, pre->dirents[i].nrde * sizeof(node));
                    } else {
                        temp[i].dirents = NULL;
                    }
                    temp[i].content = NULL;
                } else if (temp[i].type == FILE_NODE) {
                    if (pre->dirents[i].size != 0) {
                        temp[i].content = malloc(pre->dirents[i].size * sizeof(char));
                        memcpy(temp[i].content, pre->dirents[i].content, pre->dirents[i].size * sizeof(char));
                    } else {
                        temp[i].content = NULL;
                    }
                    temp[i].dirents = NULL;
                }
                temp[i].nrde = pre->dirents[i].nrde;
                temp[i].size = pre->dirents[i].size;
                temp[i].name = malloc(strlen(pre->dirents[i].name) + 1);
                memcpy(temp[i].name, pre->dirents[i].name, strlen(pre->dirents[i].name) + 1);
            }
            for (int i = gg + 1; i < pre->nrde; i++) {
                temp[i - 1].type = pre->dirents[i].type;
                if (temp[i - 1].type == DIR_NODE) {
                    if (pre->dirents[i].nrde != 0) {
                        temp[i - 1].dirents = malloc(pre->dirents[i].nrde * sizeof(node));
                        memcpy(temp[i - 1].dirents, pre->dirents[i].dirents, pre->dirents[i].nrde * sizeof(node));
                    } else {
                        temp[i - 1].dirents = NULL;
                    }
                    temp[i - 1].content = NULL;
                } else if (temp[i - 1].type == FILE_NODE) {
                    if (pre->dirents[i].size != 0) {
                        temp[i - 1].content = malloc(pre->dirents[i].size * sizeof(char));
                        memcpy(temp[i - 1].content, pre->dirents[i].content, pre->dirents[i].size * sizeof(char));
                    } else {
                        temp[i - 1].content = NULL;
                    }
                    temp[i - 1].dirents = NULL;
                }
                temp[i - 1].nrde = pre->dirents[i].nrde;
                temp[i - 1].size = pre->dirents[i].size;
                temp[i - 1].name = malloc(strlen(pre->dirents[i].name) + 1);
                memcpy(temp[i - 1].name, pre->dirents[i].name, strlen(pre->dirents[i].name) + 1);
            }
            for (int i = 0; i < pre->nrde; i++) {
                if ((pre->dirents[i].type == FILE_NODE) && (pre->dirents[i].size != 0)) {
                    free(pre->dirents[i].content);
                } else if ((pre->dirents[i].type == DIR_NODE) && (pre->dirents[i].nrde != 0)) {
                    free(pre->dirents[i].dirents);
                }
                free(pre->dirents[i].name);
            }
            free(pre->dirents);
            pre->dirents = malloc((pre->nrde - 1) * sizeof(node));
            memcpy(pre->dirents, temp, (pre->nrde - 1) * sizeof(node));
            for (int i = 0; i < pre->nrde - 1; i++) {
                if ((temp[i].type == FILE_NODE) && (temp[i].size != 0)) {
                    free(temp[i].content);
                } else if ((temp[i].type == DIR_NODE) && (temp[i].nrde != 0)) {
                    free(temp[i].dirents);
                }
                free(temp[i].name);
            }
            free(temp);
            (pre->nrde)--;
            all = &root;
            pre = NULL;
            return 0;
        } else if (pre->nrde == 1) {
            free(all->name);
            if (all->size > 0) {
                free(all->content);
            }
            free(pre->dirents);
            pre->dirents = NULL;
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

void init_ramfs() {
  // TODO();
  root.type = DIR_NODE;
  root.dirents = NULL;
  root.content = NULL;
  root.nrde = 0;
  root.size = -1;
  root.name = "/";
  for (int i = 0; i < 4097; i++) {
      fd_[i].offset = -1;
      fd_[i].flags = -1;
      fd_[i].f = NULL;
  }
}
