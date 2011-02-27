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

noinst_LTLIBRARIES += libfollowernode.la
libfollowernode_la_SOURCES = followernode.c
libfollowernode_la_LIBADD = -lzmq

noinst_LTLIBRARIES += libtree.la
libtree_la_SOURCES = alpha_queue.c list.c tree.c 

bin_PROGRAMS = eyeunitesource
eyeunitesource_SOURCES = eyeunitesource.c
eyeunitesource_LDADD = $(top_srcdir)/src/libsourcenode.la
eyeunitesource_LDADD += $(top_srcdir)/src/libtree.la
eyeunitesource_LDADD += $(top_srcdir)/src/libbootstrap.la
