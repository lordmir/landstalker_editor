EXEC=target
CC=g++
LDFLAGS= `wx-config --libs xrc,propgrid,aui,adv,core,base` -lpng
CXXFLAGS= `wx-config --cxxflags` -std=c++17 -I./third_party
CPPFLAGS = `wx-config --cppflags` -I./third_party
TARGET    := $(notdir $(CURDIR))
SOURCEDIR := .
SOURCE := $(foreach DIR,$(SOURCEDIR),$(wildcard $(DIR)/*.cpp))

DEBUG=no
ifeq ($(DEBUG),yes)
    CXXFLAGS += -g
endif

all: $(EXEC)

target: $(OBJ)
	$(CC) $(SOURCE) -o $(TARGET) $^ $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS)

.PHONY: clean

clean:
	rm -rf *.o
