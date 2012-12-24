SUBDIRS	:= gprs tcpserver alarm camera
.PHONY: $(SUBDIRS)

LIBS	:= $(PWD)/gprs/libgprs.a \
	$(PWD)/tcpserver/libtcpserver.a \
	$(PWD)/alarm/libalarm.a \
	$(PWD)/camera/libcamera.a \
#	$(PWD)/led_beep_ctl/libled_beep_ctl.a \

APPS	:= app
AOBJS	:= $(patsubst %.s, %.o, $(wildcard *.s))
COBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJS	:= $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OBJS	:= $(AOBJS) $(COBJS) $(CPPOBJS)

DEBUG 	:= g

CROSS_COMPILE = arm-linux-
#CROSS_COMPILE =
CC	:= $(CROSS_COMPILE)gcc
STDINC  := /usr/local/arm-linux-gnueabi/include
EXTRAINC:= /usr/local/arm-linux-gnueabi/lib/gcc/arm-linux-gnueabi/4.6/include
INCLUDE	:= $(PWD)/include
CFLAGS	:= -Wall -O2 -$(DEBUG) -nostdinc -I$(INCLUDE) -I$(STDINC) -I$(EXTRAINC) -march=armv5t
LDFLAGS	:= -L/usr/local/arm-linux-gnueabi/lib -L/usr/local/arm-linux-gnueabi/lib/gcc/arm-linux-gnueabi/4.6 -lpthread -ljpeg

export CC

all: $(OBJS)
	@for dir in $(SUBDIRS) ; \
		do $(MAKE) -C $$dir all; \
	done

	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(APPS)

clean:
	rm -f $(APPS) $(OBJS)
	@for dir in $(SUBDIRS) ; \
		do $(MAKE) -C $$dir clean ; \
	done
