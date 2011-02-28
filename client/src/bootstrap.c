#define _BSD_SOURCE

#include "bootstrap.h"
#include "debug.h"
#include "easyudp.h"
#include "eyeunite.h"

#include <curl/curl.h>
#include <libxml/parser.h>
#include <stdio.h>
#include <string.h>

struct buffer
{
  char *data;
  size_t pos;
  size_t len;
};

struct bootstrap_peer
{
  char pid[EU_TOKENSTRLEN];
  char addr[EU_ADDRSTRLEN];
  uint16_t port;
};

struct bootstrap
{
  CURL *curl;
  struct buffer buf;
  char *host;
  struct bootstrap_peer bsp;
  char lobby_token[EU_TOKENSTRLEN];
};

enum response_type
{
  NEW_PEER,
  LOBBY_CREATE,
  LOBBY_JOIN,
  LOBBY_LIST,
  LOBBY_LEAVE
};

void
bootstrap_global_init (void)
{
  curl_global_init (CURL_GLOBAL_ALL);
}

void
bootstrap_global_cleanup (void)
{
  curl_global_cleanup ();
}

static int
expand_buffer (struct buffer *buf, size_t size)
{
  if (buf->len - buf->pos < size) {
    size_t new_len = buf->len + size;
    if (new_len < buf->len * 2)
      new_len = buf->len * 2;

    buf->data = realloc (buf->data, new_len);
    if (NULL == buf->data) {
      print_error ("out of memory");
      return -1;
    }

    buf->len = new_len;
  }

  return 0;
}

static size_t
curl_writefunction (void *ptr, size_t size, size_t nmemb, void *stream)
{
  struct buffer *buf = stream;

  if (NULL == ptr || NULL == buf)
    return -1;

  if (0 == size * nmemb)
    return 0;

  if (expand_buffer (buf, size * nmemb)) {
    print_error ("expand_buffer");
    return -1;
  }

  memcpy (buf->data, ptr, size * nmemb);
  buf->pos += size * nmemb;

  return size * nmemb;
}

#define parse_option(d, c, v, s, k) { \
  if (!xmlStrcmp ((c)->name, (const xmlChar *) (k))) { \
    xmlChar *key = xmlNodeListGetString ((d), (c)->xmlChildrenNode, 1); \
    strncpy ((v), (const char *) key, (s)); \
    xmlFree (key); \
  } \
}

static int
parse_peer (xmlDocPtr doc, xmlNodePtr cur, struct bootstrap_peer *bsp)
{
  char port[EU_PORTSTRLEN];

  if (NULL == doc || NULL == cur || NULL == bsp) {
    print_error ("invalid arguments");
    return -1;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    parse_option (doc, cur, bsp->pid, EU_TOKENSTRLEN, "pid");
    parse_option (doc, cur, bsp->addr, EU_ADDRSTRLEN, "ip");
    parse_option (doc, cur, port, EU_PORTSTRLEN, "port");
    cur = cur->next;
  }

  if (0 == bsp->pid[0] || 0 == bsp->addr[0] || 0 == port[0]) {
    print_error ("incomplete <peer> document");
    return -1;
  }

  bsp->port = strtoul (port, NULL, 10);

  return 0;
}

static int
parse_lid (xmlDocPtr doc, xmlNodePtr cur, char *lobby_token)
{
  if (NULL == doc || NULL == cur || NULL == lobby_token) {
    print_error ("invalid arguments");
    return -1;
  }

  parse_option (doc, cur, lobby_token, EU_TOKENSTRLEN, "lid");

  if (0 == lobby_token[0]) {
    print_error ("incomplete <lid>");
    return -1;
  }

  return 0;
}

static int
parse_response (xmlDocPtr doc, xmlNodePtr cur, char *lobby_token,
                struct bootstrap_peer *bsp, size_t nmemb,
                enum response_type rt)
{
  int lobby_count = 0;
  int peer_count = 0;
  size_t umemb = 0;

  if (NULL == doc || NULL == cur) {
    print_error ("error: invalid arguments");
    return -1;
  }

  if (NEW_PEER != rt && NULL == lobby_token) {
    print_error ("error: invalid arguments");
    return -1;
  }

  if (LOBBY_LIST == rt && (NULL == bsp || 0 == nmemb)) {
    print_error ("error: invalid arguments");
    return -1;
  }

