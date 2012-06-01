# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
#   http://www.gnu.org/software/make/manual/make.html
#

#
# Project information
#
# These variables store project specific settings for the project name
# build flags, files to copy or install.  In the examples it is typically
# only the list of sources and project name that will actually change and
# the rest of the makefile is boilerplate for defining build rules.
#
PROJECT:=tumbler
LDFLAGS:=-lppapi_gles2 -lppapi_cpp -lppapi libs/libIrrlicht.so
CXX_SOURCES:=$(PROJECT).cc
CXX_SOURCES+=transforms.cc shader_util.cc opengl_context.cc tumbler_module.cc
CXX_SOURCES+=scripting_bridge.cc cube.cc testirrlicht.cc

#
# Get pepper directory for toolchain and includes.
#
# If PEPPER_ROOT is not set, then assume it can be found a two directories up,
# from the default example directory location.
#
THIS_MAKEFILE:=$(abspath $(lastword $(MAKEFILE_LIST)))
NACL_SDK_ROOT?=$(abspath $(dir $(THIS_MAKEFILE))../..)

# Project Build flags
WARNINGS:=-Wno-long-long -Wall
CXXFLAGS:=-std=gnu++98 $(WARNINGS) -I../IrrNaCl/include

#
# Compute tool paths
#
#
OSNAME:=win
TC_PATH:=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_glibc)
CXX:=$(TC_PATH)/bin/i686-nacl-g++

#
# Disable DOS PATH warning when using Cygwin based tools Windows
#
CYGWIN ?= nodosfilewarning
export CYGWIN


# Declare the ALL target first, to make the 'all' target the default build
all: $(PROJECT)_x86_32.nexe

# Define 32 bit compile and link rules for C++ sources
x86_32_OBJS:=$(patsubst %.cc,%_32.o,$(CXX_SOURCES))
$(x86_32_OBJS) : %_32.o : %.cc $(THIS_MAKE)
	$(CXX) -o $@ -c $< -m32 -O0 -g $(CXXFLAGS)

$(PROJECT)_x86_32.nexe : $(x86_32_OBJS)
	$(CXX) -o $@ $^ -m32 -O0 -g $(CXXFLAGS) $(LDFLAGS)


# Define a phony rule so it always runs, to build nexe and start up server.
.PHONY: RUN 
RUN: all
	python ../httpd.py


