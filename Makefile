# Makefile to build scc
# Copyright (C) 2022  Alexandros Athanasiadis
#
# This file is part of scc
# 
# scc is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# scc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# The C Compiler and Compiler flags
CC=clang
CFLAGS=-O3
LDFLAGS=-lpthread

# The name of the executable
PROGNAME=scc

# the build and bin directories will contain the objects and executables
BINDIR=bin
OBJDIR=build

# the path to the executable
PROGRAM=$(BINDIR)/$(PROGNAME)

# the object files
SRCOBJ=scc.o graph.o scc_serial.o scc_openmp.o
EXTOBJ=mmio.o
OBJFILES=$(addprefix $(OBJDIR)/,$(SRCOBJ) $(EXTOBJ))

# The path make searches for dependency files
SPACE= 
VPATH=$(subst $(SPACE),:,$(patsubst %.o,src/%,$(SRCOBJ)) $(patsubst %.o,external/%,$(EXTOBJ)))

# Adding VPATH to the compiler path
override CFLAGS += $(patsubst %,-I%,$(subst :,$(SPACE),$(VPATH)))

# default target
default: $(PROGRAM)

# all target
all: clean $(PROGRAM)

# Linking the object files into the final executable
$(PROGRAM): $(OBJFILES) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(PROGRAM) $(OBJFILES) $(LDFLAGS)

# Compiling the C files into object files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Creating the bin directory
$(BINDIR):
	mkdir $(BINDIR)

# Creating the object directory
$(OBJDIR):
	mkdir $(OBJDIR)

# Remove all the produced files from the directory
.PHONY: clean
clean: 
	rm -rf $(BINDIR) $(OBJDIR)

