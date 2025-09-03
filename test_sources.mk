PHASE1_C_SOURCES = \
    src/phase1/dsrtos_boot.c \
    src/phase1/dsrtos_clock.c \
    src/phase1/dsrtos_interrupt.c \
    src/phase1/dsrtos_timer.c \
    src/phase1/dsrtos_uart.c

PHASE2_C_SOURCES = \
    src/phase2/dsrtos_kernel_init.c \
    src/phase2/dsrtos_critical.c \
    src/phase2/dsrtos_panic.c \
    src/phase2/dsrtos_syscall.c \
    src/phase2/dsrtos_hooks.c \
    src/phase2/dsrtos_stats.c \
    src/phase2/dsrtos_assert.c

COMMON_C_SOURCES = \
    src/common/dsrtos_memory_stub.c

SYSTEM_C_SOURCES = \
    src/startup/system_stm32f4xx.c

C_SOURCES = \
    $(PHASE1_C_SOURCES) \
    src/dsrtos_stubs.c \
    $(PHASE2_C_SOURCES) \
    $(COMMON_C_SOURCES) \
    $(SYSTEM_C_SOURCES)

test:
	@echo "PHASE1_C_SOURCES = $(PHASE1_C_SOURCES)"
	@echo "PHASE2_C_SOURCES = $(PHASE2_C_SOURCES)"
	@echo "COMMON_C_SOURCES = $(COMMON_C_SOURCES)"
	@echo "SYSTEM_C_SOURCES = $(SYSTEM_C_SOURCES)"
	@echo "C_SOURCES = $(C_SOURCES)"
	@echo "notdir C_SOURCES = $(notdir $(C_SOURCES))"
	@echo "notdir C_SOURCES .c=.o = $(notdir $(C_SOURCES:.c=.o))"

