#CC = clang
#LD = clang
CC = gcc
LD = gcc
AR = ar
RM = rm -f
STRIP = strip
INSTALL = install

ETHERLAB_MASTER_PATH = /opt/etherlab
INCLUDE = -Iinclude -I$(ETHERLAB_MASTER_PATH)/include
LIB_PATH =  -Llib -L$(ETHERLAB_MASTER_PATH)/lib

LIBRARY = -lrt -lpthread -lethercat -lncurses -ltinfo

CFLAGS = -Wall --std=c99 -g -O2 $(INCLUDE) -D_XOPEN_SOURCE -D_BSD_SOURCE -D_GNU_SOURCE
LDFLAGS = -g -Wall -Wextra 
LIBS_D += -g -Wall -Wextra -lncurses
ARFLAGS = rcs

VERSION = $(shell cat VERSION)
CFLAGS += -DVERSIONING=$(VERSION)

# the low-level eterhcat daemon main
TARGET = sncnllecat
OBJECTS = ./src/main.o

# the low-level ethercat module, the lib is currently not needed
TARGETDIR = lib
TARGETLIB = libethercat_wrapper
LIBOBJECTS = src/ethercat_wrapper.o src/slave.o

# test main for the low-level ethercat module
TESTECAT = testlib
TESTOBJECTS = src/testlib.o

PREFIX = /usr/local

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^

all: $(TARGETLIB).a

lib:
	@mkdir lib

include:
	@mkdir include

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS) $(LIB_PATH) $(LIBRARY)

$(TESTECAT): CFLAGS += -DDEBUG_APP
$(TESTECAT): $(TARGETLIB).a $(TESTOBJECTS)
	$(LD) -o $@ $(TESTOBJECTS) $(LDFLAGS) -static  $(LIB_PATH) -lethercat_wrapper $(LIBRARY)

$(TARGETLIB).a: include lib $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $(TARGETDIR)/$@ $(LIBOBJECTS)

.PHONY: clean install release

release: $(TARGETLIB)-$(VERSION).tar.gz

$(TARGETLIB)-$(VERSION).tar.gz:
	@tar cfz $@ ../ethercat_wrapper/Makefile ../ethercat_wrapper/VERSION ../ethercat_wrapper/README  ../ethercat_wrapper/src ../ethercat_wrapper/include

install: install_lib

install_daemon:
	$(INSTALL) bin/$(TARGET) $(PREFIX)/bin

install_lib:
	$(INSTALL) lib/$(TARGETLIB).a $(PREFIX)/lib
	$(INSTALL) include/ethercat_wrapper.h include/ethercat_wrapper_slave.h $(PREFIX)/include

clean:
	$(RM) $(OBJECTS) $(LIBOBJECTS) $(TESTOBJECTS) ${TESTECAT} $(TARGETDIR)/$(TARGETLIB).a $(TARGET) $(TARGETLIB)-*.tar.gz
