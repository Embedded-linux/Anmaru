# ============================================================================
# DSRTOS - Dynamic Scheduler Real-Time Operating System
# Phase 7: Dynamic Scheduler Switching - Makefile
# ============================================================================

# Toolchain Configuration
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
AR = arm-none-eabi-ar
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

# Target Configuration
TARGET = dsrtos_phase7
MCU = cortex-m4
ARCH = armv7e-m
FPU = fpv4-sp-d16
FLOAT_ABI = hard

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = test
DOC_DIR = docs

# Create directories
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Source Files - Phase 7
PHASE7_SRCS = \
    $(SRC_DIR)/dsrtos_switch.c \
    $(SRC_DIR)/dsrtos_migration.c

# Previous Phases Sources (required dependencies)
PREV_SRCS = \
    $(SRC_DIR)/dsrtos_kernel.c \
    $(SRC_DIR)/dsrtos_task.c \
    $(SRC_DIR)/dsrtos_scheduler.c \
    $(SRC_DIR)/dsrtos_critical.c \
    $(SRC_DIR)/dsrtos_memory.c \
    $(SRC_DIR)/dsrtos_rr_scheduler.c \
    $(SRC_DIR)/dsrtos_priority_scheduler.c

# Test Sources
TEST_SRCS = \
    $(TEST_DIR)/test_switch.c \
    $(TEST_DIR)/test_migration.c \
    $(TEST_DIR)/test_rollback.c \
    $(TEST_DIR)/test_framework.c

# All Sources
SRCS = $(PHASE7_SRCS) $(PREV_SRCS)

# Object Files
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)

# Header Dependencies
DEPS = $(OBJS:.o=.d)

# Compiler Flags
CFLAGS = \
    -mcpu=$(MCU) \
    -mthumb \
    -mfpu=$(FPU) \
    -mfloat-abi=$(FLOAT_ABI) \
    -O2 \
    -g3 \
    -Wall \
    -Wextra \
    -Werror \
    -Wno-unused-parameter \
    -Wno-unused-function \
    -ffunction-sections \
    -fdata-sections \
    -fno-common \
    -fno-builtin \
    -std=c99 \
    -pedantic

# MISRA-C:2012 Compliance Flags
MISRA_FLAGS = \
    -Wcast-align \
    -Wconversion \
    -Wsign-conversion \
    -Wstrict-prototypes \
    -Wmissing-prototypes \
    -Wmissing-declarations \
    -Wredundant-decls \
    -Wnested-externs \
    -Wshadow \
    -Wundef \
    -Wunreachable-code \
    -Wlogical-op \
    -Wfloat-equal \
    -Wswitch-default \
    -Wswitch-enum

# Include Paths
INCLUDES = \
    -I$(INC_DIR) \
    -I$(INC_DIR)/phase7 \
    -I$(INC_DIR)/core \
    -I$(INC_DIR)/schedulers

# Linker Flags
LDFLAGS = \
    -mcpu=$(MCU) \
    -mthumb \
    -mfpu=$(FPU) \
    -mfloat-abi=$(FLOAT_ABI) \
    -Wl,--gc-sections \
    -Wl,--print-memory-usage \
    -Wl,-Map=$(BIN_DIR)/$(TARGET).map \
    -T linker_script.ld

# Libraries
LIBS = -lm -lc -lgcc -lnosys

# Define Macros
DEFINES = \
    -DSTM32F407xx \
    -DUSE_HAL_DRIVER \
    -DDEBUG \
    -DDSRTOS_PHASE7 \
    -DMISRA_COMPLIANT

# ============================================================================
# Build Rules
# ============================================================================

.PHONY: all clean test docs analyze coverage profile

# Default Target
all: $(BIN_DIR)/$(TARGET).elf $(BIN_DIR)/$(TARGET).bin $(BIN_DIR)/$(TARGET).hex size

# Compile C Sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) $(MISRA_FLAGS) $(DEFINES) $(INCLUDES) -MMD -MP -c $< -o $@

