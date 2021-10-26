CC=gcc
LD=gcc
CFLAGS=-g -Wall 
CPPFLAGS=-I. -I include
SP_LIBRARY= ./libspread-core.a  ./libspread-util.a

all: sp_user class_user

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

sp_user:  user.o
	$(LD) -o $@ user.o -ldl $(SP_LIBRARY)

class_user:  class_user.o
	$(LD) -o $@ class_user.o -ldl $(SP_LIBRARY)

clean:
	rm -f *.o sp_user class_user

