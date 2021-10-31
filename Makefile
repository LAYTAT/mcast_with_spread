#CC=gcc
#LD=gcc
LD= g++
CC = g++
CFLAGS = -std=c++11 -c -Ofast -march=native -flto -Wall -DNDEBUG -frename-registers -funroll-loops
#CFLAGS=-g -Wall
CPPFLAGS=-I. -I include
SP_LIBRARY= libspread-core.a  libspread-util.a

all: sp_user class_user mcast

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

sp_user:  user.o
	$(LD) -o $@ user.o -ldl $(SP_LIBRARY)

class_user:  class_user.o
	$(LD) -o $@ class_user.o -ldl $(SP_LIBRARY)

mcast: mcast.o Processor.o recv_dbg.o
	$(CC) -o mcast mcast.o Processor.o recv_dbg.o

%.o: %.cpp
	$(CC) $(CFLAGS) $*.cpp

clean:
	rm -f *.o sp_user class_user

