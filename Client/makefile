SRCDIR=src
INCDIR=inc
OBJDIR=obj
BINDIR=bin
DEPDIR=dep
LIBDIR=lib

POSTFIX=cpp
OUTFILENAME=outfile.out
TESTPOSTFIX=.test

SOURCES=$(notdir $(wildcard $(SRCDIR)/*.$(POSTFIX)))
OBJS=$(patsubst %.$(POSTFIX),%.o,$(SOURCES))
LIBS=$(notdir $(wildcard $(LIBDIR)/*.so))
LIBKINKERS=$(patsubst lib%.so,-l%,$(LIBS))

BASEFLAGS= -Wall -I $(INCDIR) -std=c++0x -pthread 
POSTFIXFLAGS= -L $(LIBDIR) -lstdc++ $(LIBKINKERS)
CFLAGS=$(BASEFLAGS) -O2 -fno-strict-aliasing 

vpath %.h 			$(INCDIR)
vpath %.$(POSTFIX)	$(SRCDIR)
vpath %.o			$(OBJDIR)
vpath %.d			$(DEPDIR)

ifeq ($(MAKECMDGOALS), runtest)
CFLAGS=$(BASEFLAGS) -g -DDEBUG -O0
endif
ifeq ($(MAKECMDGOALS), debug)
CFLAGS=$(BASEFLAGS) -g -DDEBUG -O0
endif
ifeq ($(MAKECMDGOALS), gdbrun)
CFLAGS=$(BASEFLAGS) -g -DDEBUG -O0
endif

release:$(BINDIR)/$(OUTFILENAME)

$(BINDIR)/$(OUTFILENAME):$(OBJS)
	@$(CC) $(CFLAGS) -o $(BINDIR)/$(OUTFILENAME) $(patsubst %,$(OBJDIR)/%,$(OBJS)) $(POSTFIXFLAGS)
%.o:%.$(POSTFIX)
	@$(CC) -c $(CFLAGS) $< -o $(OBJDIR)/$@
$(DEPDIR)/%.d:%.$(POSTFIX)
	@$(RM) $(DEPDIR)/$@;
	@$(CC) -MM $(CFLAGS) $< | sed 's/\($(patsubst %.$(POSTFIX),%,$(notdir $<))\)\.o *: */\1.o \1.d:/' > $@

$(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX):$(OBJS)
	@$(CC) $(CFLAGS) -o $(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX) $(patsubst %,$(OBJDIR)/%,$(OBJS)) $(POSTFIXFLAGS)
 
ifneq ($(MAKECMDGOALS),clean)
sinclude $(patsubst %.o,$(DEPDIR)/%.d,$(OBJS))
endif

.PHONY: run runtest gdbrun release debug clean

run:$(BINDIR)/$(OUTFILENAME)
	@$(BINDIR)/$(OUTFILENAME)

runtest:$(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)
	@$(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)

gdbrun:$(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)
	@gdb $(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)

debug:$(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)

clean:
	@$(RM) $(OBJDIR)/*.o
	@$(RM) $(DEPDIR)/*.d
	@$(RM) $(BINDIR)/$(OUTFILENAME)
	@$(RM) $(BINDIR)/$(OUTFILENAME)$(TESTPOSTFIX)