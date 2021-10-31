CC=g++
LD=g++
CFLAGS=-g -Wall
#CFLAGS= -std=c++11 -c -Ofast -march=native -flto -Wall -DNDEBUG -frename-registers -funroll-loops # TODO: change to this one after debugging
CPPFLAGS=-I. -I include
SP_LIBRARY= ./libspread-core.a  ./libspread-util.a

all: sp_user class_user mcast

.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

sp_user:  user.o
	$(LD) -o $@ user.o -ldl $(SP_LIBRARY)

class_user:  class_user.o
	$(LD) -o $@ class_user.o -ldl $(SP_LIBRARY)

mcast: mcast.o 
	$(LD) -o $@ mcast.o -ldl $(SP_LIBRARY)

mcast.o: mcast.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

clean:
	rm -f *.o sp_user class_user mcast