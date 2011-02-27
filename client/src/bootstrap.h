#ifndef EU_BOOTSTRAP_H
#define EU_BOOTSTRAP_H

#include "eyeunite.h"

#include <stdint.h>
#include <stdlib.h>

struct bootstrap;

/* bootstrap_global_init: must be called exactly once before any other
 *  bootstrap calls
 */
void
bootstrap_global_init (void);

/* bootstrap_global_cleanup: must be called exactly once after bootstrap is
 *  finished to completely close all internal state
 */
void
bootstrap_global_cleanup (void);

/* bootstrap_init: initializes internal state and gets a user token from the
 *  bootstrap node
 *  `host' should be the remote web server with protocol prefix.
 *         For example, http://eyeunite.appspot.com
 *  `port' is the UDP port you will be receiving data on
 *  `pid_token'  should point to a block of memory that is EU_TOKENSTRLEN bytes
 *               long and it will be filled in with your peer id token.
 *  Returns: NULL on failure
 */
struct bootstrap *
bootstrap_init (char *host, uint16_t port, char *pid_token);

void
bootstrap_cleanup (struct bootstrap *b);

/* bootstrap_lobby_create: create a new lobby
 *  `b' previously returned from a call to bootstrap_init()
 *  `lobby_token' should point to a block of memory that is EU_TOKENSTRLEN bytes
 *                long and it will be filled in with the token identifying your
 *                new lobby
 *  Returns: 0 on success
 */
int
bootstrap_lobby_create (struct bootstrap *b, char *lobby_token);

int
bootstrap_lobby_join (struct bootstrap *b, char *lobby_token);

int
bootstrap_lobby_get_source (struct bootstrap *b, struct peer_info *pi);

 int
bootstrap_lobby_leave (struct bootstrap *b);

#endif