# Compile Test Sources
$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -DUNIT_TEST -MMD -MP -c $< -o $@

# Link ELF
$(BIN_DIR)/$(TARGET).elf: $(OBJS)
	@echo "[LD] $@"
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

# Create Binary
$(BIN_DIR)/$(TARGET).bin: $(BIN_DIR)/$(TARGET).elf
	@echo "[BIN] $@"
	@$(OBJCOPY) -O binary $< $@

# Create Hex
$(BIN_DIR)/$(TARGET).hex: $(BIN_DIR)/$(TARGET).elf
	@echo "[HEX] $@"
	@$(OBJCOPY) -O ihex $< $@

# Size Information
size: $(BIN_DIR)/$(TARGET).elf
	@echo ""
	@echo "=== Memory Usage ==="
	@$(SIZE) -B $<
	@echo ""
	@$(SIZE) -A $<
	@echo ""

# ============================================================================
# Test Targets
# ============================================================================

# Build and Run Tests
test: test_switch test_migration test_rollback test_performance

# Individual Test Targets
test_switch: $(OBJ_DIR)/test_switch.o $(OBJS)
	@echo "[TEST] Switch Controller Tests"
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $(BIN_DIR)/test_switch
	@$(BIN_DIR)/test_switch

test_migration: $(OBJ_DIR)/test_migration.o $(OBJS)
	@echo "[TEST] Migration Engine Tests"
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $(BIN_DIR)/test_migration
	@$(BIN_DIR)/test_migration

test_rollback: $(OBJ_DIR)/test_rollback.o $(OBJS)
	@echo "[TEST] Rollback Mechanism Tests"
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $(BIN_DIR)/test_rollback
	@$(BIN_DIR)/test_rollback

test_performance: $(OBJ_DIR)/test_performance.o $(OBJS)
	@echo "[TEST] Performance Tests"
	@$(CC) $(LDFLAGS) $^ $(LIBS) -o $(BIN_DIR)/test_performance
	@$(BIN_DIR)/test_performance

# ============================================================================
# Quality Assurance Targets
# ============================================================================

# Static Analysis with Cppcheck
analyze:
	@echo "[ANALYZE] Running static analysis..."
	@cppcheck --enable=all \
	          --std=c99 \
	          --platform=unix32 \
	          --suppress=missingIncludeSystem \
	          --inline-suppr \
	          --force \
	          --xml \
	          --xml-version=2 \
	          $(SRCS) 2> $(BIN_DIR)/cppcheck.xml
	@echo "Analysis complete. Report: $(BIN_DIR)/cppcheck.xml"

# MISRA-C:2012 Compliance Check
misra:
	@echo "[MISRA] Checking MISRA-C:2012 compliance..."
	@pc-lint-plus \
	    -i$(INC_DIR) \
	    -dMISRA_COMPLIANT \
	    +e900 \
	    au-misra3.lnt \
	    $(SRCS) > $(BIN_DIR)/misra_report.txt
	@echo "MISRA check complete. Report: $(BIN_DIR)/misra_report.txt"

# Code Coverage with gcov
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov --coverage
coverage: clean all test
	@echo "[COVERAGE] Generating coverage report..."
	@gcov $(SRCS) -o $(OBJ_DIR)
	@lcov -c -d $(OBJ_DIR) -o $(BIN_DIR)/coverage.info
	@genhtml $(BIN_DIR)/coverage.info -o $(BIN_DIR)/coverage_html
	@echo "Coverage report: $(BIN_DIR)/coverage_html/index.html"

# Performance Profiling with gprof
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean all test
	@echo "[PROFILE] Generating profiling data..."
	@gprof $(BIN_DIR)/$(TARGET).elf gmon.out > $(BIN_DIR)/profile.txt
	@echo "Profile report: $(BIN_DIR)/profile.txt"

# ============================================================================
# Documentation
# ============================================================================

docs:
	@echo "[DOC] Generating documentation..."
	@doxygen Doxyfile
	@echo "Documentation generated in $(DOC_DIR)/html/"

# ============================================================================
# Certification Support
# ============================================================================

