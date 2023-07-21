CFLAGS=-W -Wall -Wno-unused-parameter -g -O0 -Ivcd
LIBS=-lpthread
OBJS=main.o cp.o cm.o mc.o ptr.o vcd/vcd.o

#Do you have lots of disk space?
#CPPFLAGS+=-DDEBUG_VCD

RPI=$(shell grep -a "Raspberry Pi" /proc/device-tree/model 2>/dev/null)

ifeq ($(RPI),)
OBJS+=ddt-panel.o
else
OBJS+=rpi-panel.o
LIBS+=-lpigpio
endif

all: pdp9

$(OBJS):: defs.h vcd/vcd.h Makefile

pdp9: $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

vcd/vcd.h:
	git submodule update --init

clean:
	rm -f pdp9 $(OBJS)
