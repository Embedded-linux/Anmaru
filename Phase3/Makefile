# DSRTOS Master Makefile - Phases 1-3 Integration
# Date: 2024-12-30
# Builds complete DSRTOS foundation (Boot, Kernel, Task Management)
# Compliance: MISRA-C:2012, DO-178C DAL-B

#==============================================================================
# TOOLCHAIN CONFIGURATION
#==============================================================================

# Cross-compiler toolchain
CROSS_COMPILE ?= arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size
GDB = $(CROSS_COMPILE)gdb

# Host tools
MKDIR = mkdir -p
RM = rm -rf
CP = cp -r
ECHO = @echo

#==============================================================================
# TARGET CONFIGURATION
#==============================================================================

# Target architecture
CPU = cortex-m4
FPU = fpv4-sp-d16
FLOAT_ABI = hard
ARCH_FLAGS = -mcpu=$(CPU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

# Target board
BOARD ?= STM32F407
LINKER_SCRIPT = config/$(BOARD).ld

#==============================================================================
# PROJECT STRUCTURE
#==============================================================================

# Root directories
PROJECT_ROOT = .
SRC_DIR = $(PROJECT_ROOT)/src
INC_DIR = $(PROJECT_ROOT)/include
BUILD_DIR = $(PROJECT_ROOT)/build
LIB_DIR = $(PROJECT_ROOT)/lib
DOC_DIR = $(PROJECT_ROOT)/docs
TEST_DIR = $(PROJECT_ROOT)/tests
TOOLS_DIR = $(PROJECT_ROOT)/tools

# Phase directories
PHASE1_DIR = $(SRC_DIR)/arch/arm/cortex-m4
PHASE2_DIR = $(SRC_DIR)/kernel/core
PHASE3_DIR = $(SRC_DIR)/kernel/task

# Include directories
INC_PHASE1 = $(INC_DIR)/arch/arm/cortex-m4
INC_PHASE2 = $(INC_DIR)/kernel/core
INC_PHASE3 = $(INC_DIR)/kernel/task
INC_COMMON = $(INC_DIR)

# Build output directories
OBJ_DIR = $(BUILD_DIR)/obj
DEP_DIR = $(BUILD_DIR)/dep
MAP_DIR = $(BUILD_DIR)/map
LST_DIR = $(BUILD_DIR)/lst

#==============================================================================
# SOURCE FILES
#==============================================================================

# Phase 1: Boot & Board Bring-up
PHASE1_SRCS = \
    $(PHASE1_DIR)/boot.c \
    $(PHASE1_DIR)/system_config.c \
    $(PHASE1_DIR)/clock_config.c \
    $(PHASE1_DIR)/interrupt_handlers.c \
    $(PHASE1_DIR)/timer_hw.c \
    $(PHASE1_DIR)/uart_init.c \
    $(PHASE1_DIR)/memory_init.c

PHASE1_ASM = \
    $(PHASE1_DIR)/startup.s \
    $(PHASE1_DIR)/vectors.s

# Phase 2: Kernel Core
PHASE2_SRCS = \
    $(PHASE2_DIR)/kernel_init.c \
    $(PHASE2_DIR)/kernel_services.c \
    $(PHASE2_DIR)/critical_section.c \
    $(PHASE2_DIR)/system_calls.c \
    $(PHASE2_DIR)/error_handler.c \
    $(PHASE2_DIR)/hooks.c \
    $(PHASE2_DIR)/panic.c

# Phase 3: Task Management
PHASE3_SRCS = \
    $(PHASE3_DIR)/dsrtos_task_manager.c \
    $(PHASE3_DIR)/dsrtos_task_creation.c \
    $(PHASE3_DIR)/dsrtos_stack_manager.c \
    $(PHASE3_DIR)/dsrtos_task_statistics.c \
    $(PHASE3_DIR)/dsrtos_task_queue.c \
    $(PHASE3_DIR)/dsrtos_task_state.c

# All source files
C_SRCS = $(PHASE1_SRCS) $(PHASE2_SRCS) $(PHASE3_SRCS)
ASM_SRCS = $(PHASE1_ASM)

# Object files
C_OBJS = $(C_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
ASM_OBJS = $(ASM_SRCS:$(SRC_DIR)/%.s=$(OBJ_DIR)/%.o)
OBJS = $(C_OBJS) $(ASM_OBJS)

# Dependency files
DEPS = $(C_SRCS:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)

#==============================================================================
# COMPILER FLAGS
#==============================================================================

# Warning flags
WARN_FLAGS = \
    -Wall \
    -Wextra \
    -Werror \
    -Wstrict-prototypes \
    -Wmissing-prototypes \
    -Wmissing-declarations \
    -Wold-style-definition \
    -Wconversion \
    -Wshadow \
    -Wlogical-op \
    -Wfloat-equal \
    -Wcast-qual \
    -Wcast-align \
    -Wwrite-strings \
    -Wredundant-decls \
    -Wnested-externs

# MISRA-C:2012 compliance flags
MISRA_FLAGS = \
    -Wmissing-include-dirs \
    -Wswitch-default \
    -Wswitch-enum \
    -Wunused-parameter \
    -Wuninitialized \
    -Wundef \
    -Wbad-function-cast \
    -Wstrict-overflow=5

# Safety-critical flags
SAFETY_FLAGS = \
    -fno-common \
    -fno-builtin \
    -ffreestanding \
    -fstack-protector-strong \
    -fstack-usage \
    -fcyclomatic-complexity \
    -fno-delete-null-pointer-checks \
    -fno-strict-aliasing \
    -ffunction-sections \
    -fdata-sections

# Optimization flags
OPT_FLAGS = -O2 -g3

# C standard
STD_FLAGS = -std=c99 -pedantic

# Include paths
INCLUDES = \
    -I$(INC_COMMON) \
    -I$(INC_PHASE1) \
    -I$(INC_PHASE2) \
    -I$(INC_PHASE3)

# Preprocessor definitions
DEFINES = \
    -DDSRTOS \
    -DARM_CORTEX_M4 \
    -DSTM32F407 \
    -DDSRTOS_USE_FPU \
    -DMISRA_C_2012 \
    -DHSE_VALUE=8000000 \
    -DSYSCLK_FREQ=168000000

# Combined C flags
CFLAGS = $(ARCH_FLAGS) $(OPT_FLAGS) $(STD_FLAGS) $(WARN_FLAGS) \
         $(MISRA_FLAGS) $(SAFETY_FLAGS) $(INCLUDES) $(DEFINES)

# Assembler flags
ASFLAGS = $(ARCH_FLAGS) $(INCLUDES) $(DEFINES)

# Linker flags
LDFLAGS = $(ARCH_FLAGS) \
    -T$(LINKER_SCRIPT) \
    -Wl,-Map=$(MAP_DIR)/dsrtos.map \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,--no-warn-rwx-segments \
    -specs=nosys.specs \
    -specs=nano.specs

#==============================================================================
# BUILD TARGETS
#==============================================================================

# Default target
.PHONY: all
all: directories $(BUILD_DIR)/dsrtos.elf $(BUILD_DIR)/dsrtos.bin size

# Create necessary directories
.PHONY: directories
directories:
	$(ECHO) "Creating build directories..."
	$(MKDIR) $(BUILD_DIR)
	$(MKDIR) $(OBJ_DIR)/arch/arm/cortex-m4
	$(MKDIR) $(OBJ_DIR)/kernel/core
	$(MKDIR) $(OBJ_DIR)/kernel/task
	$(MKDIR) $(DEP_DIR)/arch/arm/cortex-m4
	$(MKDIR) $(DEP_DIR)/kernel/core
	$(MKDIR) $(DEP_DIR)/kernel/task
	$(MKDIR) $(MAP_DIR)
	$(MKDIR) $(LST_DIR)
	$(MKDIR) $(LIB_DIR)

# Build ELF file
$(BUILD_DIR)/dsrtos.elf: $(OBJS) $(LINKER_SCRIPT)
	$(ECHO) "Linking $@..."
	$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(ECHO) "Generating listings..."
	$(OBJDUMP) -h -S $@ > $(LST_DIR)/dsrtos.lst
	$(OBJDUMP) -D $@ > $(LST_DIR)/dsrtos.dis

# Generate binary file
$(BUILD_DIR)/dsrtos.bin: $(BUILD_DIR)/dsrtos.elf
	$(ECHO) "Creating binary $@..."
	$(OBJCOPY) -O binary $< $@

# Generate hex file
$(BUILD_DIR)/dsrtos.hex: $(BUILD_DIR)/dsrtos.elf
	$(ECHO) "Creating hex file $@..."
	$(OBJCOPY) -O ihex $< $@

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(ECHO) "Compiling $<..."
	$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(CC) $(CFLAGS) -S $< -o $(LST_DIR)/$*.s
	$(CC) $(CFLAGS) -fstack-usage -c $< -o $@

# Assemble ASM sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.s
	$(ECHO) "Assembling $<..."
	$(AS) $(ASFLAGS) $< -o $@

#==============================================================================
# STATIC LIBRARIES
#==============================================================================

# Build static libraries for each phase
.PHONY: libs
libs: $(LIB_DIR)/libdsrtos_boot.a \
      $(LIB_DIR)/libdsrtos_kernel.a \
      $(LIB_DIR)/libdsrtos_task.a

$(LIB_DIR)/libdsrtos_boot.a: $(filter $(OBJ_DIR)/arch/%, $(C_OBJS)) $(ASM_OBJS)
	$(ECHO) "Creating boot library..."
	$(AR) rcs $@ $^

$(LIB_DIR)/libdsrtos_kernel.a: $(filter $(OBJ_DIR)/kernel/core/%, $(C_OBJS))
	$(ECHO) "Creating kernel library..."
	$(AR) rcs $@ $^

$(LIB_DIR)/libdsrtos_task.a: $(filter $(OBJ_DIR)/kernel/task/%, $(C_OBJS))
	$(ECHO) "Creating task library..."
	$(AR) rcs $@ $^

#==============================================================================
# ANALYSIS TARGETS
#==============================================================================

# Display size information
.PHONY: size
size: $(BUILD_DIR)/dsrtos.elf
	$(ECHO) "\n=== Memory Usage ==="
	$(SIZE) -A -d $<
	$(ECHO) "\n=== Section Sizes ==="
	$(SIZE) -B -d $<

# Stack usage analysis
.PHONY: stack
stack:
	$(ECHO) "=== Stack Usage Analysis ==="
	@find $(OBJ_DIR) -name "*.su" -exec cat {} \; | \
		sort -k2 -n -r | head -20

# Cyclomatic complexity analysis
.PHONY: complexity
complexity:
	$(ECHO) "=== Complexity Analysis ==="
	@pmccabe -v $(C_SRCS) | sort -k1 -n -r | head -20

#==============================================================================
# QUALITY ASSURANCE
#==============================================================================

# Run static analysis with PC-lint
.PHONY: lint
lint:
	$(ECHO) "Running PC-lint analysis..."
	pc-lint-plus $(INCLUDES) $(DEFINES) \
		-i$(TOOLS_DIR)/lint std.lnt \
		-i$(TOOLS_DIR)/lint misra-c-2012.lnt \
		$(C_SRCS)

# MISRA compliance check with cppcheck
.PHONY: misra
misra:
	$(ECHO) "Checking MISRA-C:2012 compliance..."
	cppcheck --enable=all --addon=misra \
		--suppress=missingIncludeSystem \
		--inline-suppr \
		$(INCLUDES) $(C_SRCS)

# Run all static analysis
.PHONY: analysis
analysis: lint misra complexity

#==============================================================================
# TESTING
#==============================================================================

# Build and run unit tests
.PHONY: test
test:
	$(ECHO) "Building unit tests..."
	$(MAKE) -C $(TEST_DIR)/unit all
	$(ECHO) "Running unit tests..."
	$(TEST_DIR)/unit/run_tests

# Code coverage
.PHONY: coverage
coverage:
	$(ECHO) "Building with coverage..."
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="$(LDFLAGS) --coverage"
	$(MAKE) test
	lcov --capture --directory $(OBJ_DIR) --output-file coverage.info
	genhtml coverage.info --output-directory $(BUILD_DIR)/coverage
	$(ECHO) "Coverage report: $(BUILD_DIR)/coverage/index.html"

#==============================================================================
# DOCUMENTATION
#==============================================================================

# Generate documentation
.PHONY: docs
docs:
	$(ECHO) "Generating documentation..."
	doxygen $(DOC_DIR)/Doxyfile
	$(ECHO) "Documentation: $(DOC_DIR)/html/index.html"

#==============================================================================
# CERTIFICATION
#==============================================================================

# Generate certification package
.PHONY: cert
cert: all libs analysis test coverage docs
	$(ECHO) "Generating certification package..."
	$(MKDIR) $(BUILD_DIR)/certification
	$(CP) $(BUILD_DIR)/*.elf $(BUILD_DIR)/certification/
	$(CP) $(BUILD_DIR)/*.bin $(BUILD_DIR)/certification/
	$(CP) $(BUILD_DIR)/*.map $(BUILD_DIR)/certification/
	$(CP) $(LIB_DIR)/*.a $(BUILD_DIR)/certification/
	$(CP) $(BUILD_DIR)/coverage $(BUILD_DIR)/certification/
	$(CP) $(DOC_DIR)/html $(BUILD_DIR)/certification/docs
	$(ECHO) "Generating traceability matrix..."
	python3 $(TOOLS_DIR)/gen_trace_matrix.py > $(BUILD_DIR)/certification/trace.csv
	$(ECHO) "Certification package ready: $(BUILD_DIR)/certification/"

#==============================================================================
# DEBUGGING
#==============================================================================

# Start GDB debugging session
.PHONY: debug
debug: $(BUILD_DIR)/dsrtos.elf
	$(GDB) -ex "target remote localhost:3333" \
	       -ex "monitor reset halt" \
	       -ex "load" \
	       -ex "monitor reset init" \
	       $<

# Flash to target using OpenOCD
.PHONY: flash
flash: $(BUILD_DIR)/dsrtos.bin
	openocd -f interface/stlink-v2.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program $< 0x08000000 verify reset exit"

# Start OpenOCD server
.PHONY: openocd
openocd:
	openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg

#==============================================================================
# CLEAN TARGETS
#==============================================================================

# Clean build artifacts
.PHONY: clean
clean:
	$(ECHO) "Cleaning build artifacts..."
	$(RM) $(BUILD_DIR)
	$(RM) $(LIB_DIR)/*.a
	$(RM) coverage.info

# Deep clean including generated docs
.PHONY: distclean
distclean: clean
	$(ECHO) "Deep cleaning..."
	$(RM) $(DOC_DIR)/html
	$(RM) $(DOC_DIR)/latex
	$(RM) tags cscope.*

#==============================================================================
# UTILITY TARGETS
#==============================================================================

# Generate tags for code navigation
.PHONY: tags
tags:
	$(ECHO) "Generating tags..."
	ctags -R $(SRC_DIR) $(INC_DIR)
	cscope -b -R -s $(SRC_DIR) -s $(INC_DIR)

# Show all targets
.PHONY: help
help:
	@echo "DSRTOS Build System - Phases 1-3"
	@echo "================================="
	@echo ""
	@echo "Build targets:"
	@echo "  all       - Build complete DSRTOS (default)"
	@echo "  libs      - Build static libraries"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo ""
	@echo "Analysis targets:"
	@echo "  size      - Display memory usage"
	@echo "  stack     - Analyze stack usage"
	@echo "  complexity- Analyze code complexity"
	@echo "  lint      - Run PC-lint analysis"
	@echo "  misra     - Check MISRA compliance"
	@echo "  analysis  - Run all analysis tools"
	@echo ""
	@echo "Test targets:"
	@echo "  test      - Run unit tests"
	@echo "  coverage  - Generate coverage report"
	@echo ""
	@echo "Debug targets:"
	@echo "  debug     - Start GDB session"
	@echo "  flash     - Flash to target"
	@echo "  openocd   - Start OpenOCD server"
	@echo ""
	@echo "Documentation:"
	@echo "  docs      - Generate Doxygen docs"
	@echo "  cert      - Generate certification package"
	@echo ""
	@echo "Configuration:"
	@echo "  BOARD=$(BOARD)"
	@echo "  CROSS_COMPILE=$(CROSS_COMPILE)"

# Print configuration
.PHONY: info
info:
	@echo "=== Build Configuration ==="
	@echo "Target: $(CPU)"
	@echo "Board: $(BOARD)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Source files: $(words $(C_SRCS)) C, $(words $(ASM_SRCS)) ASM"
	@echo "Build dir: $(BUILD_DIR)"

#==============================================================================
# DEPENDENCIES
#==============================================================================

# Include dependency files
-include $(DEPS)

#==============================================================================
# PHONY TARGETS SUMMARY
#==============================================================================

.PHONY: all directories libs size stack complexity lint misra analysis \
        test coverage docs cert debug flash openocd clean distclean \
        tags help info

# End of Makefile
