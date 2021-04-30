### Project name (also used for output file name)
PROJECT	= barometer

### Source files, text/binary files and search directory
CSRC    = main.c aqm0802.c i2c.c dps368.c
ASRC    =
TSRC    =
BSRC    =
VPATH   =

### Target device
DEVICE  = attiny402
ATPACK  = /mnt/c/Users/minagi/Downloads/Microchip.ATtiny_DFP.2.5.116

### Optimization level (0, 1, 2, 3, 4 or s)
OPTIMIZE = s

### C Standard level (c89, gnu89, c99 or gnu99)
CSTD = gnu99

### Include dirs, library dirs and definitions
LIBDIRS	=
INCDIRS	=
DEFS	= F_CPU=3333333
ADEFS	=

### Warning contorls
WARNINGS = all extra

### Output directory
OBJDIR = obj

### Output file format (hex, bin or both) and debugger type
OUTPUT	= both
HEXFMT  = ihex
DEBUG	= dwarf-2


### Programs to build porject
CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size
NM      = avr-nm


### Object file format
FORMAT = elf32-avr


### Data section
DSEC = .progmem.data # MUST BE FIXED

# Define all object files
COBJ      = $(CSRC:.c=.o)
AOBJ      = $(ASRC:.S=.o)
TOBJ      = $(TSRC:.txt=.o)
BOBJ      = $(addsuffix .o,$(basename $(BSRC)))
COBJ      := $(addprefix $(OBJDIR)/,$(COBJ))
AOBJ      := $(addprefix $(OBJDIR)/,$(AOBJ))
TOBJ      := $(addprefix $(OBJDIR)/,$(TOBJ))
BOBJ      := $(addprefix $(OBJDIR)/,$(BOBJ))
PROJECT   := $(OBJDIR)/$(PROJECT)


# Flags for C files
LIBDIRS += $(addsuffix /gcc/dev/$(DEVICE)/,$(ATPACK))
INCDIRS += $(addsuffix /include/,$(ATPACK))
CFLAGS += -std=$(CSTD)
CFLAGS += -g$(DEBUG)
CFLAGS += -mmcu=$(DEVICE)
CFLAGS += -O$(OPTIMIZE) -mcall-prologues
CFLAGS += $(addprefix -W,$(WARNINGS))
CFLAGS += $(addprefix -B,$(LIBDIRS))
CFLAGS += $(addprefix -I,$(INCDIRS))
CFLAGS += $(addprefix -D,$(DEFS))
CFLAGS += -Wp,-MM,-MP,-MT,$(OBJDIR)/$(*F).o,-MF,$(OBJDIR)/$(*F).d


# Assembler flags
ASFLAGS += $(addprefix -D,$(ADEFS)) -Wa,-gstabs,-g$(DEBUG)
ALL_ASFLAGS = -mmcu=$(DEVICE) -I. -x assembler-with-cpp $(ASFLAGS)


# Linker flags
LDFLAGS += -Wl,-Map,$(PROJECT).map


# Default target.
all: build size

ifeq ($(OUTPUT),hex)
build: elf hex lst sym
hex: $(PROJECT).hex
else
ifeq ($(OUTPUT),bin)
build: elf bin lst sym
bin: $(PROJECT).bin
else
ifeq ($(OUTPUT),both)
build: elf hex bin lst sym
hex: $(PROJECT).hex
bin: $(PROJECT).bin
else
$(error "Invalid format: $(OUTPUT)")
endif
endif
endif

elf: $(PROJECT).elf
lst: $(PROJECT).lst
sym: $(PROJECT).sym


# Display compiler version information.
version :
	@$(CC) --version

# Create final output file from ELF output file.
%.hex: %.elf
	@echo
	$(OBJCOPY) -j .text -j .rodata -j .data -j .eeprom -j .fuse -O $(HEXFMT) $< $@

%.bin: %.elf
	@echo
	$(OBJCOPY) -j .text -j .rodata -j .data -O binary $< $@

