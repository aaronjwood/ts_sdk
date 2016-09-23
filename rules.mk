# Copyright(C) 2016 Verizon. All rights reserved.

# This fragment of the Makefile lists the object files that need to be generated
# along with the target rules.

# Remove files common to both LIB_SRC and DBG_LIB_SRC from LIB_SRC
LIB_SRC := $(filter-out $(DBG_LIB_SRC), $(LIB_SRC))

# Create lists of object files to be generated from the sources
OBJ_USER = $(addsuffix .o, $(basename $(notdir $(USER_SRC))))
OBJ_LIB = $(addsuffix .o, $(basename $(CORELIB_SRC)))
OBJ_LIB += $(addsuffix .o, $(basename $(LIB_SRC)))
OBJ_DBG_LIB = $(addsuffix .o, $(basename $(DBG_LIB_SRC)))

OBJ = $(OBJ_USER) $(OBJ_LIB) $(OBJ_DBG_LIB)

CFLAGS_USER = -Wall -Werror -std=c99 -Wcast-align $(INC) $(DBG_OP_USER_FLAGS)
CFLAGS_LIB = -Werror -std=c99 $(INC) $(DBG_OP_LIB_FLAGS) -fdata-sections \
	     -ffunction-sections
CFLAGS_DBG_LIB = -Werror -std=c99 $(INC) $(DBG_OP_USER_FLAGS) -fdata-sections \
		 -ffunction-sections

#==================================RULES=======================================#
.PHONY: all build dump bin clean upload

all build: $(FW_EXEC)
	$(SIZE) $(FW_EXEC)

$(FW_EXEC): $(OBJ) $(OBJ_STARTUP)
	$(CC) -Wl,-Map,fw.map,--cref $(NOSYSLIB) $(INC) -Os $(MFLAGS) -T $(LDSCRIPT) \
		$(OBJ) $(OBJ_STARTUP) -o $(FW_EXEC)

$(OBJ_STARTUP):
	$(AS) $(MFLAGS) -o $(OBJ_STARTUP) $(STARTUP_SRC)

$(OBJ_LIB): %.o: %.c
	$(CC) -c $(CFLAGS_LIB) $(DBG_MACRO) $(MFLAGS) $(MDEF) $< -o $@

$(OBJ_DBG_LIB): %.o: %.c
	$(CC) -c $(CFLAGS_DBG_LIB) $(MFLAGS) $(MDEF) $< -o $@

$(OBJ_USER): %.o: %.c
	$(CC) -c $(CFLAGS_USER) $(DBG_MACRO) $(MFLAGS) $(MDEF) $< -o $@

clean:
	rm -rf $(SRCDIR)/build

dump: $(FW_EXEC)
	$(OBJDUMP) -DsS $(FW_EXEC) > dump.s

bin: $(FW_EXEC)
	$(OBJCOPY) -O binary $(FW_EXEC) fw.bin
	$(OBJCOPY) -O ihex $(FW_EXEC) fw.hex

upload: $(FW_EXEC)
	openocd -f $(TOOLS_ROOT)/openocdcfg-stm32f4/board/st_nucleo_f4.cfg \
		-c init -c "reset halt" \
		-c "flash write_image erase $(FW_EXEC)" \
		-c reset -c shutdown
