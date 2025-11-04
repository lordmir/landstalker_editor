CXX	= g++
LD	= g++
MAKE = make
WXCONFIG	= wx-config

CXXFLAGS	= `$(WXCONFIG) --cxxflags` -std=c++2a -Wall -Wextra
CPPFLAGS	= `$(WXCONFIG) --cppflags`

EXEC		:= $(notdir $(CURDIR))
LIBS		:= `$(WXCONFIG) --libs xrc,propgrid,aui,adv,core,base,xml` -llandstalker -lpng -lyaml-cpp -lpugixml
SRCDIR  	:= ./src
BUILDDIR	:= build
BINDIR		:= bin
INC_DIRS	:= ./src/include ./modules/liblandstalker/landstalker/include
LIB_DIRS	:= ./modules/liblandstalker/build
INCS		:= $(SRCDIR) $(INC_DIRS)

SRC			:= $(foreach sdir,$(SRCDIR),$(wildcard $(sdir)/*/*.cpp))
OBJ			:= $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRC))
INCLUDES	:= $(addprefix -I,$(INCS))
LIBPATH		:= $(addprefix -L,$(LIB_DIRS))

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
	@$(MAKE) -C ./modules/liblandstalker clean

clean-all: clean

lib:
	@$(MAKE) -C ./modules/liblandstalker

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(EXEC): $(OBJ) $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(wildcard $(SRCDIR)/*.cpp)) lib
	$(LD) $(filter-out lib,$^) -o $@ $(LIBPATH) $(LIBS)
