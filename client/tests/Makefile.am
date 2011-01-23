AM_CPPFLAGS = -I$(top_srcdir)/src

noinst_PROGRAMS = hwclient
hwclient_SOURCES = hwclient.c
hwclient_LDADD = $(top_srcdir)/src/libeasyudp.la

noinst_PROGRAMS += hwserver
hwserver_SOURCES = hwserver.c
hwserver_LDADD = $(top_srcdir)/src/libeasyudp.la
