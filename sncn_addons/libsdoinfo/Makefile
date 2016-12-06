CC = gcc
LD = gcc
AR = ar

CFLAGS = -g -Wall -Wextra --std=c99 -O0
LDFLAGS = -g -Wall -Wextra
ARFLAGS = rcs

INCLUDES = -I../../ -I../../include -I.

LIBOBJECTS = sdoinfo.o
OBJECTS = main.o

TARGETLIB = libsdoinfo.a
TARGET = testsdoinfo

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $(INCLUDES) $^

all: $(TARGETLIB) $(TARGET)

$(TARGETLIB): $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $(TARGETLIB) $(LIBOBJECTS)

$(TARGET): $(OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^ -L. -L/opt/etherlab/lib -Wl,-Bstatic -lethercat -lsdoinfo -Wl,-Bdynamic

.PHONY: clean

clean:
	@rm -f $(OBJETS) $(LIBOBJECTS) $(TARGETLIB) $(TARGET)
