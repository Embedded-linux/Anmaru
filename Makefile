
# =============================================================================
# DSRTOS - Dynamic Scheduler Real-Time Operating System
# Complete Makefile for Phase 1 (Boot & Board Bring-up) and Phase 2 (Kernel Core)
# 
# Copyright (c) 2024 DSRTOS
# Compliance: MISRA-C:2012, DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
# =============================================================================

# -----------------------------------------------------------------------------
# PROJECT CONFIGURATION
# -----------------------------------------------------------------------------
PROJECT_NAME    = dsrtos
VERSION_MAJOR   = 1
VERSION_MINOR   = 0
VERSION_PATCH   = 0
VERSION         = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
BUILD_DATE      = $(shell date +%Y%m%d_%H%M%S)
BUILD_NUMBER    = $(shell git rev-parse --short HEAD 2>/dev/null || echo "dev")

# -----------------------------------------------------------------------------
# TARGET CONFIGURATION
# -----------------------------------------------------------------------------
TARGET          = stm32f407vg
MCU             = STM32F407VG
MCU_FAMILY      = STM32F4xx
CPU             = cortex-m4
FPU             = fpv4-sp-d16
FLOAT_ABI       = hard
FLASH_START     = 0x08000000
RAM_START       = 0x20000000
CCM_START       = 0x10000000

# -----------------------------------------------------------------------------
# TOOLCHAIN CONFIGURATION
# -----------------------------------------------------------------------------
TOOLCHAIN_PREFIX = arm-none-eabi-
CC              = $(TOOLCHAIN_PREFIX)gcc
CXX             = $(TOOLCHAIN_PREFIX)g++
AS              = $(TOOLCHAIN_PREFIX)gcc -x assembler-with-cpp
LD              = $(TOOLCHAIN_PREFIX)gcc
AR              = $(TOOLCHAIN_PREFIX)ar
OBJCOPY         = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP         = $(TOOLCHAIN_PREFIX)objdump
SIZE            = $(TOOLCHAIN_PREFIX)size
NM              = $(TOOLCHAIN_PREFIX)nm
STRIP           = $(TOOLCHAIN_PREFIX)strip
GDB             = $(TOOLCHAIN_PREFIX)gdb
READELF         = $(TOOLCHAIN_PREFIX)readelf

# -----------------------------------------------------------------------------
# HOST TOOLS
# -----------------------------------------------------------------------------
MKDIR           = mkdir -p
RM              = rm -rf
CP              = cp -f
ECHO            = @echo
CAT             = cat
SED             = sed
AWK             = awk
PYTHON          = python3

# -----------------------------------------------------------------------------
# EXTERNAL TOOLS
# -----------------------------------------------------------------------------
OPENOCD         = openocd
JLINK           = JLinkExe
STFLASH         = st-flash
DOXYGEN         = doxygen
CPPCHECK        = cppcheck
SPLINT          = splint
PCLINT          = pc-lint-plus
GCOV            = $(TOOLCHAIN_PREFIX)gcov
LCOV            = lcov
GENHTML         = genhtml

# -----------------------------------------------------------------------------
# DIRECTORY STRUCTURE
# -----------------------------------------------------------------------------
# Top-level directories
TOP_DIR         = .
SRC_DIR         = src
INC_DIR         = include
BUILD_DIR       = build
DOC_DIR         = docs
TEST_DIR        = test
TOOLS_DIR       = tools
CONFIG_DIR      = config

# Source subdirectories
PHASE1_SRC_DIR  = $(SRC_DIR)/phase1
PHASE2_SRC_DIR  = $(SRC_DIR)/phase2
PHASE4_SRC_DIR  = phase4/src
COMMON_SRC_DIR  = $(SRC_DIR)/common
STARTUP_DIR     = $(SRC_DIR)/phase2
HAL_SRC_DIR     = $(SRC_DIR)/hal
DRIVER_SRC_DIR  = $(SRC_DIR)/drivers

# Include subdirectories
PHASE1_INC_DIR  = $(INC_DIR)/phase1
PHASE2_INC_DIR  = $(INC_DIR)/phase2
PHASE3_INC_DIR  = $(INC_DIR)/phase3
PHASE4_INC_DIR  = phase4/include
COMMON_INC_DIR  = $(INC_DIR)/common
HAL_INC_DIR     = $(INC_DIR)/hal
DRIVER_INC_DIR  = $(INC_DIR)/drivers

