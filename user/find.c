#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

struct frame {
  char *path;
  struct dirent *entries;
  int count;
  int index;
  struct frame *parent;
};

static void
fail(const char *reason, const char *path)
{
  fprintf(2, "find: %s %s\n", reason, path);
  exit(1);
}

static void *
allocate(uint size)
{
  void *p = malloc(size);
  if (p == 0) {
    fprintf(2, "find: out of memory\n");
    exit(1);
  }
  return p;
}

// Only '*' is special; every other character matches literally.
static int
matches(const char *name, const char *pattern)
{
  const char *star = 0;
  const char *retry = 0;

  while (*name) {
    if (*pattern == '*') {
      star = pattern++;
      retry = name;
    } else if (*pattern == *name) {
      pattern++;
      name++;
    } else if (star) {
      pattern = star + 1;
      name = ++retry;
    } else {
      return 0;
    }
  }

  while (*pattern == '*')
    pattern++;
  return *pattern == '\0';
}

static char *
join(const char *dir, const char *name)
{
  int n = strlen(dir);
  char *path = allocate(n + strlen(name) + 2);

  strcpy(path, dir);
  if (n == 0 || path[n - 1] != '/')
    path[n++] = '/';
  strcpy(path + n, name);
  return path;
}

// Read this directory into heap memory, then close its descriptor.
static struct frame *
read_directory(char *path, struct frame *parent)
{
  struct stat st;
  int fd = open(".", O_RDONLY);
  if (fd < 0)
    fail("cannot open", path);
  if (fstat(fd, &st) < 0)
    fail("cannot stat", path);

  struct frame *f = allocate(sizeof(*f));
  f->path = path;
  f->parent = parent;
  f->count = st.size / sizeof(struct dirent);
  f->index = 0;
  f->entries = allocate(st.size ? st.size : 1);

  int n = read(fd, f->entries, st.size);
  close(fd);
  if (n < 0 || (uint64)n != st.size)
    fail("cannot read", path);

  return f;
}

int
main(int argc, char *argv[])
{
  char filter = 0;

  if (argc != 3 && argc != 5) {
    fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
    exit(1);
  }

  if (argc == 5) {
    if (strcmp(argv[3], "-type") != 0 ||
        (strcmp(argv[4], "f") != 0 && strcmp(argv[4], "d") != 0)) {
      fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
      exit(1);
    }
    filter = argv[4][0];
  }

  if (chdir(argv[1]) < 0)
    fail("cannot open", argv[1]);

  char *root = allocate(strlen(argv[1]) + 1);
  strcpy(root, argv[1]);
  struct frame *top = read_directory(root, 0);

  while (top) {
    if (top->index == top->count) {
      struct frame *done = top;
      top = top->parent;
      if (top && chdir("..") < 0)
        fail("cannot open", top->path);
      free(done->entries);
      free(done->path);
      free(done);
      continue;
    }

    struct dirent *de = &top->entries[top->index++];
    if (de->inum == 0)
      continue;

    char name[DIRSIZ + 1];
    memmove(name, de->name, DIRSIZ);
    name[DIRSIZ] = '\0';
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    char *path = join(top->path, name);
    struct stat st;
    if (stat(name, &st) < 0)
      fail("cannot stat", path);

    int right_type = filter == 0 ||
      (filter == 'd' && st.type == T_DIR) ||
      (filter == 'f' && (st.type == T_FILE || st.type == T_DEVICE));

    if (right_type && matches(name, argv[2]))
      printf("%s\n", path);

    if (st.type == T_DIR) {
      if (chdir(name) < 0)
        fail("cannot open", path);
      top = read_directory(path, top);
    } else {
      free(path);
    }
  }

  exit(0);
}

