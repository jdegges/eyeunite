#include "bootstrap.h"
#include "debug.h"
#include "eyeunite.h"

#include <stdlib.h>

FILE *DEBUGFP = NULL;

int
main (void)
{
  struct bootstrap *bs;
  char pid_token[EU_TOKENSTRLEN];
  char lid_token[EU_TOKENSTRLEN];
  char addr[EU_ADDRSTRLEN];

  bootstrap_global_init ();

  bs = bootstrap_init ("http://eyeunite.appspot.com", 0, pid_token, addr);
  if (NULL == bs) {
    print_error ("bootstrap_init");
    return 1;
  }

  if (bootstrap_lobby_create (bs, lid_token)) {
    print_error ("bootstrap_lobby_create");
    return 1;
  }

  bootstrap_global_cleanup ();

  return 0;
}