# Build subdirectories
OBJ_DIR         = $(BUILD_DIR)/obj
DEP_DIR         = $(BUILD_DIR)/dep
BIN_DIR         = $(BUILD_DIR)/bin
MAP_DIR         = $(BUILD_DIR)/map
LST_DIR         = $(BUILD_DIR)/lst
ASM_DIR         = $(BUILD_DIR)/asm
REPORT_DIR      = $(BUILD_DIR)/reports
COV_DIR         = $(BUILD_DIR)/coverage

# Linker script directory
LINKER_DIR      = $(CONFIG_DIR)/linker

# -----------------------------------------------------------------------------
# SOURCE FILES - PHASE 1 (Boot & Board Bring-up)
# -----------------------------------------------------------------------------
PHASE1_C_SOURCES = \
    src/dsrtos_stubs.c \
	src/main.c \
    $(PHASE1_SRC_DIR)/dsrtos_boot.c \
    $(PHASE1_SRC_DIR)/dsrtos_clock.c \
    $(PHASE1_SRC_DIR)/dsrtos_interrupt.c \
    $(PHASE1_SRC_DIR)/dsrtos_timer.c \
    $(PHASE1_SRC_DIR)/dsrtos_uart.c

PHASE1_H_HEADERS = \
    $(PHASE1_INC_DIR)/dsrtos_boot.h \
    $(PHASE1_INC_DIR)/dsrtos_clock.h \
    $(PHASE1_INC_DIR)/dsrtos_interrupt.h \
    $(PHASE1_INC_DIR)/dsrtos_timer.h \
    $(PHASE1_INC_DIR)/dsrtos_uart.h

# -----------------------------------------------------------------------------
# SOURCE FILES - PHASE 2 (Kernel Core)
# -----------------------------------------------------------------------------
PHASE2_C_SOURCES = \
    $(PHASE2_SRC_DIR)/dsrtos_kernel_init.c \
    $(PHASE2_SRC_DIR)/dsrtos_critical.c \
    $(PHASE2_SRC_DIR)/dsrtos_panic.c \
    $(PHASE2_SRC_DIR)/dsrtos_syscall.c \
    $(PHASE2_SRC_DIR)/dsrtos_hooks.c \
    $(PHASE2_SRC_DIR)/dsrtos_stats.c \
    $(PHASE2_SRC_DIR)/dsrtos_assert.c

PHASE2_H_HEADERS = \
    $(PHASE2_INC_DIR)/dsrtos_kernel_init.h \
    $(PHASE2_INC_DIR)/dsrtos_critical.h \
    $(PHASE2_INC_DIR)/dsrtos_panic.h \
    $(PHASE2_INC_DIR)/dsrtos_syscall.h \
    $(PHASE2_INC_DIR)/dsrtos_hooks.h \
    $(PHASE2_INC_DIR)/dsrtos_stats.h \
    $(PHASE2_INC_DIR)/dsrtos_assert.h

PHASE4_C_SOURCES = \
    phase4/src/dsrtos_task_scheduler_interface.c \
    phase4/src/dsrtos_context_switch.c \
    phase4/src/dsrtos_preemption.c \
    phase4/src/dsrtos_ready_queue_ops.c \
    phase4/src/dsrtos_scheduler_stats.c \
    phase4/src/dsrtos_priority_mgmt.c \
    phase4/src/dsrtos_scheduler_decision.c \
    phase4/src/dsrtos_queue_integrity.c

PHASE4_H_HEADERS = \
    $(PHASE4_INC_DIR)/dsrtos_task_scheduler_interface.h \
    $(PHASE4_INC_DIR)/dsrtos_preemption.h \
    $(PHASE4_INC_DIR)/dsrtos_ready_queue_ops.h \
    $(PHASE4_INC_DIR)/dsrtos_scheduler_stats.h \
    $(PHASE4_INC_DIR)/dsrtos_priority_mgmt.h \
    $(PHASE4_INC_DIR)/dsrtos_scheduler_decision.h \
    $(PHASE4_INC_DIR)/dsrtos_queue_integrity.h

# -----------------------------------------------------------------------------
# SOURCE FILES - COMMON/SUPPORT
# -----------------------------------------------------------------------------
COMMON_C_SOURCES = \
    $(COMMON_SRC_DIR)/dsrtos_memory_stub.c \
    $(COMMON_SRC_DIR)/dsrtos_error.c

