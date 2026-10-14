#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

static struct uproc buf[NPROC];

static void
check(int condition, char *message)
{
  if(!condition){
    fprintf(2, "FAIL: %s\n", message);
    exit(1);
  }
}

int
main(void)
{
  check(getprocs(buf, -1) == -1,
        "negative max");
  printf("PASS: negative max\n");

  check(getprocs(buf, 0) == 0,
        "zero max");
  printf("PASS: zero max\n");

  check(getprocs(buf, 1) == 1,
        "small buffer");
  printf("PASS: small buffer\n");

  check(getprocs((struct uproc *)-1, 1) == -1,
        "invalid pointer");
  printf("PASS: invalid pointer\n");

  int n = getprocs(buf, NPROC);
  check(n > 0 && n <= NPROC,
        "process count");

  int found = 0;
  for(int i = 0; i < n; i++){
    if(buf[i].pid == getpid())
      found = 1;
  }
  check(found, "current process exists");
  printf("PASS: process information\n");

  // Four concurrent workers create and reap children.
  for(int i = 0; i < 4; i++){
    int pid = fork();
    check(pid >= 0, "worker fork");

    if(pid == 0){
      for(int j = 0; j < 20; j++){
        int child = fork();
        if(child < 0)
          exit(1);
        if(child == 0){
          pause(1);
          exit(0);
        }

        int status;
        if(wait(&status) != child || status != 0)
          exit(1);
      }
      exit(0);
    }
  }

  // Scan the process table while workers fork and exit.
  for(int i = 0; i < 150; i++){
    n = getprocs(buf, NPROC);
    check(n > 0 && n <= NPROC,
          "concurrent getprocs");

    if(i % 5 == 0)
      pause(1);
  }

  for(int i = 0; i < 4; i++){
    int status;
    check(wait(&status) > 0 && status == 0,
          "worker completion");
  }

  printf("PASS: fork/exit stress\n");
  printf("ALL GETPROCS TESTS PASSED\n");
  exit(0);
}
