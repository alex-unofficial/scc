# The C Compiler and Compiler flags
CC=gcc
CFLAGS=-g3

# The name of the executable
PROGRAM=exec

# The directory to put object files in and the object files themselves
OBJDIR=obj
OBJ=main.o matrix.o mmio.o
OBJFILES=$(addprefix $(OBJDIR)/, $(OBJ))

# Directories with header files
SRCDIR=src/matrix
EXTERNALDIR=external/matrix-market

# The path make searches for dependency files
VPATH=$(SRCDIR):$(EXTERNALDIR)

# Adding VPATH to the compiler path
override CFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))

# Linking the object files into the final executable
$(PROGRAM): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(PROGRAM)

# Compiling the C files into object files
$(OBJDIR)/%.o: %.c %.h | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Creating the object directory
$(OBJDIR):
	mkdir $(OBJDIR)

# Directory management targets, clean removes everything but the executable,
# purge removes the executable
.PHONY: purge clean
purge: clean
	rm -f $(PROGRAM)

clean:
	rm -rf $(OBJDIR)

