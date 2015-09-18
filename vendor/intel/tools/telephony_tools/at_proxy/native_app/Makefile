OBJECTS = proxy_main.o serial.o request.o atp_util.o proxythread.o

INC_FLAGS = \
-I ../../../../../hardware/libhardware_legacy/include/


CFLAGS	= -g -Wall -static -m32 $(INC_FLAGS)

.PHONY:all
all:proxy

proxy:$(OBJECTS)
	$(CC) $(CFLAGS) -o proxy $(OBJECTS) -lpthread

.PHONY:clean
clean:
	-rm proxy $(OBJECTS)