  if (NEW_PEER != rt && LOBBY_CREATE != rt && LOBBY_JOIN != rt
   && LOBBY_LIST != rt && LOBBY_LEAVE != rt) {
    print_error ("invalid response type");
    return -1;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if (!xmlStrcmp (cur->name, (const xmlChar *) "lid")) {
      if (NEW_PEER == rt) {
        print_error ("invalid response: got <lid>, expected <peer>");
        return -1;
      }

      if (lobby_count++) {
        print_error ("invalid response: got multiple <lid>");
        return -1;
      }

      if (parse_lid (doc, cur, lobby_token)) {
        print_error ("invalid response: could not parse <lid>");
        return -1;
      }
    } else if (!xmlStrcmp (cur->name, (const xmlChar *) "peer")) {
      if (LOBBY_LIST != rt && peer_count++) {
        print_error ("invalid response: got multiple <peer>");
        return -1;
      }

      if (nmemb <= umemb) {
        print_error ("invalid response: contains too many <peer>");
        return -1;
      }

      if (parse_peer (doc, cur, &bsp[umemb++])) {
        print_error ("invalid response: could not parse <peer>");
        return -1;
      }
    }

    cur = cur->next;
  }

  return 0;
}

#define ce(expr, ejmp) { \
  CURLcode rc = (expr); \
  if (rc) { \
    print_error ("%s", curl_easy_strerror (rc)); \
    goto ejmp; \
  } \
}

struct bootstrap *
bootstrap_init (char *host, uint16_t port, char *pid_token, char *addr)
{
  struct bootstrap *b = NULL;
  CURL *curl = NULL;
  CURLcode rc;
  size_t len;
  char *url = NULL;

  b = calloc (1, sizeof *b);
  if (NULL == b) {
    print_error ("out of memory");
    goto error;
  }

  curl = curl_easy_init ();
  if (NULL == curl) {
    print_error ("curl_easy_init");
    goto error;
  }

  /* construct request url */
  len = snprintf (NULL, 0, "%s/u?o=%u", host, port) + 1;
  b->buf.pos = 0;
  if (expand_buffer (&b->buf, len+0)) {
    print_error ("expand_buffer");
    goto error;
  }

  len = snprintf (b->buf.data, b->buf.len, "%s/u?o=%u", host, port);
  if (b->buf.len < len) {
    print_error ("absurd failure");
    goto error;
  }

  url = curl_easy_escape (curl, b->buf.data, 0);
  if (NULL == url) {
    print_error ("curl_easy_escape");
    goto error;
  }

  /* setup curl */
  ce (curl_easy_setopt (curl, CURLOPT_URL, b->buf.data), error);
  ce (curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curl_writefunction),
      error);
  ce (curl_easy_setopt (curl, CURLOPT_WRITEDATA, &b->buf), error);

  /* download response */
  ce (curl_easy_perform (curl), error);

  /* parse response */
  {
    xmlDocPtr doc;
    xmlNodePtr cur;

    doc = xmlParseMemory (b->buf.data, b->buf.pos);
    if (NULL == doc) {
      print_error ("xmlParseMemory");
      goto error;
    }

    cur = xmlDocGetRootElement (doc);
    if (NULL == cur) {
      print_error ("xmlDocGetRootElement");
      goto error;
    }

    if (xmlStrcmp (cur->name, (const xmlChar *) "eyeunite")) {
      print_error ("document of the wrong type, root node != eyeunite");
      goto error;
    }

    if (parse_response (doc, cur, NULL, &b->bsp, 1, NEW_PEER)) {
      print_error ("parse_peer");
      goto error;
    }

    if (port && b->bsp.port != port) {
      print_error ("server did accept my port request");
      goto error;
    }

    xmlFreeDoc (doc);
  }

  b->host = malloc (strlen (host) + 1);
  if (NULL == b->host) {
    print_error ("out of memory");
    goto error;
  }
  strcpy (b->host, host);

  /* temporary, should be removed */
  print_error ("pid:  %s", b->bsp.pid);
  print_error ("addr: %s", b->bsp.addr);
  print_error ("port: %u", b->bsp.port);

  if (pid_token) memcpy (pid_token, b->bsp.pid, EU_TOKENSTRLEN);
  if (addr) memcpy (addr, b->bsp.addr, EU_ADDRSTRLEN);

  curl_free (url);
  b->buf.pos = 0;
  b->curl = curl;

  return b;

error:
  if (url) curl_free (url);
  if (curl) curl_easy_cleanup (curl);
  if (b) free (b->buf.data);
  free (b);
  return NULL;
}

