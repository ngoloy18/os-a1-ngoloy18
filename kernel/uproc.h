#ifndef _UPROC_H_
#define _UPROC_H_

#include "types.h"

struct uproc {
  int pid;
  int ppid;
  int state;
  uint64 sz;
  char name[16];
};

#endif
