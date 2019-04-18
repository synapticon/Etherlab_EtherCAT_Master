
lib_LTLIBRARIES = libethercat_wrapper.la

libethercat_wrapper_la_SOURCES = \
	src/slave.c \
	src/ethercat_wrapper.c

include_HEADERS = \
    include/commons.h \
	include/slave.h \
	include/ethercat_wrapper.h \
	include/ethercat_wrapper_slave.h

libethercat_wrapper_la_CFLAGS = -I.. -I../include -Iinclude --std=c99 -D_XOPEN_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE -DVERSIONING=@LIBETHERCAT_WRAPPERVERSION@
libethercat_wrapper_la_LDFLAGS =

noinst_PROGRAMS = testlib

testlib_SOURCES = src/testlib.c

testlib_CFLAGS = --std=c99 -I../include -Iinclude -g  -D_XOPEN_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE
testlib_LDFLAGS = -static -L.libs -L../lib/.libs -lethercat_wrapper  -lethercat

EXTRA_DIST = Version
