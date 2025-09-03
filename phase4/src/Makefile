#******************************************************************************
# DSRTOS Phase 4: Task-Scheduler Interface Makefile
#
# Copyright (C) 2024 DSRTOS
# Version: 1.0.0
# 
# Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D
# MISRA-C:2012 Compliant Build System
#******************************************************************************

#==============================================================================
# Phase Identification
#==============================================================================
PHASE_NAME = Phase4_Task_Scheduler_Interface
PHASE_VERSION = 1.0.0
BUILD_DATE = $(shell date +%Y%m%d)
BUILD_NUMBER ?= 001

#==============================================================================
# Directories
#==============================================================================
# Root directories
TOP_DIR := $(shell pwd)/..
PHASE4_DIR := $(shell pwd)

# Source directories
SRC_DIR = $(PHASE4_DIR)/src
INC_DIR = $(PHASE4_DIR)/include
TEST_DIR = $(PHASE4_DIR)/tests
TOOLS_DIR = $(TOP_DIR)/tools

# Build directories
BUILD_DIR = $(PHASE4_DIR)/build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
DEP_DIR = $(BUILD_DIR)/deps
MAP_DIR = $(BUILD_DIR)/maps
LST_DIR = $(BUILD_DIR)/listings
REPORT_DIR = $(BUILD_DIR)/reports

# Installation directories
INSTALL_PREFIX ?= /usr/local/dsrtos
INSTALL_INC_DIR = $(INSTALL_PREFIX)/include/phase4
INSTALL_LIB_DIR = $(INSTALL_PREFIX)/lib

#==============================================================================
# Toolchain Configuration
#==============================================================================
TOOLCHAIN_PREFIX ?= arm-none-eabi

# Tools
CC = $(TOOLCHAIN_PREFIX)-gcc
AS = $(TOOLCHAIN_PREFIX)-as
AR = $(TOOLCHAIN_PREFIX)-ar
LD = $(TOOLCHAIN_PREFIX)-ld
OBJCOPY = $(TOOLCHAIN_PREFIX)-objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)-objdump
SIZE = $(TOOLCHAIN_PREFIX)-size
NM = $(TOOLCHAIN_PREFIX)-nm
RANLIB = $(TOOLCHAIN_PREFIX)-ranlib

# Host tools
MKDIR = mkdir -p
RM = rm -rf
CP = cp -r
MV = mv
ECHO = @echo
PYTHON = python3

# Analysis tools
CPPCHECK = cppcheck
LINT = pc-lint-plus
GCOV = $(TOOLCHAIN_PREFIX)-gcov
LCOV = lcov
GENHTML = genhtml

#==============================================================================
# Target Configuration
#==============================================================================
MCU = cortex-m4
FPU = fpv4-sp-d16
FLOAT_ABI = hard
ARCH = armv7e-m

# STM32F407 specific
DEVICE = STM32F407xx
HSE_VALUE = 8000000
SYSCLK_FREQ = 168000000

#==============================================================================
# Source Files
#==============================================================================

# Phase 4 C source files
PHASE4_C_SRCS = \
    $(SRC_DIR)/dsrtos_task_scheduler_interface.c \
    $(SRC_DIR)/dsrtos_context_switch.c \
    $(SRC_DIR)/dsrtos_preemption.c \
    $(SRC_DIR)/dsrtos_ready_queue_ops.c \
    $(SRC_DIR)/dsrtos_scheduler_stats.c \
    $(SRC_DIR)/dsrtos_priority_mgmt.c \
    $(SRC_DIR)/dsrtos_scheduler_decision.c \
    $(SRC_DIR)/dsrtos_queue_integrity.c

# Phase 4 Assembly source files
PHASE4_ASM_SRCS = \
    $(SRC_DIR)/dsrtos_context_switch.S

# Phase 4 header files
PHASE4_HEADERS = \
    $(INC_DIR)/dsrtos_task_scheduler_interface.h \
    $(INC_DIR)/dsrtos_context_switch.h \
    $(INC_DIR)/dsrtos_preemption.h \
    $(INC_DIR)/dsrtos_ready_queue_ops.h \
    $(INC_DIR)/dsrtos_scheduler_stats.h \
    $(INC_DIR)/dsrtos_priority_mgmt.h \
    $(INC_DIR)/dsrtos_scheduler_decision.h \
    $(INC_DIR)/dsrtos_queue_integrity.h

