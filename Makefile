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

CROSS_COMPILE = arm-linux-
#CROSS_COMPILE =
CC	:= $(CROSS_COMPILE)gcc
INCLUDE	:= $(PWD)/include
CFLAGS	:= -Wall -O2 -g -I$(INCLUDE)
LDFLAGS	:= -lpthread -ljpeg

export CC

all: $(OBJS)
	@for dir in $(SUBDIRS) ; \
##		do $(MAKE) -C $$dir all; \
		do $(MAKE) -C $$dir all; \
	done

	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(APPS)

clean:
	rm -f $(APPS) $(OBJS)
	@for dir in $(SUBDIRS) ; \
		do $(MAKE) -C $$dir clean ; \
	done
