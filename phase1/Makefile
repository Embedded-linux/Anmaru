# DSRTOS Phase 1 Makefile
# Auto-generated - Optimized for low latency

TARGET = dsrtos_phase1
CC = gcc
AS = gcc
LD = gcc

# Compiler flags optimized for RTOS performance
CFLAGS = -O2 -Wall -Wextra
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -I./include -I../include -I../../include

# Linker flags
PHASE1_LDFLAGS = -Wl,--gc-sections

# Handle ARM targets
ifeq ($(findstring arm-none-eabi,$(CC)),arm-none-eabi)
    CFLAGS += -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
    PHASE1_LDFLAGS += -T../../build/link.ld -nostartfiles
    TARGET_EXT = .elf
else
    TARGET_EXT = 
endif

# Source files
C_SRCS = $(wildcard *.c) $(wildcard */*.c)
ASM_SRCS = $(wildcard *.S) $(wildcard */*.S)
OBJS = $(C_SRCS:.c=.o) $(ASM_SRCS:.S=.o)

# Build rules
all: $(TARGET)$(TARGET_EXT)

$(TARGET)$(TARGET_EXT): $(OBJS)
	$(CC) $(PHASE1_LDFLAGS) -o $@ $^
	@echo "Successfully built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)$(TARGET_EXT)

.PHONY: all clean
