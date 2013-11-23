#include "libguile.h"
#include "types.h"

#include "commands.h"

typedef struct {
  const char *name;
  int req;
  int opt;
  int rst;
  void *func;
} funcmap_t;


bool init_guile(const char *script);
bool call_guile_keypress(char key, bool ctrl, bool shift);
bool call_guile_buttonpress(unsigned int button, bool ctrl, int x, int y);