void
bootstrap_cleanup (struct bootstrap *b)
{
  xmlCleanupParser ();

  if (b) {
    free (b->lobby_token);
    free (b->host);
    free (b->buf.data);
    if (b->curl) curl_easy_cleanup (b->curl);
    free (b);
  }
}

int
bootstrap_lobby_create (struct bootstrap *b, char *lobby_token)
{
  size_t len;
  char *url;

  if (NULL == b || NULL == b->curl || NULL == b->host
   || NULL == lobby_token) {
    print_error ("error: invalid argument");
    goto error;
  }

  /* construct request url */
  b->buf.pos = 0;
  len = snprintf (b->buf.data, b->buf.len, "%s/n?p=%s&o=%u", b->host,
                  b->bsp.pid, b->bsp.port) + 1;
  if (len && expand_buffer (&b->buf, len)) {
    print_error ("expand_buffer");
    goto error;
  }

  len = snprintf (b->buf.data, b->buf.len, "%s/n?p=%s&o=%u", b->host,
                  b->bsp.pid, b->bsp.port);
  if (b->buf.len <= len) {
    print_error ("absurd failure");
    goto error;
  }

  url = curl_easy_escape (b->curl, b->buf.data, 0);
  if (NULL == url) {
    print_error ("curl_easy_escape");
    goto error;
  }

  /* setup curl */
  ce (curl_easy_setopt (b->curl, CURLOPT_URL, b->buf.data), error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEFUNCTION, curl_writefunction),
      error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEDATA, &b->buf), error);

  /* download response */
  ce (curl_easy_perform (b->curl), error);

  /* parse response */
  {
    xmlDocPtr doc;
    xmlNodePtr cur;
    struct bootstrap_peer bsp;

    doc = xmlParseMemory (b->buf.data, b->buf.pos);
    if (NULL == doc) {
      print_error ("xmlParseMemory");
      goto error;
    }

    cur = xmlDocGetRootElement (doc);
    if (NULL == cur) {
      print_error ("xmlDocGetRootElement");
      goto error;
    }

    if (xmlStrcmp (cur->name, (const xmlChar *) "eyeunite")) {
      print_error ("document of the wrong type, root node != eyeunite");
      goto error;
    }

    if (parse_response (doc, cur, b->lobby_token, &bsp, 1, LOBBY_CREATE)) {
      print_error ("parse_peer");
      goto error;
    }

    if (strncmp (b->bsp.pid, bsp.pid, EU_TOKENSTRLEN)
     || strncmp (b->bsp.addr, bsp.addr, EU_ADDRSTRLEN)
     || b->bsp.port != bsp.port) {
      print_error ("server did not process my request properly");
      goto error;
    }
  }

  /* temporary, should be removed */
  print_error ("lid:  %s", b->lobby_token);
  print_error ("pid:  %s", b->bsp.pid);
  print_error ("addr: %s", b->bsp.addr);
  print_error ("port: %u", b->bsp.port);

  memcpy (lobby_token, b->lobby_token, EU_TOKENSTRLEN);

  curl_free (url);
  b->buf.pos = 0;

  return 0;

error:
  return -1;
}

