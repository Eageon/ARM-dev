LIBS	:= libtcpserver.a
INCLUDE	:= ../include

AOBJS	:= $(patsubst %.s, %.o, $(wildcard *.s))
COBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
CPPOBJS	:= $(patsubst %.cpp, %.o, $(wildcard *.cpp))
OBJS	:= $(AOBJS) $(COBJS) $(CPPOBJS)

CFLAGS  += -Wall -O2 -g -I$(INCLUDE)

all: $(OBJS)
	ar cr $(LIBS) $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(LIBS) *.h.gch

