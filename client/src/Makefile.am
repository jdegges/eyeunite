AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = -avoid-version

noinst_LTLIBRARIES = libeasyudp.la
libeasyudp_la_SOURCES = easyudp.c

noinst_LTLIBRARIES += libbootstrap.la
libbootstrap_la_SOURCES = bootstrap.c
libbootstrap_la_LIBADD = -lcurl -lxml2
libbootstrap_la_CFLAGS = -I/usr/include/libxml2

noinst_LTLIBRARIES += libsourcenode.la
libsourcenode_la_SOURCES = sourcenode.c
libsourcenode_la_LIBADD = -lzmq