int
bootstrap_lobby_join (struct bootstrap *b, char *lobby_token)
{
  size_t len;
  char *url;

  if (NULL == b || NULL == b->curl || NULL == b->host
   || NULL == lobby_token) {
    print_error ("error: invalid argument");
    goto error;
  }

  /* construct request url */
  b->buf.pos = 0;
  len = snprintf (b->buf.data, b->buf.len, "%s/j?l=%s&p=%s&o=%u", b->host,
                  lobby_token, b->bsp.pid, b->bsp.port) + 1;
  if (len && expand_buffer (&b->buf, len)) {
    print_error ("expand_buffer");
    goto error;
  }

  len = snprintf (b->buf.data, b->buf.len, "%s/j?l=%s&p=%s&o=%u", b->host,
                  lobby_token, b->bsp.pid, b->bsp.port);
  if (b->buf.len <= len) {
    print_error ("absurd failure");
    goto error;
  }

  url = curl_easy_escape (b->curl, b->buf.data, 0);
  if (NULL == url) {
    print_error ("curl_easy_escape");
    goto error;
  }

  /* setup curl */
  ce (curl_easy_setopt (b->curl, CURLOPT_URL, b->buf.data), error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEFUNCTION, curl_writefunction),
      error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEDATA, &b->buf), error);

  /* download response */
  ce (curl_easy_perform (b->curl), error);

  /* parse response */
  {
    xmlDocPtr doc;
    xmlNodePtr cur;
    struct bootstrap_peer bsp;

    doc = xmlParseMemory (b->buf.data, b->buf.pos);
    if (NULL == doc) {
      print_error ("xmlParseMemory");
      goto error;
    }

    cur = xmlDocGetRootElement (doc);
    if (NULL == cur) {
      print_error ("xmlDocGetRootElement");
      goto error;
    }

    if (xmlStrcmp (cur->name, (const xmlChar *) "eyeunite")) {
      print_error ("document of the wrong type, root node != eyeunite");
      goto error;
    }

    if (parse_response (doc, cur, b->lobby_token, &bsp, 1, LOBBY_JOIN)) {
      print_error ("parse_peer");
      goto error;
    }

    if (strncmp (b->lobby_token, lobby_token, EU_TOKENSTRLEN)
     || strncmp (b->bsp.pid, bsp.pid, EU_TOKENSTRLEN)
     || strncmp (b->bsp.addr, bsp.addr, EU_ADDRSTRLEN)
     || b->bsp.port != bsp.port) {
      print_error ("server did not process my request properly");
      goto error;
    }
  }

  /* temporary, should be removed */
  print_error ("lid:  %s", b->lobby_token);
  print_error ("pid:  %s", b->bsp.pid);
  print_error ("addr: %s", b->bsp.addr);
  print_error ("port: %u", b->bsp.port);

  curl_free (url);
  b->buf.pos = 0;

  return 0;

error:
  return -1;
}

int
bootstrap_lobby_get_source (struct bootstrap *b, struct peer_info *pi)
{
  size_t len;
  char *url;

  if (NULL == b || NULL == b->curl || NULL == b->host
   || '\0' == b->lobby_token[0]
   || NULL == pi) {
    print_error ("error: invalid argument");
    goto error;
  }

  /* construct request url */
  b->buf.pos = 0;
  len = snprintf (b->buf.data, b->buf.len, "%s/l?l=%s", b->host,
                  b->lobby_token) + 1;
  if (len && expand_buffer (&b->buf, len)) {
    print_error ("expand_buffer");
    goto error;
  }

  len = snprintf (b->buf.data, b->buf.len, "%s/l?l=%s", b->host,
                  b->lobby_token);
  if (b->buf.len <= len) {
    print_error ("absurd failure");
    goto error;
  }

  url = curl_easy_escape (b->curl, b->buf.data, 0);
  if (NULL == url) {
    print_error ("curl_easy_escape");
    goto error;
  }

  /* setup curl */
  ce (curl_easy_setopt (b->curl, CURLOPT_URL, b->buf.data), error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEFUNCTION, curl_writefunction),
      error);
  ce (curl_easy_setopt (b->curl, CURLOPT_WRITEDATA, &b->buf), error);

  /* download response */
  ce (curl_easy_perform (b->curl), error);

  /* parse response */
  {
    xmlDocPtr doc;
    xmlNodePtr cur;
    struct bootstrap_peer bsp;
    char lobby_token[EU_TOKENSTRLEN];

    doc = xmlParseMemory (b->buf.data, b->buf.pos);
    if (NULL == doc) {
      print_error ("xmlParseMemory");
      goto error;
    }

    cur = xmlDocGetRootElement (doc);
    if (NULL == cur) {
      print_error ("xmlDocGetRootElement");
      goto error;
    }

    if (xmlStrcmp (cur->name, (const xmlChar *) "eyeunite")) {
      print_error ("document of the wrong type, root node != eyeunite");
      goto error;
    }

    if (parse_response (doc, cur, lobby_token, &bsp, 1, LOBBY_LIST)) {
      print_error ("parse_peer");
      goto error;
    }

    memcpy (pi->pid, bsp.pid, EU_TOKENSTRLEN);
    memcpy (pi->addr, bsp.addr, EU_ADDRSTRLEN);
    pi->port = bsp.port;
    pi->peerbw = -1;
  }

  /* temporary, should be removed */
  print_error ("lid:  %s", b->lobby_token);
  print_error ("pid:  %s", b->bsp.pid);
  print_error ("addr: %s", b->bsp.addr);
  print_error ("port: %u", b->bsp.port);

  curl_free (url);
  b->buf.pos = 0;

  return 0;

error:
  return -1;
}

int
bootstrap_lobby_leave (struct bootstrap *b);