COMMON_H_HEADERS = \
    $(COMMON_INC_DIR)/dsrtos_types.h \
    $(COMMON_INC_DIR)/dsrtos_error.h \
    $(COMMON_INC_DIR)/dsrtos_config.h \
    $(COMMON_INC_DIR)/dsrtos_memory.h

# -----------------------------------------------------------------------------
# STARTUP AND SYSTEM FILES
# -----------------------------------------------------------------------------
STARTUP_C_SOURCES = \
    $(STARTUP_DIR)/startup_stm32f407xx.c

SYSTEM_C_SOURCES = \
    $(STARTUP_DIR)/system_stm32f4xx.c

# -----------------------------------------------------------------------------
# LINKER SCRIPTS
# -----------------------------------------------------------------------------
LDSCRIPT = $(LINKER_DIR)/stm32f407vg.ld

# -----------------------------------------------------------------------------
# ALL SOURCE FILES COMBINED
# -----------------------------------------------------------------------------
C_SOURCES = \
    $(PHASE1_C_SOURCES) \
    $(PHASE2_C_SOURCES) \
    $(PHASE4_C_SOURCES) \
    $(COMMON_C_SOURCES) \
    $(SYSTEM_C_SOURCES) \
    $(STARTUP_C_SOURCES)

ASM_SOURCES = \
    phase4/src/dsrtos_context_switch.S

ALL_SOURCES = \
    $(C_SOURCES) \
    $(ASM_SOURCES)

ALL_HEADERS = \
    $(PHASE1_H_HEADERS) \
    $(PHASE2_H_HEADERS) \
    $(COMMON_H_HEADERS)

# -----------------------------------------------------------------------------
# INCLUDE PATHS
# -----------------------------------------------------------------------------
INCLUDES = \
    -I$(INC_DIR) \
    -I$(COMMON_INC_DIR) \
    -I$(PHASE1_INC_DIR) \
    -I$(PHASE2_INC_DIR) \
    -I$(PHASE3_INC_DIR) \
    -I$(PHASE4_INC_DIR) \
    -I$(HAL_INC_DIR) \
    -I$(DRIVER_INC_DIR)

# -----------------------------------------------------------------------------
# COMPILER FLAGS - ARCHITECTURE
# -----------------------------------------------------------------------------
ARCH_FLAGS = \
    -mcpu=$(CPU) \
    -mthumb \
    -mfpu=$(FPU) \
    -mfloat-abi=$(FLOAT_ABI)

# -----------------------------------------------------------------------------
# COMPILER FLAGS - PREPROCESSOR DEFINES
# -----------------------------------------------------------------------------
DEFINES = \
    -D$(MCU_FAMILY) \
    -D$(MCU) \
    -DUSE_HAL_DRIVER \
    -DSTM32F407xx \
    -DARM_MATH_CM4 \
    -DDSRTOS_BUILD_NUMBER=\"$(BUILD_NUMBER)\" \
    -DDSRTOS_BUILD_DATE=\"$(BUILD_DATE)\"

# -----------------------------------------------------------------------------
# COMPILER FLAGS - C STANDARD
# -----------------------------------------------------------------------------
C_STANDARD = -std=c11

# -----------------------------------------------------------------------------
# COMPILER FLAGS - WARNINGS
# -----------------------------------------------------------------------------
WARN_FLAGS = \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wshadow \
    -Wundef \
    -Wcast-align \
    -Wcast-qual \
    -Wconversion \
    -Wsign-conversion \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wmissing-declarations \
    -Wredundant-decls \
    -Wnested-externs \
    -Wswitch-default \
    -Wswitch-enum \
    -Wlogical-op \
    -Wfloat-equal \
    -Wwrite-strings \
    -Wpointer-arith \
    -Wstrict-overflow=5 \
    -Wformat=2 \
    -Wformat-truncation \
    -Wnull-dereference \
    -Wstack-usage=256 \
    -Wno-unused-parameter \
    -Wno-missing-field-initializers

# -----------------------------------------------------------------------------
# COMPILER FLAGS - MISRA COMPLIANCE
# -----------------------------------------------------------------------------
MISRA_FLAGS = \
    -Wmissing-include-dirs \
    -Wswitch-default \
    -Wswitch-enum \
    -Wconversion \
    -Wunreachable-code

