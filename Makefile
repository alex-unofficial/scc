# The C Compiler and Compiler flags
CC=gcc
CFLAGS=-fopenmp -g
LDFLAGS=-lpthread

# The name of the executable
PROGRAM=scc

# The directory to put object files in and the object files themselves
OBJDIR=obj
SRCOBJ=scc.o graph.o scc_serial.o scc_pthreads.o scc_openmp.o
EXTOBJ=mmio.o
OBJFILES=$(addprefix $(OBJDIR)/,$(SRCOBJ) $(EXTOBJ))

# The path make searches for dependency files
SPACE= 
VPATH=$(subst $(SPACE),:,$(patsubst %.o,src/%,$(SRCOBJ)) $(patsubst %.o,external/%,$(EXTOBJ)))

# Adding VPATH to the compiler path
override CFLAGS += $(patsubst %,-I%,$(subst :,$(SPACE),$(VPATH)))

# default all target
all: $(PROGRAM)

# Linking the object files into the final executable
$(PROGRAM): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(PROGRAM) $^ $(LDFLAGS)

# Compiling the C files into object files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Creating the object directory
$(OBJDIR):
	mkdir $(OBJDIR)

# Remove all the produced files from the directory
.PHONY: clean
clean: 
	rm -rf $(PROGRAM) $(OBJDIR)

