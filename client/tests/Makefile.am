AM_CPPFLAGS = -I$(top_srcdir)/src

noinst_PROGRAMS = hwclient
hwclient_SOURCES = hwclient.c
hwclient_LDADD = $(top_srcdir)/src/libeasyudp.la

noinst_PROGRAMS += hwserver
hwserver_SOURCES = hwserver.c
hwserver_LDADD = $(top_srcdir)/src/libeasyudp.la

noinst_PROGRAMS += bootstrap_test
bootstrap_test_SOURCES = bootstrap_test.c
bootstrap_test_LDADD = $(top_srcdir)/src/libbootstrap.la

noinst_PROGRAMS += hwsource
hwsource_SOURCES = hwsource.c
hwsource_LDADD = $(top_srcdir)/src/libsourcenode.la

noinst_PROGRAMS += hwfollower
hwfollower_SOURCES = hwfollower.c
hwfollower_LDADD = $(top_srcdir)/src/libfollowernode.la
