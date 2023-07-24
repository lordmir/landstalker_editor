CXX	= g++
LD	= g++
WXCONFIG	= wx-config

CXXFLAGS	= `$(WXCONFIG) --cxxflags` -std=c++17 -Wall -Wextra -pedantic
CPPFLAGS	= `$(WXCONFIG) --cppflags`

EXEC		:= $(notdir $(CURDIR))
LIBS		:= `$(WXCONFIG) --libs xrc,propgrid,aui,adv,core,base,xml` -lpng
SRCDIR  	:= src
BUILDDIR	:= build
BINDIR		:= bin
INC_DIRS	:= ./third_party
INCS		:= $(SRCDIR) $(addsuffix /include,$(SRCDIR)) $(INC_DIRS)

SRC		:= $(foreach sdir,$(SRCDIR),$(wildcard $(sdir)/*.cpp))
OBJ		:= $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRC))
INCLUDES	:= $(addprefix -I,$(INCS))

vpath %.cpp $(SRCDIR) $(EXEC_SDIR)

DEBUG=no
ifeq ($(DEBUG),yes)
    CXXFLAGS += -O0 -g
else
    CXXFLAGS += -O3 -DNDEBUG
endif

.PHONY: all checkdirs clean clean-all

all: checkdirs $(EXEC)

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILDDIR)

clean-all: clean
	@rm -rf $(EXEC)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(EXEC): $(OBJ) $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(wildcard $(SRCDIR)/*.cpp))
	$(LD) $^ -o $@ $(LIBS)

