# Copyright(C) 2015 Verizon. All rights reserved.
#
# Magic to run the including Makefile after changing to a subdirectory.
# This lets VPATH be used to find the sources, while collecting all the 
# objects into a single object directory.
# Based on the example at:
#   http://make.mad-scientist.net/papers/multi-architecture-builds/#advanced
#
.SUFFIXES:

OBJDIR := build

MAKETARGET = $(MAKE) -C $@ -f $(CURDIR)/Makefile SRCDIR=$(CURDIR) \
	$(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR) :
	@# Re-invoke the including Makefile
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

# Everything depends on OBJDIR (but no explict action to take)
% :: $(OBJDIR) ; :

