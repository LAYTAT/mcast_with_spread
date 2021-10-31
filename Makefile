CC=g++
LD=g++
CFLAGS=-g -Wall
CPPFLAGS=-I. -I include
SP_LIBRARY= ./libspread-core.a  ./libspread-util.a

all: sp_user class_user mcast

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

sp_user:  user.o
	$(LD) -o $@ user.o -ldl $(SP_LIBRARY)

class_user:  class_user.o
	$(LD) -o $@ class_user.o -ldl $(SP_LIBRARY)

mcast: mcast.o Processor.o recv_dbg.o
	$(LD) -o $@ mcast.o Processor.o recv_dbg.o -ldl $(SP_LIBRARY)

mcast.o: mcast.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

Processor.o: Processor.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

recv_dbg.o: recv_dbg.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f *.o sp_user class_user mcast