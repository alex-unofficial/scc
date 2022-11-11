CC=gcc
CFLAGS=-g3

PROGRAM=exec

OBJDIR=obj
OBJFILES=$(addprefix $(OBJDIR)/, main.o matrix.o mmio.o)

SRCDIR=src/matrix
EXTERNALDIR=external/matrix-market

VPATH=$(SRCDIR):$(EXTERNALDIR)

override CFLAGS += $(patsubst %,-I%,$(subst :, ,$(VPATH)))

$(PROGRAM): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(PROGRAM)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

.PHONY: purge clean

purge: clean
	rm -f $(PROGRAM)

clean:
	rm -rf $(OBJDIR)

