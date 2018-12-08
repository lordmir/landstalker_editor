EXEC=target
CC=g++
LDFLAGS= `wx-config --libs xrc,propgrid,aui,adv,core,base`
CXXFLAGS= `wx-config --cxxflags` -std=c++11
CPPFLAGS = `wx-config --cppflags`
TARGET    := $(notdir $(CURDIR))
SOURCEDIR := .
SOURCE := $(foreach DIR,$(SOURCEDIR),$(wildcard $(DIR)/*.cpp))

all: $(EXEC)

target: $(OBJ)
	$(CC) $(SOURCE) -o $(TARGET) $^ $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS)

.PHONY: clean

clean:
	rm -rf *.o
