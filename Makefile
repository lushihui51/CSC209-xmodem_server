PORT = 56945
CFLAGS = -D PORT=\$(PORT) -g -Wall
OBJ = xmodemserver.o helper.o crc16.o
DEPENDENCIES = xmodemserver.h crc16.h

all : xmodemserver

xmodemserver : ${OBJ}
	gcc ${CFLAGS} -o $@ $^

%.o : %.c ${DEPENDENCIES}
	gcc ${CFLAGS} -c $<

clean :
	rm -f *.o xmodemserver