# -----------------------------------------------------------------------------
# COMPILER FLAGS - SAFETY CRITICAL
# -----------------------------------------------------------------------------
SAFETY_FLAGS = \
    -fno-strict-aliasing \
    -fno-builtin \
    -ffreestanding \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fstack-protector-strong \
    -fstack-usage \
    -fno-delete-null-pointer-checks

# -----------------------------------------------------------------------------
# COMPILER FLAGS - OPTIMIZATION
# -----------------------------------------------------------------------------
OPT_FLAGS_DEBUG = -O0 -g3 -ggdb
OPT_FLAGS_RELEASE = -Os -g
OPT_FLAGS_CERT = -O2 -g

# -----------------------------------------------------------------------------
# COMPILER FLAGS - DEBUG
# -----------------------------------------------------------------------------
DEBUG_FLAGS = \
    -DDEBUG \
    -DDSRTOS_DEBUG_ENABLED=1 \
    -DDSRTOS_ASSERT_ENABLED=1 \
    -DDSRTOS_STATS_ENABLED=1

# -----------------------------------------------------------------------------
# COMPILER FLAGS - RELEASE
# -----------------------------------------------------------------------------
RELEASE_FLAGS = \
    -DNDEBUG \
    -DDSRTOS_DEBUG_ENABLED=0 \
    -DDSRTOS_ASSERT_ENABLED=0 \
    -DDSRTOS_STATS_ENABLED=1

# -----------------------------------------------------------------------------
# COMPILER FLAGS - CERTIFICATION
# -----------------------------------------------------------------------------
CERT_FLAGS = \
    -DNDEBUG \
    -DDSRTOS_MISRA_COMPLIANT=1 \
    -DDSRTOS_DO178C_COMPLIANT=1 \
    -DDSRTOS_IEC62304_COMPLIANT=1 \
    -DDSRTOS_ISO26262_COMPLIANT=1 \
    -DDSRTOS_SAFETY_CRITICAL=1 \
    -DDSRTOS_ASSERT_ENABLED=1 \
    -DDSRTOS_STATS_ENABLED=1



# -----------------------------------------------------------------------------
# BUILD TYPE SELECTION
# -----------------------------------------------------------------------------
BUILD_TYPE ?= debug

ifeq ($(BUILD_TYPE),debug)
    OPT_FLAGS = $(OPT_FLAGS_DEBUG)
    TYPE_FLAGS = $(DEBUG_FLAGS)
    BUILD_SUFFIX = _debug
else ifeq ($(BUILD_TYPE),release)
    OPT_FLAGS = $(OPT_FLAGS_RELEASE)
    TYPE_FLAGS = $(RELEASE_FLAGS)
    BUILD_SUFFIX = _release
else ifeq ($(BUILD_TYPE),cert)
    OPT_FLAGS = $(OPT_FLAGS_CERT)
    TYPE_FLAGS = $(CERT_FLAGS) $(MISRA_FLAGS)
    BUILD_SUFFIX = _cert
else
    $(error Invalid BUILD_TYPE: $(BUILD_TYPE))
endif

# -----------------------------------------------------------------------------
# COMPILER FLAGS - COMBINED
# -----------------------------------------------------------------------------
COMMON_FLAGS = \
    $(ARCH_FLAGS) \
    $(DEFINES) \
    $(INCLUDES) \
    $(WARN_FLAGS) \
    $(SAFETY_FLAGS) \
    $(OPT_FLAGS) \
    $(TYPE_FLAGS)

CFLAGS = \
    $(COMMON_FLAGS) \
    $(C_STANDARD) \
    -fmessage-length=0 \
    -fdiagnostics-color=always \
    --specs=nano.specs \
    -MMD -MP
$(info CFLAGS = $(CFLAGS))

ASFLAGS = \
    $(ARCH_FLAGS) \
    $(DEFINES) \
    $(INCLUDES) \
    $(OPT_FLAGS) \
    -x assembler-with-cpp \
    -MMD -MP

#CFLAGS += -Iinclude/arch/arm/cortex-m4
$(info CFLAGS = $(CFLAGS))

# -----------------------------------------------------------------------------
# LINKER FLAGS
# -----------------------------------------------------------------------------
LDFLAGS = \
    $(ARCH_FLAGS) \
    -T$(LDSCRIPT) \
    --specs=nosys.specs \
    --specs=nano.specs \
    -lnosys \
    -Wl,-Map=$(MAP_FILE),--cref \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,--no-undefined \
    -Wl,--start-group \
    -Wl,--end-group \
    -static