# DO-178C Artifacts
do178c:
	@echo "[CERT] Generating DO-178C artifacts..."
	@mkdir -p $(DOC_DIR)/do178c
	@echo "Requirements Traceability Matrix" > $(DOC_DIR)/do178c/rtm.txt
	@echo "Test Coverage Report" > $(DOC_DIR)/do178c/coverage.txt
	@echo "Code Review Checklist" > $(DOC_DIR)/do178c/review.txt

# IEC 62304 Artifacts
iec62304:
	@echo "[CERT] Generating IEC 62304 artifacts..."
	@mkdir -p $(DOC_DIR)/iec62304
	@echo "Software Safety Classification" > $(DOC_DIR)/iec62304/safety.txt
	@echo "Risk Analysis" > $(DOC_DIR)/iec62304/risk.txt
	@echo "Verification Report" > $(DOC_DIR)/iec62304/verification.txt

# ============================================================================
# Utility Targets
# ============================================================================

# Flash to Target
flash: $(BIN_DIR)/$(TARGET).bin
	@echo "[FLASH] Programming target..."
	@openocd -f interface/stlink.cfg \
	         -f target/stm32f4x.cfg \
	         -c "program $< 0x08000000 verify reset exit"

# Debug with GDB
debug: $(BIN_DIR)/$(TARGET).elf
	@echo "[DEBUG] Starting GDB..."
	@arm-none-eabi-gdb $< \
	    -ex "target remote localhost:3333" \
	    -ex "monitor reset halt" \
	    -ex "load" \
	    -ex "break main" \
	    -ex "continue"

# Clean Build Artifacts
clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*
	@rm -f *.gcov *.gcda *.gcno gmon.out

# Clean Everything
distclean: clean
	@echo "[CLEAN] Removing all generated files..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(DOC_DIR)/html
	@rm -f tags TAGS cscope.out

# Generate Tags
tags:
	@echo "[TAGS] Generating tags..."
	@ctags -R $(SRC_DIR) $(INC_DIR)
	@cscope -b -R -s $(SRC_DIR) -s $(INC_DIR)

# ============================================================================
# Performance Benchmarks
# ============================================================================

benchmark: $(BIN_DIR)/$(TARGET).elf
	@echo ""
	@echo "=== Phase 7 Performance Benchmarks ==="
	@echo "--------------------------------------"
	@echo "Switch Time (target):        < 500 μs"
	@echo "Critical Section (target):   < 100 μs"
	@echo "Task Migration (per task):   < 50 μs"
	@echo "State Save/Restore:          < 100 μs"
	@echo "Rollback Time:               < 200 μs"
	@echo "--------------------------------------"
	@$(BIN_DIR)/$(TARGET).elf --benchmark

# ============================================================================
# Help Target
# ============================================================================

help:
	@echo ""
	@echo "DSRTOS Phase 7 Build System"
	@echo "==========================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all       - Build the Phase 7 implementation"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo ""
	@echo "Test Targets:"
	@echo "  test            - Run all tests"
	@echo "  test_switch     - Test switch controller"
	@echo "  test_migration  - Test migration engine"
	@echo "  test_rollback   - Test rollback mechanism"
	@echo ""
	@echo "Quality Targets:"
	@echo "  analyze   - Run static analysis"
	@echo "  misra     - Check MISRA-C:2012 compliance"
	@echo "  coverage  - Generate code coverage report"
	@echo "  profile   - Generate performance profile"
	@echo ""
	@echo "Documentation:"
	@echo "  docs      - Generate Doxygen documentation"
	@echo "  do178c    - Generate DO-178C artifacts"
	@echo "  iec62304  - Generate IEC 62304 artifacts"
	@echo ""
	@echo "Utility:"
	@echo "  flash     - Program target device"
	@echo "  debug     - Start GDB debug session"
	@echo "  tags      - Generate ctags/cscope"
	@echo "  benchmark - Run performance benchmarks"
	@echo ""

# Include Dependencies
-include $(DEPS)

# End of Makefile
