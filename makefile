CC=g++
CFLAGS=-std=c++11
LIBS=-lpthread -lwiringPi

ifeq ($(DEBUG), 1)
	CFLAGS+=-DDEBUG -g -Wall
endif

demo:	MCP2510.cpp MCP2510.h CanUtil.cpp CanUtil.h demo.cpp
	g++ demo.cpp MCP2510.cpp CanUtil.cpp -o demo.out $(LIBS)

prod:	MCP2510.cpp MCP2510.h CanUtil.cpp CanUtil.h main_dev.cpp
	g++ main_dev.cpp MCP2510.cpp CanUtil.cpp -o dev.out $(LIBS) $(CFLAGS)
	sudo chown root dev.out && sudo chmod u+s dev.out && mv dev.out CAN_HANDLER

all: prod demo

clean:
	rm -f *.out *.o CAN_HANDLER
