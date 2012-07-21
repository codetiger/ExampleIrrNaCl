PROJECT:=testirrnacl
LDFLAGS:=Irrlicht/libIrrlicht.a -lnacl-mounts -lppapi -lppapi_cpp -lppapi_gles2 
CXX_SOURCES:=$(PROJECT).cc game.cc

THIS_MAKEFILE:=$(abspath $(lastword $(MAKEFILE_LIST)))
NACL_SDK_ROOT?=$(abspath $(dir $(THIS_MAKEFILE))../..)

WARNINGS:=-Wno-long-long -Wall
CXXFLAGS:=-pthread $(WARNINGS) -I../IrrNaCl/include

OSNAME:=win
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_newlib)
CXX:=$(TC_PATH)/bin/i686-nacl-g++

CYGWIN ?= nodosfilewarning
export CYGWIN

# Declare the ALL target first, to make the 'all' target the default build
all: $(PROJECT)_x86_32.nexe

# Define 32 bit compile and link rules for main application
x86_32_OBJS:=$(patsubst %.cc,%_32.o,$(CXX_SOURCES))
$(x86_32_OBJS) : %_32.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m32 -O3 $(CXXFLAGS)

$(PROJECT)_x86_32.nexe : $(x86_32_OBJS)
	$(CXX) -o $@ $^ -m32 -O3 $(CXXFLAGS) $(LDFLAGS)

x86_64_OBJS:=$(patsubst %.cc,%_64.o,$(CXX_SOURCES))
$(x86_64_OBJS) : %_64.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m64 -O3 $(CXXFLAGS)

$(PROJECT)_x86_64.nexe : $(x86_64_OBJS)
	$(CXX) -o $@ $^ -m64 -O3 $(CXXFLAGS) $(LDFLAGS)

# Define a phony rule so it always runs, to build nexe and start up server.
.PHONY: RUN 
RUN: all
	python ../httpd.py


