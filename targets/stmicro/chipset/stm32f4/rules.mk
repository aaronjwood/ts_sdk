# Copyright(C) 2016, 2017 Verizon. All rights reserved.

# This fragment of the Makefile lists the object files that need to be generated
# along with the target rules.

CHIPSET_SRC := $(filter-out $(DBG_LIB_SRC), $(CHIPSET_SRC))

OBJ_CHIPSET = $(addsuffix .o, $(basename $(CHIPSET_SRC)))
OBJ_DBG_LIB = $(addsuffix .o, $(basename $(DBG_LIB_SRC)))

OBJ = $(OBJ_CHIPSET) $(OBJ_DBG_LIB) $(OBJ_STARTUP)

CFLAGS_COM = -Werror -std=c99 $(CHIPSET_INC)
CFLAGS_CHIPSET = -Os $(CFLAGS_COM) $(DBG_OP_LIB_FLAGS) \
	-fdata-sections -ffunction-sections
CFLAGS_DBG_LIB = -Wall -Wcast-align $(CFLAGS_COM) $(DBG_OP_USER_FLAGS) \
	-fdata-sections -ffunction-sections

#==================================RULES=======================================#
.PHONY: all build_chipset clean

all build_chipset: $(OBJ) lib$(CHIPSET_MCU).a

lib$(CHIPSET_MCU).a:
	$(AR) rc $@ $(OBJ)
	$(RANLIB) $@

$(OBJ_STARTUP):
	$(AS) -c $(ARCHFLAGS) -o $(OBJ_STARTUP) $(STARTUP_SRC)

$(OBJ_CHIPSET): %.o: %.c
	$(CC) -c $(CFLAGS_CHIPSET) $(ARCHFLAGS) $(MDEF) $< -o $@

$(OBJ_DBG_LIB): %.o: %.c
	$(CC) -c $(CFLAGS_DBG_LIB) $(DBG_MACRO) $(ARCHFLAGS) $(MDEF) $< -o $@

clean:
	rm -rf $(SRCDIR)/build
