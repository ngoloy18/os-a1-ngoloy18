#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc < 3){
    fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
    exit(1);
  }

  int mask = 0;

  for(char *p = argv[1]; *p; p++){
    if(*p < '0' || *p > '9'){
      fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
      exit(1);
    }
    mask = mask * 10 + (*p - '0');
  }

  if(trace(mask) < 0){
    fprintf(2, "trace: syscall failed\n");
    exit(1);
  }

  exec(argv[2], &argv[2]);

  fprintf(2, "trace: exec failed\n");
  exit(1);
}
