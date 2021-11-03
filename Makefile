CC=g++
LD=g++
#CFLAGS=-g -Wall
CFLAGS= -std=c++11 -c -Ofast -march=native -flto -Wall -DNDEBUG -frename-registers -funroll-loops # TODO: change to this one after debugging
CPPFLAGS=-I. -I include
SP_LIBRARY= ./libspread-core.a  ./libspread-util.a

all: mcast

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

mcast: mcast.o 
	$(LD) -o $@ mcast.o -ldl $(SP_LIBRARY)

mcast.o: mcast.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f *.o sp_user class_user mcast