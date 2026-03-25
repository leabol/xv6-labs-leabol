#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  char c;
  char buf[1024];
  int buf_idx = 0;

  while (read(0, &c, 1) == 1) {
    if (buf_idx >= sizeof(buf) - 1) {
      fprintf(2, "xargs: line too long\n");
      exit(1);
    }

    buf[buf_idx++] = c;

    if (c == '\n') {
      buf[buf_idx] = '\0';  

      char *cmd_argv[MAXARG];
      int cmd_argc = 0;

      for (int i = 1; i < argc; i++) {
        cmd_argv[cmd_argc++] = argv[i];
      }

      char *p = buf;
      while (*p) {
        while (*p && (*p == ' ' || *p == '\t')) {
          p++;
        }

        if (!*p || *p == '\n') {
          break;  
        }

        char *word_start = p;

        while (*p && *p != ' ' && *p != '\t' && *p != '\n') {
          p++;
        }

        char sep = *p;
        *p = '\0';

        cmd_argv[cmd_argc++] = word_start;

        if (sep == '\0' || sep == '\n') {
          break;
        }
        p++;
      }

      cmd_argv[cmd_argc] = 0;

      if (fork() == 0) {
        exec(cmd_argv[0], cmd_argv);
        fprintf(2, "xargs: exec failed\n");
        exit(1);
      } else {
        wait(0);
      }

      buf_idx = 0;
    }
  }

  exit(0);
}