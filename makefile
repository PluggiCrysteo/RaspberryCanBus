demo:	MCP2510.cpp MCP2510.h CanUtil.cpp CanUtil.h demo.cpp
	g++ demo.cpp MCP2510.cpp CanUtil.cpp -o demo -lwiringPi -lpthread

dev:	MCP2510.cpp MCP2510.h CanUtil.cpp CanUtil.h main_dev.cpp
	g++ main_dev.cpp MCP2510.cpp CanUtil.cpp -o dev -lwiringPi -lpthread -DDEBUG

all: dev demo