ifeq ($(BUILD_TYPE),debug)
    LDFLAGS += -Wl,--print-gc-sections
endif

# -----------------------------------------------------------------------------
# OUTPUT FILES
# -----------------------------------------------------------------------------
TARGET_NAME = $(PROJECT_NAME)$(BUILD_SUFFIX)
ELF_FILE = $(BIN_DIR)/$(TARGET_NAME).elf
BIN_FILE = $(BIN_DIR)/$(TARGET_NAME).bin
HEX_FILE = $(BIN_DIR)/$(TARGET_NAME).hex
MAP_FILE = $(MAP_DIR)/$(TARGET_NAME).map
LST_FILE = $(LST_DIR)/$(TARGET_NAME).lst
SYM_FILE = $(BIN_DIR)/$(TARGET_NAME).sym
DIS_FILE = $(ASM_DIR)/$(TARGET_NAME).dis

# -----------------------------------------------------------------------------
# OBJECT FILES
# -----------------------------------------------------------------------------
OBJECTS = $(addprefix $(OBJ_DIR)/, \
    $(notdir $(C_SOURCES:.c=.o)) \
    $(notdir $(ASM_SOURCES:.s=.o)))

# Dependency files
DEPS = $(addprefix $(DEP_DIR)/, \
    $(notdir $(C_SOURCES:.c=.d)) \
    $(notdir $(ASM_SOURCES:.s=.d)))

# -----------------------------------------------------------------------------
# BUILD DIRECTORIES
# -----------------------------------------------------------------------------
BUILD_DIRS = \
    $(BUILD_DIR) \
    $(OBJ_DIR) \
    $(DEP_DIR) \
    $(BIN_DIR) \
    $(MAP_DIR) \
    $(LST_DIR) \
    $(ASM_DIR) \
    $(REPORT_DIR) \
    $(COV_DIR)

# -----------------------------------------------------------------------------
# PHONY TARGETS
# -----------------------------------------------------------------------------
.PHONY: all clean debug release cert build directories
.PHONY: flash flash-jlink flash-stlink erase
.PHONY: size symbols disassembly
.PHONY: analyze cppcheck splint misra
.PHONY: test coverage
.PHONY: docs
.PHONY: info help version
.PHONY: phase1 phase2
.PHONY: debug-server debug-client

# -----------------------------------------------------------------------------
# DEFAULT TARGET
# -----------------------------------------------------------------------------
all: build

# -----------------------------------------------------------------------------
# BUILD TARGETS
# -----------------------------------------------------------------------------
build: directories $(ELF_FILE) size

debug:
	$(MAKE) BUILD_TYPE=debug build

release:
	$(MAKE) BUILD_TYPE=release build

cert:
	$(MAKE) BUILD_TYPE=cert build

# -----------------------------------------------------------------------------
# PHASE-SPECIFIC BUILDS
# -----------------------------------------------------------------------------
phase1: directories
	$(ECHO) "Building Phase 1 (Boot & Board Bring-up)..."
	$(MAKE) $(addprefix $(OBJ_DIR)/, $(notdir $(PHASE1_C_SOURCES:.c=.o)))

phase2: directories
	$(ECHO) "Building Phase 2 (Kernel Core)..."
	$(MAKE) $(addprefix $(OBJ_DIR)/, $(notdir $(PHASE2_C_SOURCES:.c=.o)))

phase4: directories
	$(ECHO) "Building Phase 4 (Task Scheduler)..."
	$(MAKE) $(addprefix $(OBJ_DIR)/, $(notdir $(PHASE4_C_SOURCES:.c=.o)))

# -----------------------------------------------------------------------------
# DIRECTORY CREATION
# -----------------------------------------------------------------------------
directories: $(BUILD_DIRS)

$(BUILD_DIRS):
	$(MKDIR) $@

# -----------------------------------------------------------------------------
# COMPILATION RULES - C FILES
# -----------------------------------------------------------------------------
# Phase 1 C files
$(OBJ_DIR)/%.o: $(PHASE1_SRC_DIR)/%.c | directories
	$(ECHO) "[CC]  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

# Phase 2 C files
$(OBJ_DIR)/%.o: $(PHASE2_SRC_DIR)/%.c | directories
	$(ECHO) "[CC]  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

