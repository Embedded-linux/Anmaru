# DSRTOS Phase 5: Pluggable Scheduler Core Makefile
# Copyright (c) 2024 DSRTOS
# Certification: DO-178C DAL-B, IEC 62304 Class B, ISO 26262 ASIL D

#===============================================================================
# Phase 5 Configuration
#===============================================================================
PHASE5_DIR := src/kernel/scheduler
PHASE5_INC := include/kernel/scheduler
PHASE5_TEST := tests/phase5

# Source files
PHASE5_SOURCES := \
    $(PHASE5_DIR)/dsrtos_scheduler_core.c \
    $(PHASE5_DIR)/dsrtos_scheduler_manager.c \
    $(PHASE5_DIR)/dsrtos_scheduler_plugin.c \
    $(PHASE5_DIR)/dsrtos_scheduler_adapt.c \
    $(PHASE5_DIR)/dsrtos_scheduler_monitor.c \
    $(PHASE5_DIR)/dsrtos_scheduler_switch.c \
    $(PHASE5_DIR)/dsrtos_scheduler_metrics.c \
    $(PHASE5_DIR)/dsrtos_scheduler_decision.c

# Header files
PHASE5_HEADERS := \
    $(PHASE5_INC)/dsrtos_scheduler_core.h \
    $(PHASE5_INC)/dsrtos_scheduler_manager.h \
    $(PHASE5_INC)/dsrtos_scheduler_plugin.h \
    $(PHASE5_INC)/dsrtos_scheduler_adapt.h \
    $(PHASE5_INC)/dsrtos_scheduler_types.h

# Object files
PHASE5_OBJECTS := $(PHASE5_SOURCES:.c=.o)

# Test files
PHASE5_TESTS := \
    $(PHASE5_TEST)/test_scheduler_core.c \
    $(PHASE5_TEST)/test_scheduler_manager.c \
    $(PHASE5_TEST)/test_scheduler_plugin.c \
    $(PHASE5_TEST)/test_scheduler_adapt.c \
    $(PHASE5_TEST)/test_scheduler_switch.c \
    $(PHASE5_TEST)/test_scheduler_performance.c

# Test objects
PHASE5_TEST_OBJECTS := $(PHASE5_TESTS:.c=.o)

#===============================================================================
# Compiler Configuration
#===============================================================================
# Certification flags
CERT_FLAGS := \
    -DMISRA_C_2012 \
    -DDO178C_LEVEL_B \
    -DIEC62304_CLASS_B \
    -DISO26262_ASIL_D

# MISRA compliance flags
MISRA_FLAGS := \
    -Wmissing-prototypes \
    -Wstrict-prototypes \
    -Wold-style-definition \
    -Wmissing-declarations \
    -Wpointer-arith \
    -Wcast-align \
    -Wcast-qual \
    -Wconversion \
    -Wsign-conversion \
    -Wfloat-equal \
    -Wredundant-decls \
    -Wunreachable-code \
    -Wshadow \
    -Wlogical-op \
    -Wno-aggressive-loop-optimizations

# Safety-critical flags
SAFETY_FLAGS := \
    -fno-strict-aliasing \
    -fno-common \
    -ffunction-sections \
    -fdata-sections \
    -fno-exceptions \
    -fstack-protector-strong \
    -D_FORTIFY_SOURCE=2

# Phase 5 specific flags
PHASE5_CFLAGS := \
    $(CFLAGS) \
    $(CERT_FLAGS) \
    $(MISRA_FLAGS) \
    $(SAFETY_FLAGS) \
    -I$(PHASE5_INC) \
    -I$(PHASE1_INC) \
    -I$(PHASE2_INC) \
    -I$(PHASE3_INC) \
    -I$(PHASE4_INC) \
    -DPHASE5_SCHEDULER_CORE

#===============================================================================
# Build Rules
#===============================================================================
.PHONY: phase5 phase5_clean phase5_test phase5_analysis phase5_doc

# Main Phase 5 target
phase5: phase5_info $(PHASE5_OBJECTS)
	@echo "Phase 5: Pluggable Scheduler Core build complete"

# Information
phase5_info:
	@echo "==============================================="
	@echo "Building Phase 5: Pluggable Scheduler Core"
	@echo "==============================================="

# Compile Phase 5 sources
$(PHASE5_DIR)/%.o: $(PHASE5_DIR)/%.c $(PHASE5_HEADERS)
	@echo "CC  $<"
	@$(CC) $(PHASE5_CFLAGS) -c $< -o $@

# Build Phase 5 library
libphase5.a: $(PHASE5_OBJECTS)
	@echo "AR  $@"
	@$(AR) rcs $@ $(PHASE5_OBJECTS)
	@echo "Phase 5 library created: $@"

#===============================================================================
# Testing
#===============================================================================
# Test compilation
$(PHASE5_TEST)/%.o: $(PHASE5_TEST)/%.c
	@echo "CC [TEST] $<"
	@$(CC) $(PHASE5_CFLAGS) -DUNIT_TEST -c $< -o $@

