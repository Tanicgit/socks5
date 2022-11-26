#COMPLITE=arm-none-linux-gnueabi-

CC=$(COMPLITE)gcc
CXX=$(COMPLITE)g++
LD=$(COMPLITE)ld
CFLAGS=-I./
CPPFLAGS=-I./
LDFLAGS=-L./
#-Wall -fPIC -shared  -lrt

LIBS= -lpthread -lrt
STRIP=$(COMPLITE)strip

#
BUILD=build
CPP_FILES := $(wildcard *.cpp)
CPP_OBJFILE = $(addprefix $(BUILD)/,$(patsubst %.cpp, %.o, $(CPP_FILES)) )

C_FILES := $(wildcard *.c)
C_OBJFILE = $(addprefix $(BUILD)/,$(patsubst %.c, %.o, $(C_FILES)) )

bin=$(shell basename  $(shell pwd))
all:DBG $(bin) strip

strip:
	$(STRIP) $(bin)

DBG:
	$(shell if [ ! -d $(BUILD) ];then  mkdir -p  $(BUILD); fi;)
	echo $(C_OBJFILE)
	echo $(CPP_OBJFILE)

$(bin):$(C_OBJFILE) $(CPP_OBJFILE)
	$(CXX) $^ $(LIBS)  -o $@

$(CPP_OBJFILE) : $(BUILD)/%.o:%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(C_OBJFILE) : $(BUILD)/%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(CPP_OBJFILE) $(C_OBJFILE) $(bin)
	rm -f $(BUILD)/*
