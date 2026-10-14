#include "kernel/types.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: sleep <ticks>\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  if (argc != 2 || argv[1][0] == '\0')
    usage();

  int ticks = 0;

  for (int i = 0; argv[1][i] != '\0'; i++) {
    if (argv[1][i] < '0' || argv[1][i] > '9')
      usage();

    int digit = argv[1][i] - '0';

    if (ticks > (2147483647 - digit) / 10)
      usage();

    ticks = ticks * 10 + digit;
  }

  if (pause(ticks) < 0) {
    fprintf(2, "sleep: pause failed\n");
    exit(1);
  }

  exit(0);
}

