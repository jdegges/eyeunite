#include "bootstrap.h"
#include "debug.h"

#include <stdlib.h>

FILE *DEBUGFP = NULL;

int
main (void)
{
  struct bootstrap *bs;

  bootstrap_global_init ();

  bs = bootstrap_init ("http://eyeunite.appspot.com", 0);
  if (NULL == bs) {
    print_error ("bootstrap_init");
    return 1;
  }

  if (bootstrap_lobby_create (bs)) {
    print_error ("bootstrap_lobby_create");
    return 1;
  }

  bootstrap_global_cleanup ();

  return 0;
}
