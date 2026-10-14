#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXLINE 4096

static char line[MAXLINE];

int
run(char *cmd, char **fixed, int fixedc, char **words, int wordc)
{
  char *argv[MAXARG + 1];

  if(fixedc + wordc > MAXARG){
    fprintf(2, "xargs: too many arguments\n");
    return 1;
  }

  for(int i = 0; i < fixedc; i++)
    argv[i] = fixed[i];

  for(int i = 0; i < wordc; i++)
    argv[fixedc + i] = words[i];

  argv[fixedc + wordc] = 0;

  int pid = fork();
  if(pid < 0){
    fprintf(2, "xargs: fork failed\n");
    return 1;
  }

  if(pid == 0){
    exec(cmd, argv);
    fprintf(2, "xargs: exec failed\n");
    exit(1);
  }

  int status = 0;
  if(wait(&status) < 0)
    return 1;

  return status == 0 ? 0 : 1;
}

int
main(int argc, char *argv[])
{
  int limit = MAXARG;
  int grouped = 0;
  int start = 1;

  if(argc > 1 && strcmp(argv[1], "-n") == 0){
    if(argc < 4)
      goto usage;

    grouped = 1;
    limit = 0;

    for(char *p = argv[2]; *p; p++){
      if(*p < '0' || *p > '9')
        goto usage;
      limit = limit * 10 + (*p - '0');
    }

    if(limit < 1)
      goto usage;

    start = 3;
  }

  if(start >= argc)
    goto usage;

  char *cmd = argv[start];
  char **fixed = &argv[start];
  int fixedc = argc - start;

  if(fixedc > MAXARG)
    goto usage;

  int capacity = MAXARG - fixedc;
  if(capacity < 1){
    fprintf(2, "xargs: too many arguments\n");
    exit(1);
  }

  if(grouped && limit > capacity)
    limit = capacity;

  char *words[MAXARG];
  int count = 0;
  int used = 0;
  int inword = 0;
  int failed = 0;

  char ch;

  while(read(0, &ch, 1) == 1){
    if(ch == ' ' || ch == '\t' || ch == '\n'){
      if(inword){
        line[used++] = 0;
        inword = 0;

        if(grouped && count == limit){
          if(run(cmd, fixed, fixedc, words, count))
            failed = 1;
          count = 0;
          used = 0;
        }
      }

      if(!grouped && ch == '\n'){
        if(count > 0){
          if(run(cmd, fixed, fixedc, words, count))
            failed = 1;
        }
        count = 0;
        used = 0;
      }

      continue;
    }

    if(!inword){
      if(count == capacity){
        fprintf(2, "xargs: too many arguments\n");
        exit(1);
      }
      words[count++] = &line[used];
      inword = 1;
    }

    if(used >= MAXLINE - 1){
      fprintf(2, "xargs: input too long\n");
      exit(1);
    }

    line[used++] = ch;
  }

  if(inword){
    line[used++] = 0;
  }

  if(count > 0){
    if(run(cmd, fixed, fixedc, words, count))
      failed = 1;
  }

  exit(failed ? 1 : 0);

usage:
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
  exit(1);
}