# Create extended listing file from ELF output file.
%.lst: %.elf
	@echo
	$(OBJDUMP) -h -S -C $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	$(NM) -n $< > $@

# Display size of file.
size:
	@echo
#	$(SIZE) -C --mcu=$(DEVICE) $(PROJECT).elf
	$(SIZE) $(PROJECT).elf


# Link: create ELF output file from object files.
%.elf:  $(TOBJ) $(BOBJ) $(AOBJ) $(COBJ)
	@echo
	@echo Linking...
	$(CC) $(CFLAGS) $(LDFLAGS) $(AOBJ) $(COBJ) $(TOBJ) $(BOBJ) -o $@

# Compile: create object files from C source files. ARM or Thumb(-2)
$(COBJ) : $(OBJDIR)/%.o : %.c
	@echo
	@echo $< :
	$(CC) -c $(CFLAGS) $< -o $@

# Assemble: create object files from assembler source files. ARM or Thumb(-2)
$(AOBJ) : $(OBJDIR)/%.o : %.S
	@echo
	@echo $< :
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

# create object files from text files.
$(TOBJ) : $(OBJDIR)/%.o : %.txt
	@echo $< :
	@echo Converting $<
	@cp $(<) $(*).tmp
	@printf "\0" >> $(*).tmp
	$(OBJCOPY) -I binary -O $(FORMAT) \
	--rename-section .data=$(DSEC),contents,alloc,load,readonly,data \
	--redefine-sym _binary_$*_tmp_start=$* \
	--redefine-sym _binary_$*_tmp_end=$*_end \
	--redefine-sym _binary_$*_tmp_size=$*_size_sym \
	$(*).tmp $(@)
	@echo #include ^<stdint.h^> > $(*).h
	@echo #include ^<stddef.h^> > $(*).h
	@echo extern const __flash uint8_t $(*)[]; >> $(*).h
	@echo extern const __flash uint8_t $(*)_end[]; >> $(*).h
	@echo extern const size_t $(*)_size_sym[]; >> $(*).h
	@echo #define $(*)_size ((size_t)$(*)_size_sym) >> $(*).h
	@rm $(*).tmp

# https://www.microchip.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_binarydata.html
# create object files from binary files.
define bfunc
$(1) : $(2)
	@echo $(2) :
	$(OBJCOPY) -I binary -O $(FORMAT) \
	--rename-section .data=$(DSEC),contents,alloc,load,readonly,data \
	--redefine-sym _binary_$(subst .,_,$(2))_start=$(basename $(2)) \
	--redefine-sym _binary_$(subst .,_,$(2))_end=$(basename $(2))_end \
	--redefine-sym _binary_$(subst .,_,$(2))_size=$(basename $(2))_size_sym \
	$(2) $(1)
	@echo #include ^<stdint.h^> > $(basename $(2)).h
	@echo #include ^<stddef.h^> >> $(basename $(2)).h
	@echo extern const __flash uint8_t $(basename $(2))[]; >> $(basename $(2)).h
	@echo extern const __flash uint8_t $(basename $(2))_end[]; >>$(basename $(2)).h
	@echo extern const size_t $(basename $(2))_size_sym[]; >> $(basename $(2)).h
	@echo #define $(basename $(2))_size ((size_t)$(basename $(2))_size_sym) >> $(basename $(2)).h
endef
$(foreach BFILE,$(BSRC),$(eval $(call bfunc,$(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(BFILE)))),$(BFILE))))

# Target: clean project.
clean:
	@echo
	rm -f -r $(OBJDIR) | exit 0


# Program to MCU
program:
	C:/Users/minagi/git/updisp-dev/x64/Debug/updisp-dev.exe -p COM4 -d $(DEVICE) -i $(PROJECT).hex

# Include the dependency files.
-include $(shell mkdir $(OBJDIR) ) $(wildcard $(OBJDIR)/*.d)