# Common C files
$(OBJ_DIR)/%.o: $(COMMON_SRC_DIR)/%.c | directories
	$(ECHO) "[CC]  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

# System C files
$(OBJ_DIR)/%.o: $(STARTUP_DIR)/%.c | directories
	$(ECHO) "[CC]  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

$(OBJ_DIR)/%.o: $(PHASE4_SRC_DIR)/%.c | directories
	$(ECHO) "[CC]  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

$(OBJ_DIR)/%.o: $(PHASE4_SRC_DIR)/%.S | directories
	$(ECHO) "[AS]  $<"
	$(AS) $(ASFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

#build/bin/dsrtos_debug.elf: $(PHASE1_OBJECTS) $(PHASE2_OBJECTS) $(COMMON_OBJECTS)
#	$(CC) $(LDFLAGS) $^ -o $@

# Add this line to include main.c
MAIN_SOURCES = src/main.c
MAIN_OBJECTS = $(MAIN_SOURCES:src/%.c=build/obj/%.o)

# Add compilation rule for main source
build/obj/main.o: src/main.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Add compilation rule for dsrtos_stubs.c
build/obj/dsrtos_stubs.o: src/dsrtos_stubs.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@



#Update your final linking target to include main object
#build/bin/dsrtos_debug.elf: $(PHASE1_OBJECTS) $(PHASE2_OBJECTS) $(COMMON_OBJECTS) $(MAIN_OBJECTS)
#	$(CC) $(LDFLAGS) $^ -o $@

# -----------------------------------------------------------------------------
# COMPILATION RULES - ASSEMBLY FILES
# -----------------------------------------------------------------------------
$(OBJ_DIR)/%.o: $(STARTUP_DIR)/%.s | directories
	$(ECHO) "[AS]  $<"
	$(AS) $(ASFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(ECHO) "[OK]  $@"

# -----------------------------------------------------------------------------
# LINKING RULE
# -----------------------------------------------------------------------------
$(ELF_FILE): $(OBJECTS) $(LDSCRIPT) | directories
	$(ECHO) "[LD]  $@"
	arm-none-eabi-gcc $(OBJECTS) -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -T$(LDSCRIPT) -Wl,-Map=$(MAP_FILE) -Wl,--gc-sections -Wl,--print-memory-usage -o $@ --specs=nosys.specs --specs=nano.specs -lnosys -Wl,--no-undefined -Wl,--start-group -Wl,--end-group -static
	$(ECHO) "[OK]  $@"

# -----------------------------------------------------------------------------
# BINARY GENERATION RULES
# -----------------------------------------------------------------------------
$(BIN_FILE): $(ELF_FILE)
	$(ECHO) "[BIN] $@"
	$(OBJCOPY) -O binary -S $< $@
	$(ECHO) "[OK]  $@"

$(HEX_FILE): $(ELF_FILE)
	$(ECHO) "[HEX] $@"
	$(OBJCOPY) -O ihex -S $< $@
	$(ECHO) "[OK]  $@"

$(LST_FILE): $(ELF_FILE)
	$(ECHO) "[LST] $@"
	$(OBJDUMP) -h -S $< > $@
	$(ECHO) "[OK]  $@"

$(DIS_FILE): $(ELF_FILE)
	$(ECHO) "[DIS] $@"
	$(OBJDUMP) -d -S $< > $@
	$(ECHO) "[OK]  $@"

# -----------------------------------------------------------------------------
# SIZE INFORMATION
# -----------------------------------------------------------------------------
size: $(ELF_FILE)
	$(ECHO) ""
	$(ECHO) "========== Memory Usage =========="
	$(SIZE) -A -d $<
	$(ECHO) ""
	$(ECHO) "========== Section Sizes =========="
	$(SIZE) -B -d $<
	$(ECHO) ""

symbols: $(ELF_FILE)
	$(ECHO) "Generating symbol table..."
	$(NM) -n -S --size-sort $< > $(SYM_FILE)
	$(ECHO) "Symbols saved to $(SYM_FILE)"

disassembly: $(DIS_FILE)
	$(ECHO) "Disassembly saved to $(DIS_FILE)"

# -----------------------------------------------------------------------------
# FLASH PROGRAMMING
# -----------------------------------------------------------------------------
flash: flash-openocd

flash-openocd: $(BIN_FILE)
	$(ECHO) "Flashing via OpenOCD..."
	$(OPENOCD) -f interface/stlink-v2.cfg \
		-f target/stm32f4x.cfg \
		-c "program $< $(FLASH_START) verify reset exit"

flash-jlink: $(BIN_FILE)
	$(ECHO) "Flashing via J-Link..."
	$(ECHO) "device $(MCU)\nsi 1\nspeed 4000\nloadbin $<,$(FLASH_START)\nr\ng\nexit" | $(JLINK)

flash-stlink: $(BIN_FILE)
	$(ECHO) "Flashing via ST-Link..."
	$(STFLASH) write $< $(FLASH_START)

erase:
	$(ECHO) "Erasing flash..."
	$(OPENOCD) -f interface/stlink-v2.cfg \
		-f target/stm32f4x.cfg \
		-c "init; reset halt; flash erase_sector 0 0 11; exit"

# -----------------------------------------------------------------------------
# DEBUGGING
# -----------------------------------------------------------------------------
debug-server:
	$(ECHO) "Starting OpenOCD debug server..."
	$(OPENOCD) -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

debug-client: $(ELF_FILE)
	$(ECHO) "Starting GDB client..."
	$(GDB) -ex "target remote localhost:3333" \
		-ex "monitor reset halt" \
		-ex "load" \
		-ex "break main" \
		-ex "continue" \
		$<

# -----------------------------------------------------------------------------
# STATIC ANALYSIS
# -----------------------------------------------------------------------------
analyze: cppcheck splint

cppcheck: | $(REPORT_DIR)
	$(ECHO) "Running Cppcheck..."
	$(CPPCHECK) \
		--enable=all \
		--inconclusive \
		--std=c11 \
		--platform=unix32 \
		--suppress=missingIncludeSystem \
		--inline-suppr \
		--force \
		--xml \
		--xml-version=2 \
		$(INCLUDES) \
		$(C_SOURCES) \
		2> $(REPORT_DIR)/cppcheck.xml
	$(ECHO) "Cppcheck report: $(REPORT_DIR)/cppcheck.xml"

splint: | $(REPORT_DIR)
	$(ECHO) "Running Splint..."
	-$(SPLINT) \
		-weak \
		+bounds \
		-preproc \
		$(INCLUDES) \
		$(C_SOURCES) \
		> $(REPORT_DIR)/splint.txt 2>&1
	$(ECHO) "Splint report: $(REPORT_DIR)/splint.txt"

misra: | $(REPORT_DIR)
	$(ECHO) "Running MISRA-C:2012 check..."
	$(ECHO) "Note: Requires PC-Lint Plus installation"

# -----------------------------------------------------------------------------
# TESTING
# -----------------------------------------------------------------------------
test:
	$(ECHO) "Building and running tests..."
	$(MAKE) -C $(TEST_DIR) all

coverage: | $(COV_DIR)
	$(ECHO) "Generating coverage report..."
	$(LCOV) --capture --directory $(OBJ_DIR) --output-file $(COV_DIR)/coverage.info
	$(GENHTML) $(COV_DIR)/coverage.info --output-directory $(COV_DIR)/html
	$(ECHO) "Coverage report: $(COV_DIR)/html/index.html"

# -----------------------------------------------------------------------------
# DOCUMENTATION
# -----------------------------------------------------------------------------
docs:
	$(ECHO) "Generating documentation..."
	$(DOXYGEN) $(DOC_DIR)/Doxyfile
	$(ECHO) "Documentation: $(DOC_DIR)/html/index.html"

# -----------------------------------------------------------------------------
# CLEAN TARGETS
# -----------------------------------------------------------------------------
clean:
	$(ECHO) "Cleaning build artifacts..."
	$(RM) $(BUILD_DIR)

clean-all: clean
	$(ECHO) "Cleaning all generated files..."
	$(RM) $(DOC_DIR)/html
	$(RM) $(DOC_DIR)/latex

# -----------------------------------------------------------------------------
# INFORMATION TARGETS
# -----------------------------------------------------------------------------
info:
	$(ECHO) "=========================================="
	$(ECHO) "DSRTOS Build Information"
	$(ECHO) "=========================================="
	$(ECHO) "Project:        $(PROJECT_NAME)"
	$(ECHO) "Version:        $(VERSION)"
	$(ECHO) "Build Number:   $(BUILD_NUMBER)"
	$(ECHO) "Build Date:     $(BUILD_DATE)"
	$(ECHO) "Build Type:     $(BUILD_TYPE)"
	$(ECHO) "Target MCU:     $(MCU)"
	$(ECHO) "CPU:            $(CPU)"
	$(ECHO) "Toolchain:      $(TOOLCHAIN_PREFIX)"
	$(ECHO) ""
	$(ECHO) "Source Files:"
	$(ECHO) "  Phase 1:      $(words $(PHASE1_C_SOURCES)) files"
	$(ECHO) "  Phase 2:      $(words $(PHASE2_C_SOURCES)) files"
	$(ECHO) "  Common:       $(words $(COMMON_C_SOURCES)) files"
	$(ECHO) "  Total C:      $(words $(C_SOURCES)) files"
	$(ECHO) "  Total ASM:    $(words $(ASM_SOURCES)) files"
	$(ECHO) ""
	$(ECHO) "Output Files:"
	$(ECHO) "  ELF:          $(ELF_FILE)"
	$(ECHO) "  BIN:          $(BIN_FILE)"
	$(ECHO) "  HEX:          $(HEX_FILE)"
	$(ECHO) "=========================================="

version:
	$(ECHO) "DSRTOS Version $(VERSION) (Build: $(BUILD_NUMBER))"

help:
	$(ECHO) "DSRTOS Makefile - Available Targets"
	$(ECHO) "===================================="
	$(ECHO) ""
	$(ECHO) "Build Targets:"
	$(ECHO) "  all/build     - Build entire project (default: debug)"
	$(ECHO) "  debug         - Build with debug configuration"
	$(ECHO) "  release       - Build with release configuration"
	$(ECHO) "  cert          - Build with certification configuration"
	$(ECHO) "  phase1        - Build Phase 1 only"
	$(ECHO) "  phase2        - Build Phase 2 only"
	$(ECHO) ""
	$(ECHO) "Flash Targets:"
	$(ECHO) "  flash         - Flash binary via OpenOCD (default)"
	$(ECHO) "  flash-jlink   - Flash binary via J-Link"
	$(ECHO) "  flash-stlink  - Flash binary via ST-Link"
	$(ECHO) "  erase         - Erase flash memory"
	$(ECHO) ""
	$(ECHO) "Debug Targets:"
	$(ECHO) "  debug-server  - Start OpenOCD debug server"
	$(ECHO) "  debug-client  - Start GDB client"
	$(ECHO) ""
	$(ECHO) "Analysis Targets:"
	$(ECHO) "  analyze       - Run all static analyzers"
	$(ECHO) "  cppcheck      - Run Cppcheck analysis"
	$(ECHO) "  splint        - Run Splint analysis"
	$(ECHO) "  misra         - Run MISRA-C:2012 check"
	$(ECHO) ""
	$(ECHO) "Test Targets:"
	$(ECHO) "  test          - Build and run tests"
	$(ECHO) "  coverage      - Generate coverage report"
	$(ECHO) ""
	$(ECHO) "Utility Targets:"
	$(ECHO) "  size          - Display memory usage"
	$(ECHO) "  symbols       - Generate symbol table"
	$(ECHO) "  disassembly   - Generate disassembly"
	$(ECHO) "  docs          - Generate documentation"
	$(ECHO) "  clean         - Remove build artifacts"
	$(ECHO) "  clean-all     - Remove all generated files"
	$(ECHO) "  info          - Display build information"
	$(ECHO) "  version       - Display version"
	$(ECHO) "  help          - Display this help"
	$(ECHO) ""
	$(ECHO) "Build Options:"
	$(ECHO) "  BUILD_TYPE=[debug|release|cert]"
	$(ECHO) "  V=1           - Verbose output"
	$(ECHO) ""
	$(ECHO) "Examples:"
	$(ECHO) "  make                    # Build debug version"
	$(ECHO) "  make release           # Build release version"
	$(ECHO) "  make cert flash        # Build certified and flash"
	$(ECHO) "  make BUILD_TYPE=debug V=1  # Verbose debug build"

# -----------------------------------------------------------------------------
# DEPENDENCY FILES
# -----------------------------------------------------------------------------
-include $(DEPS)

# -----------------------------------------------------------------------------
# VERBOSE OUTPUT CONTROL
# -----------------------------------------------------------------------------
ifndef V
.SILENT:
endif

# -----------------------------------------------------------------------------
# END OF MAKEFILE
# -----------------------------------------------------------------------------