# Test source files
TEST_SRCS = \
    $(TEST_DIR)/test_scheduler_interface.c \
    $(TEST_DIR)/test_context_switch.c \
    $(TEST_DIR)/test_ready_queue.c \
    $(TEST_DIR)/test_priority_mgmt.c \
    $(TEST_DIR)/test_integrity.c \
    $(TEST_DIR)/test_safety.c

#==============================================================================
# Object Files
#==============================================================================
PHASE4_C_OBJS = $(PHASE4_C_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
PHASE4_ASM_OBJS = $(PHASE4_ASM_SRCS:$(SRC_DIR)/%.S=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/tests/%.o)

ALL_OBJS = $(PHASE4_C_OBJS) $(PHASE4_ASM_OBJS)

# Dependency files
DEPS = $(PHASE4_C_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

#==============================================================================
# Compiler Flags
#==============================================================================

# Architecture flags
ARCH_FLAGS = -mcpu=$(MCU) -mthumb -mfpu=$(FPU) -mfloat-abi=$(FLOAT_ABI)

# Define flags
DEFINES = -D$(DEVICE)
DEFINES += -DHSE_VALUE=$(HSE_VALUE)
DEFINES += -DSYSCLK_FREQ=$(SYSCLK_FREQ)
DEFINES += -DDSRTOS_PHASE4
DEFINES += -DDSRTOS_USE_FPU=1
DEFINES += -DDSRTOS_MAX_TASKS=256
DEFINES += -DDSRTOS_PRIORITY_LEVELS=256
DEFINES += -DBUILD_NUMBER=$(BUILD_NUMBER)

# Optimization flags
OPT_FLAGS = -O2
OPT_FLAGS += -ffunction-sections
OPT_FLAGS += -fdata-sections
OPT_FLAGS += -fno-common
OPT_FLAGS += -fno-builtin
OPT_FLAGS += -ffreestanding

# Warning flags for safety-critical
WARN_FLAGS = -Wall -Wextra -Werror
WARN_FLAGS += -Wstrict-prototypes
WARN_FLAGS += -Wmissing-prototypes
WARN_FLAGS += -Wmissing-declarations
WARN_FLAGS += -Wredundant-decls
WARN_FLAGS += -Wcast-align
WARN_FLAGS += -Wconversion
WARN_FLAGS += -Wsign-conversion
WARN_FLAGS += -Wshadow
WARN_FLAGS += -Wlogical-op
WARN_FLAGS += -Wfloat-equal
WARN_FLAGS += -Wundef
WARN_FLAGS += -Wunused
WARN_FLAGS += -Wswitch-default
WARN_FLAGS += -Wswitch-enum

# Debug flags
DEBUG_FLAGS = -g3 -gdwarf-2
DEBUG_FLAGS += -DDEBUG
DEBUG_FLAGS += -DDSRTOS_ENABLE_ASSERT=1
DEBUG_FLAGS += -DDSRTOS_ENABLE_TRACE=1

# Coverage flags
COVERAGE_FLAGS = -fprofile-arcs -ftest-coverage

# MISRA flags
MISRA_FLAGS = -DMISRA_C_2012_COMPLIANT

# Include paths
INCLUDES = -I$(INC_DIR)
INCLUDES += -I$(TOP_DIR)/include
INCLUDES += -I$(TOP_DIR)/include/kernel
INCLUDES += -I$(TOP_DIR)/include/arch/arm/cortex-m4

# Combined C flags
CFLAGS = $(ARCH_FLAGS) $(DEFINES) $(OPT_FLAGS) $(WARN_FLAGS)
CFLAGS += $(DEBUG_FLAGS) $(MISRA_FLAGS) $(INCLUDES)
CFLAGS += -std=c99 -MMD -MP

# Assembly flags
ASFLAGS = $(ARCH_FLAGS) $(DEBUG_FLAGS) $(DEFINES)

# Linker flags
LDFLAGS = $(ARCH_FLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(MAP_DIR)/$(PHASE_NAME).map
LDFLAGS += -specs=nano.specs
LDFLAGS += -specs=nosys.specs

# Library flags
ARFLAGS = rcs

#==============================================================================
# Build Configurations
#==============================================================================

# Debug configuration
ifeq ($(CONFIG),debug)
    OPT_FLAGS = -O0
    DEFINES += -DDEBUG=1
    CFLAGS += $(COVERAGE_FLAGS)
endif

# Release configuration
ifeq ($(CONFIG),release)
    OPT_FLAGS = -O3
    DEBUG_FLAGS = -g0
    DEFINES += -DNDEBUG=1
endif

# Safety configuration
ifeq ($(CONFIG),safety)
    OPT_FLAGS = -O2
    DEFINES += -DSAFETY_CRITICAL=1
    CFLAGS += -fstack-protector-all
endif

#==============================================================================
# Targets
#==============================================================================

# Default target
.DEFAULT_GOAL := all

# Phony targets
.PHONY: all clean test install analyze misra coverage docs help
.PHONY: print-vars safety-check cert-check

# Main build target
all: safety-check dirs $(LIB_DIR)/libphase4.a
	$(ECHO) "======================================"
	$(ECHO) "Phase 4 Build Complete"
	$(ECHO) "======================================"
	$(SIZE) $(LIB_DIR)/libphase4.a
	$(ECHO) "======================================"

# Create necessary directories
dirs:
	$(MKDIR) $(OBJ_DIR)
	$(MKDIR) $(OBJ_DIR)/tests
	$(MKDIR) $(BIN_DIR)
	$(MKDIR) $(LIB_DIR)
	$(MKDIR) $(DEP_DIR)
	$(MKDIR) $(MAP_DIR)
	$(MKDIR) $(LST_DIR)
	$(MKDIR) $(REPORT_DIR)

# Safety check
safety-check:
	$(ECHO) "Running safety checks..."
	@if [ ! -f $(TOP_DIR)/include/dsrtos_types.h ]; then \
		echo "Error: Required headers from previous phases not found"; \
		exit 1; \
	fi

# Build static library
$(LIB_DIR)/libphase4.a: $(ALL_OBJS)
	$(ECHO) "AR  $@"
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

# Compile C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(ECHO) "CC  $<"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/$*.d -c $< -o $@
	$(OBJDUMP) -d -S $@ > $(LST_DIR)/$*.lst

# Assemble sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	$(ECHO) "AS  $<"
	$(AS) $(ASFLAGS) -c $< -o $@
	$(OBJDUMP) -d $@ > $(LST_DIR)/$*.lst

# Compile test sources
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.c
	$(ECHO) "CC  $< (test)"
	$(CC) $(CFLAGS) -MF $(DEP_DIR)/tests/$*.d -c $< -o $@

# Include dependencies
-include $(DEPS)

#==============================================================================
# Testing Targets
#==============================================================================

# Build and run tests
test: $(TEST_OBJS) $(LIB_DIR)/libphase4.a
	$(ECHO) "Building test executable..."
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/test_phase4 $(TEST_OBJS) -L$(LIB_DIR) -lphase4
	$(ECHO) "Running tests..."
	$(BIN_DIR)/test_phase4

# Unit tests only
unit-test: $(OBJ_DIR)/tests/test_scheduler_interface.o $(LIB_DIR)/libphase4.a
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/unit_tests $< -L$(LIB_DIR) -lphase4
	$(BIN_DIR)/unit_tests

# Integration tests
integration-test: $(OBJ_DIR)/tests/test_ready_queue.o $(LIB_DIR)/libphase4.a
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/integration_tests $< -L$(LIB_DIR) -lphase4
	$(BIN_DIR)/integration_tests

# Safety tests
safety-test: $(OBJ_DIR)/tests/test_safety.o $(LIB_DIR)/libphase4.a
	$(CC) $(LDFLAGS) -o $(BIN_DIR)/safety_tests $< -L$(LIB_DIR) -lphase4
	$(BIN_DIR)/safety_tests

#==============================================================================
# Analysis Targets
#==============================================================================

# Static analysis with cppcheck
analyze:
	$(ECHO) "Running static analysis..."
	$(CPPCHECK) --enable=all --std=c99 \
		--suppress=missingIncludeSystem \
		--error-exitcode=1 \
		--inline-suppr \
		-I$(INC_DIR) \
		$(PHASE4_C_SRCS)

# MISRA-C:2012 compliance check
misra:
	$(ECHO) "Checking MISRA-C:2012 compliance..."
	$(LINT) --iso=c99 --misra-c=2012 \
		-i$(INC_DIR) \
		$(PHASE4_C_SRCS)

# Code coverage
coverage: CFLAGS += $(COVERAGE_FLAGS)
coverage: clean test
	$(ECHO) "Generating coverage report..."
	$(LCOV) --capture --directory $(OBJ_DIR) \
		--output-file $(REPORT_DIR)/coverage.info
	$(GENHTML) $(REPORT_DIR)/coverage.info \
		--output-directory $(REPORT_DIR)/coverage_html
	$(ECHO) "Coverage report: $(REPORT_DIR)/coverage_html/index.html"

# Certification check
cert-check:
	$(ECHO) "Certification compliance check..."
	$(PYTHON) $(TOOLS_DIR)/cert_check.py \
		--standard DO-178C \
		--level B \
		--source $(SRC_DIR) \
		--report $(REPORT_DIR)/cert_report.txt

#==============================================================================
# Documentation
#==============================================================================

docs:
	$(ECHO) "Generating documentation..."
	doxygen Doxyfile

#==============================================================================
# Installation
#==============================================================================

install: all
	$(ECHO) "Installing Phase 4..."
	$(MKDIR) $(INSTALL_INC_DIR)
	$(MKDIR) $(INSTALL_LIB_DIR)
	$(CP) $(PHASE4_HEADERS) $(INSTALL_INC_DIR)/
	$(CP) $(LIB_DIR)/libphase4.a $(INSTALL_LIB_DIR)/
	$(ECHO) "Installation complete"

uninstall:
	$(ECHO) "Uninstalling Phase 4..."
	$(RM) $(INSTALL_INC_DIR)
	$(RM) $(INSTALL_LIB_DIR)/libphase4.a

#==============================================================================
# Utility Targets
#==============================================================================

# Clean build artifacts
clean:
	$(ECHO) "Cleaning Phase 4 build..."
	$(RM) $(BUILD_DIR)

# Clean everything including dependencies
distclean: clean
	$(RM) $(DEP_DIR)
	$(RM) tags cscope.* .*.swp

# Generate tags
tags:
	ctags -R $(SRC_DIR) $(INC_DIR)

# Print configuration
print-vars:
	$(ECHO) "Build Configuration:"
	$(ECHO) "  PHASE: $(PHASE_NAME) v$(PHASE_VERSION)"
	$(ECHO) "  MCU: $(MCU)"
	$(ECHO) "  TOOLCHAIN: $(TOOLCHAIN_PREFIX)"
	$(ECHO) "  OPTIMIZATION: $(OPT_FLAGS)"
	$(ECHO) "  BUILD_DIR: $(BUILD_DIR)"

# Generate build report
report: all
	$(ECHO) "Generating build report..."
	$(ECHO) "Phase 4 Build Report" > $(REPORT_DIR)/build_report.txt
	$(ECHO) "===================" >> $(REPORT_DIR)/build_report.txt
	$(ECHO) "Date: $(BUILD_DATE)" >> $(REPORT_DIR)/build_report.txt
	$(ECHO) "Build: $(BUILD_NUMBER)" >> $(REPORT_DIR)/build_report.txt
	$(ECHO) "" >> $(REPORT_DIR)/build_report.txt
	$(SIZE) -A $(LIB_DIR)/libphase4.a >> $(REPORT_DIR)/build_report.txt
	$(ECHO) "" >> $(REPORT_DIR)/build_report.txt
	$(NM) --size-sort $(LIB_DIR)/libphase4.a >> $(REPORT_DIR)/build_report.txt

# Help
help:
	$(ECHO) "DSRTOS Phase 4 Makefile"
	$(ECHO) "======================="
	$(ECHO) "Targets:"
	$(ECHO) "  all         - Build Phase 4 library (default)"
	$(ECHO) "  test        - Build and run all tests"
	$(ECHO) "  analyze     - Run static analysis"
	$(ECHO) "  misra       - Check MISRA-C:2012 compliance"
	$(ECHO) "  coverage    - Generate code coverage report"
	$(ECHO) "  docs        - Generate documentation"
	$(ECHO) "  install     - Install headers and library"
	$(ECHO) "  clean       - Remove build artifacts"
	$(ECHO) "  help        - Show this message"
	$(ECHO) ""
	$(ECHO) "Configurations:"
	$(ECHO) "  CONFIG=debug    - Debug build"
	$(ECHO) "  CONFIG=release  - Release build"
	$(ECHO) "  CONFIG=safety   - Safety-critical build"
	$(ECHO) ""
	$(ECHO) "Examples:"
	$(ECHO) "  make all CONFIG=debug"
	$(ECHO) "  make test"
	$(ECHO) "  make install INSTALL_PREFIX=/opt/dsrtos"

#==============================================================================
# End of Makefile
#==============================================================================
