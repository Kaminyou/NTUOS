#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "errno.h"

int dir_num = 0;
int file_num = 0;

int
cnt_char_in_path(char *path, char target)
{
  char *p;
  int cnt = 0;
  for (p = path; p < path + strlen(path); p++) {
      if (*p == target) cnt++;
  }
  return cnt;
}

void
traverse(char* path, char *target)
{
  int fd;
  struct dirent de;
  struct stat st;
  char buf[512];
  char *p;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "%s [error opening dir]\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "%s [error opening dir]\n", path);
    close(fd);
    return;
  }
  
  if (st.type != T_DIR) {
    fprintf(2, "%s [error opening dir]\n", path);
    close(fd);
    return;
  }

  // print itself
  fprintf(2, "%s %d\n", path, cnt_char_in_path(path, *target));

  // move p to the point after appended '/'
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  // traverse dir
  while(read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0 || !strcmp(de.name, ".") || !strcmp(de.name, "..")) {
      continue;
    }

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    stat(buf, &st);

    if (st.type == T_DIR) {
      dir_num++;
      traverse(buf, target);
    }
    else if (st.type == T_FILE) {
      file_num++;
      fprintf(2, "%s %d\n", buf, cnt_char_in_path(path, *target));
    }
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int dir_num_parent;
  int file_num_parent;
  int fd[2];
  pipe(fd);

  int pid = fork();
  if (pid > 0) {
    read(fd[0], &dir_num_parent, sizeof(int));
    read(fd[0], &file_num_parent, sizeof(int));
    fprintf(2, "\n%d directories, %d files\n", dir_num_parent, file_num_parent);
  } 
  else {
    traverse(argv[1], argv[2]);
    write(fd[1], &dir_num, sizeof(int));
    write(fd[1], &file_num, sizeof(int));
  }

  close(fd[0]);
  close(fd[1]);

  exit(0);
}