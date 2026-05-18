# Makefile for fax gateway system (Linux)

CXX = g++
CC = gcc
CXXFLAGS = -g -Wall -Wno-write-strings -Wno-unused-result -D_GNU_SOURCE
LDFLAGS = -lpthread -lrt

TARGET = faxgw

SRCS = faxgw.cpp \
       TcpSession.cpp \
       tcppacket.cpp \
       packet.cpp \
       FileSession.cpp \
       Log.cpp \
       rwini.cpp \
       CRC32.CPP \
       LZW.cpp \
       d3des.cpp \
       md5.cpp \
       linux_compat.cpp \
       StdAfx.cpp \
       userdata_stub.cpp

OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.CPP=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.CPP
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

depend:
	$(CXX) -MM $(CXXFLAGS) $(SRCS) > .depend

-include .depend

.PHONY: all clean depend