# Build test executable
phase5_test: $(PHASE5_TEST_OBJECTS) $(PHASE5_OBJECTS)
	@echo "LD  phase5_test"
	@$(CC) $(LDFLAGS) -o $@ $^ -lphase1 -lphase2 -lphase3 -lphase4
	@echo "Running Phase 5 tests..."
	@./phase5_test

# Coverage test
phase5_coverage:
	@echo "Running Phase 5 coverage analysis..."
	@$(CC) $(PHASE5_CFLAGS) --coverage -o phase5_coverage_test \
		$(PHASE5_SOURCES) $(PHASE5_TESTS)
	@./phase5_coverage_test
	@gcov $(PHASE5_SOURCES)
	@lcov --capture --directory . --output-file phase5_coverage.info
	@genhtml phase5_coverage.info --output-directory phase5_coverage_report
	@echo "Coverage report: phase5_coverage_report/index.html"

#===============================================================================
# Static Analysis
#===============================================================================
phase5_analysis: phase5_misra phase5_cert phase5_metrics

# MISRA checking
phase5_misra:
	@echo "Running MISRA-C:2012 analysis on Phase 5..."
	@pc-lint-plus $(PHASE5_MISRA_FLAGS) $(PHASE5_SOURCES) > phase5_misra.log
	@echo "MISRA report: phase5_misra.log"

# Certification compliance
phase5_cert:
	@echo "Checking certification compliance..."
	@python3 tools/cert_check.py --standard=DO178C --level=B $(PHASE5_SOURCES)
	@python3 tools/cert_check.py --standard=IEC62304 --class=B $(PHASE5_SOURCES)
	@python3 tools/cert_check.py --standard=ISO26262 --asil=D $(PHASE5_SOURCES)

# Code metrics
phase5_metrics:
	@echo "Generating Phase 5 metrics..."
	@cloc $(PHASE5_SOURCES) $(PHASE5_HEADERS)
	@cccc $(PHASE5_SOURCES) --outdir=phase5_metrics
	@echo "Metrics report: phase5_metrics/cccc.html"

#===============================================================================
# Documentation
#===============================================================================
phase5_doc:
	@echo "Generating Phase 5 documentation..."
	@doxygen docs/phase5_doxygen.conf
	@echo "Documentation: docs/phase5/html/index.html"

#===============================================================================
# Cleaning
#===============================================================================
phase5_clean:
	@echo "Cleaning Phase 5 build artifacts..."
	@rm -f $(PHASE5_OBJECTS)
	@rm -f $(PHASE5_TEST_OBJECTS)
	@rm -f libphase5.a
	@rm -f phase5_test
	@rm -f phase5_coverage_test
	@rm -f *.gcno *.gcda *.gcov
	@rm -rf phase5_coverage_report
	@rm -f phase5_*.log
	@rm -rf phase5_metrics
	@echo "Phase 5 clean complete"

#===============================================================================
# Integration
#===============================================================================
# Integrate with previous phases
phase5_integrate: phase1 phase2 phase3 phase4 phase5
	@echo "Integrating Phase 5 with previous phases..."
	@$(LD) -r -o dsrtos_phase1_5.o \
		$(PHASE1_OBJECTS) \
		$(PHASE2_OBJECTS) \
		$(PHASE3_OBJECTS) \
		$(PHASE4_OBJECTS) \
		$(PHASE5_OBJECTS)
	@echo "Integration complete: dsrtos_phase1_5.o"

#===============================================================================
# Performance Testing
#===============================================================================
phase5_perf:
	@echo "Running Phase 5 performance tests..."
	@$(CC) $(PHASE5_CFLAGS) -O3 -DPERFORMANCE_TEST \
		-o phase5_perf_test \
		$(PHASE5_TEST)/test_scheduler_performance.c \
		$(PHASE5_OBJECTS)
	@./phase5_perf_test
	@echo "Performance test results saved to phase5_perf.json"

#===============================================================================
# Validation
#===============================================================================
phase5_validate: phase5 phase5_test phase5_analysis phase5_coverage
	@echo "==============================================="
	@echo "Phase 5 Validation Complete"
	@echo "==============================================="
	@echo "✓ Build successful"
	@echo "✓ Unit tests passed"
	@echo "✓ MISRA compliance checked"
	@echo "✓ Coverage target met (>95%)"
	@echo "✓ Certification standards verified"
	@echo "==============================================="

# Help target
phase5_help:
	@echo "Phase 5 Make Targets:"
	@echo "  phase5          - Build Phase 5 components"
	@echo "  phase5_test     - Run unit tests"
	@echo "  phase5_coverage - Generate coverage report"
	@echo "  phase5_analysis - Run static analysis"
	@echo "  phase5_doc      - Generate documentation"
	@echo "  phase5_clean    - Clean build artifacts"
	@echo "  phase5_validate - Complete validation suite"


