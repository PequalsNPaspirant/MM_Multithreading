SRCDIR := src
INCLUDEDIR := src
OBJDIR := obj/windows_gcc
OUTDIR := bin/windows_gcc
APP := $(shell echo `pwd` | sed 's/^.*\///g')
APP_NAME := $(OUTDIR)/$(APP)_gcc.exe
THIRD_PARTY_INCLUDE := -I../MM_CommonUtils/src -I../MM_UnitTestFramework/src

#LIBPATH := ../MM_CommonUtils/bin/windows_gcc
#LIBNAME := libMM_CommonUtils.a
#LDLIBS := MM_CommonUtils
LDFLAGS := -static -L$(LIBPATH) -l$(LDLIBS)

CXX := g++
CXXFLAGS_DEBUG := -std=c++1y -m64 -g -Wall
CXXFLAGS_WARN := -std=c++1y -m64 -Wall
CXXFLAGS_NO_WARN := -std=c++1y -m64 
CXXFLAGS := $(CXXFLAGS_NO_WARN)

SRCFILES := $(shell find $(SRCDIR) -name "*.cpp")
OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCFILES))

.PHONY: dependall clean distclean

all: $(APP_NAME)

#.depend: $(SRCFILES)
#	rm -rf $@
#	$(CXX) $(CXXFLAGS) -I$(INCLUDEDIR) $(THIRD_PARTY_INCLUDE) -MM $^ > $@ \
#	&& sed -zi 's/o: \\\n/o:/g' $@ \
#	&& sed -Ei 's#^(.*\.o: *)$(SRCDIR)/(.*/)?(.*\.cpp)#\n$(OBJDIR)/\2\1$(SRCDIR)/\2\3#g' $@
#include .depend

depend: $(SRCFILES)
	rm -rf .depend
	$(CXX) $(CXXFLAGS) -I$(INCLUDEDIR) $(THIRD_PARTY_INCLUDE) -MM $^ > .depend \
	&& sed -zi 's/o: \\\n/o:/g' .depend \
	&& sed -Ei 's#^(.*\.o: *)$(SRCDIR)/(.*/)?(.*\.cpp)#\n$(OBJDIR)/\2\1$(SRCDIR)/\2\3#g' .depend

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo ""
	@echo "========== Compiling $< =========="
	@mkdir -p $(dir $@)
	$(CXX) -o $@ -c $(CXXFLAGS) $< -I$(INCLUDEDIR) $(THIRD_PARTY_INCLUDE)

$(LIBPATH)/$(LIBNAME):
	cd ../MM_CommonUtils && make

$(APP_NAME): .depend $(OBJECTS) $(LIBPATH)/$(LIBNAME)
	@echo ""
	@echo "========== Linking =========="
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
	@echo ""
	@echo "========== SUCCESS =========="
	@echo ""

clean:
	rm -rf $(OBJECTS)
	
distclean:
	rm -rf .depend
	rm -rf $(OBJECTS)
	rm -rf $(APP_NAME)

	
##   This Makefile assumes that the project structure is as below:
##   
##   CurrentProject
##   │
##   ├── bin
##   │   └── windows_gcc
##   │       └── app_name or app_name.exe
##   │
##   ├── obj
##   │   └── windows_gcc
##   │       ├── main.o
##   │       ├── dir1
##   │       │   └── file1.o
##   │       └── dir2
##   │           ├── file2.o
##   │           └── file3.o
##   │
##   ├── src
##   │   ├── main.cpp
##   │   ├── dir1
##   │   │   ├── file1.h
##   │   │   └── file1.cpp
##   │   └── dir2
##   │       ├── file2.h
##   │       ├── file2.cpp
##   │       ├── file3.h
##   │       └── file3.cpp
##   │
##   ├── .depend
##   └── Makefile
##   	
	
##  More about .depend	
##	$(CXX) $(CXXFLAGS) -I$(INCLUDEDIR) $(THIRD_PARTY_INCLUDE) -MM $^ > $@ \
##	## Following line makes sure that the rule starting for a new object file has the .cpp file on the same line
##	&& sed -zi 's/o: \\\n/o:/g' $@ \
##	## This can also be done as below. But make sure there is no & character in existing text
##	## cat .depend | tr '\n' '&' | sed 's/o: \\&/o:/g' | sed 's/&/\n/g'
##	## Due to above .o and .cpp files are on same line, so we can steal the directory pattern for .cpp and apply it for .o
##	&& sed -Ei 's#^(.*\.o: *)$(SRCDIR)/(.*/)?(.*\.cpp)#\n$(OBJDIR)/\2\1$(SRCDIR)/\2\3#g' $@
	
##	If we have all .o files in the same directory obj/windows_gcc/*.o, then the following command creates and modified .depend file to prefix obj/windows_gcc/*.o:
##	$(CXX) $(CXXFLAGS) -I$(INCLUDEDIR) $(THIRD_PARTY_INCLUDE) -MM $^ > $@ && \
##	sed -Ei 's#(.*\.o:)#\n$(OBJDIR)/\1#g' $@
	
##More information on Makefile
## gcc [-c|-S|-E] [-std=standard] [-g] [-pg] [-Olevel] [-Wwarn...] [-pedantic] [-Idir...] [-Ldir...] [-Dmacro[=defn]...] [-Umacro] [-foption...] [-mmachine-option...] [-o outfile] [@file] infile...
## [-c|-S|-E] If any of these options is used, then the linker is not run, and object file names should not be used as arguments.
##
### Below option produces the effect #define name  value
##PREPROCESSOR_DEF := -Dname=value
## 
### Compile only
##COMPILE_ONLY := -c
### Output file (of compilation or of link)
##OUTPUT := -o
##
### Optimization
##LINK_FLAG_NORMAL := -O1
##LINK_FLAG_SPEED := -O3
##
### -MM : Print the header files (other than standard headers) used by each source file in a format acceptable to make. Don't produce a .o file or an executable.
##
### -L option tells the compiler where to find an external library
### -l (dash lowercase L) tells the compiler the name of the library
##
### -lMM_CommonUtils : Expecting the static lib libMM_CommonUtils.a
##
##
### While compiling source file to create .o file using below pattern, VPATH is used to search the source file in
###SRC_SEARCH_DIR := $(shell find . -maxdepth 2 -type d -name "*" | tr '\n' ' ')
###VPATH := $(SRC_SEARCH_DIR)
###$(OBJDIR)/%.o: %.cpp
##
##Other ways of notifying various paths to makefile executor:
###SRCFILES := $(shell find $(SRCDIR) -maxdepth 2 -name "?*.c*" | tr '\n' ' ')
##SRCFILES := $(shell find $(SRCDIR) -name "*.cpp")
###OBJECTS := $(shell find $(SRCDIR) -maxdepth 2 -name "?*.c*" | sed 's/.*\///g' | sed 's/\.c.*/\.o/g' | tr '\n' ' ')
###OBJECTS := $(shell find $(SRCDIR) -maxdepth 2 -name "?*.c*" | sed 's/\/.*\//\//g' | sed 's/^\.\//..\/obj\/windows_gcc\//g' | sed 's/\.c.*/\.o/g' | sed 's?$(SRCDIR)?$(OBJDIR)?g' | tr '\n' ' ')
##OBJECTS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCFILES))
###INCLUDEDIR := $(shell find $(SRCDIR) -maxdepth 2 -type d -name "*" | sed 's/^/-I/g' | tr '\n' ' ')





