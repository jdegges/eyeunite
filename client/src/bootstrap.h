#ifndef EU_BOOTSTRAP_H
#define EU_BOOTSTRAP_H

#include <stdint.h>
#include <stdlib.h>

struct bootstrap;
struct bootstrap_peer;

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
 *  bootstrap node at host:port
 *  `host' should be the remote web server with protocol prefix.
 *         For example, http://eyeunite.appspot.com
 */
struct bootstrap *
bootstrap_init (const char *host, uint16_t port);

void
bootstrap_cleanup (struct bootstrap *b);

int
bootstrap_lobby_create (struct bootstrap *b);

int
bootstrap_lobby_join (struct bootstrap *b, const char *lobby_token);

/* bootstrap_lobby_list: get list of all peers connected to the lobby
 *  `peers' is an array of `nmemb' struct bootstrap_peer objects
 */
int
bootstrap_lobby_list (struct bootstrap *b, struct bootstrap_peer *peers,
                      size_t nmemb);

int
bootstrap_lobby_leave (struct bootstrap *b);

#endif
