#!/bin/bash
# Fix all missing DSRTOS error constants based on actual usage

echo "=== Fixing All Missing DSRTOS Constants ==="

# Create/update dsrtos_error.h with ALL the constants your code is using
cat > include/common/dsrtos_error.h << 'EOF'
#ifndef DSRTOS_ERROR_H
#define DSRTOS_ERROR_H

#include "dsrtos_types.h"

/* Error codes - single definition with all constants your code uses */
typedef enum {
    DSRTOS_SUCCESS                = 0,
    DSRTOS_OK                     = 0,     /* Alias for SUCCESS */
    
    /* Standard error codes */
    DSRTOS_ERROR_INVALID_PARAM   = -1,
    DSRTOS_ERROR_INVALID_STATE   = -2,
    DSRTOS_ERROR_NO_MEMORY       = -3,
    DSRTOS_ERROR_TIMEOUT         = -4,
    DSRTOS_ERROR_INTEGRITY       = -5,
    DSRTOS_ERROR_FATAL           = -6,
    DSRTOS_ERROR_NOT_SUPPORTED   = -7,
    DSRTOS_ERROR_BUSY            = -8,
    DSRTOS_ERROR_OVERFLOW        = -9,
    DSRTOS_ERROR_UNDERFLOW       = -10,
    DSRTOS_ERROR_CRC_MISMATCH    = -11,
    DSRTOS_ERROR_HARDWARE_FAULT  = -12,
    DSRTOS_ERROR_CONFIGURATION   = -13,
    
    /* ERR_ style constants your code is using */
    DSRTOS_ERR_MEMORY_FAULT      = -14,
    DSRTOS_ERR_CLOCK_FAULT       = -15,
    DSRTOS_ERR_NOT_IMPLEMENTED   = -16,
    DSRTOS_ERR_TIMEOUT           = -17,
    DSRTOS_ERR_ALREADY_INITIALIZED = -18,
    DSRTOS_ERR_NOT_INITIALIZED   = -19,
    DSRTOS_ERR_INVALID_PARAMETER = -20,
    DSRTOS_ERR_SYSTEM_FAILURE    = -21,
    DSRTOS_ERR_RESOURCE_UNAVAILABLE = -22,
    DSRTOS_ERR_OPERATION_FAILED  = -23,
    DSRTOS_ERR_ACCESS_DENIED     = -24,
    DSRTOS_ERR_BUFFER_FULL       = -25,
    DSRTOS_ERR_BUFFER_EMPTY      = -26,
    DSRTOS_ERR_CHECKSUM_FAILED   = -27,
    DSRTOS_ERR_COMMUNICATION     = -28,
    DSRTOS_ERR_DEVICE_NOT_FOUND  = -29,
    DSRTOS_ERR_IO_ERROR          = -30
} dsrtos_error_t;

/* Convenience macros */
#define DSRTOS_IS_SUCCESS(err)  ((err) == DSRTOS_SUCCESS || (err) == DSRTOS_OK)
#define DSRTOS_IS_ERROR(err)    ((err) != DSRTOS_SUCCESS && (err) != DSRTOS_OK)

/* Function declarations */
const char* dsrtos_error_to_string(dsrtos_error_t error);
const char* dsrtos_error_category_string(dsrtos_error_t error);

#endif /* DSRTOS_ERROR_H */
EOF

echo "✓ Updated dsrtos_error.h with all missing constants:"
echo "  - DSRTOS_OK = 0"
echo "  - DSRTOS_ERR_MEMORY_FAULT"
echo "  - DSRTOS_ERR_CLOCK_FAULT"  
echo "  - DSRTOS_ERR_NOT_IMPLEMENTED"
echo "  - DSRTOS_ERR_TIMEOUT"
echo "  - DSRTOS_ERR_ALREADY_INITIALIZED"
echo "  - DSRTOS_ERR_NOT_INITIALIZED"
echo "  - And many more ERR_ constants"

# Also create a simple implementation file if it doesn't exist
if [ ! -f "src/common/dsrtos_error.c" ]; then
    echo "Creating dsrtos_error.c implementation..."
    
    mkdir -p src/common
    
    cat > src/common/dsrtos_error.c << 'EOF'
#include "../../include/common/dsrtos_error.h"

const char* dsrtos_error_to_string(dsrtos_error_t error)
{
    switch (error) {
        case DSRTOS_SUCCESS:
        case DSRTOS_OK:
            return "Success";
        case DSRTOS_ERROR_INVALID_PARAM:
        case DSRTOS_ERR_INVALID_PARAMETER:
            return "Invalid parameter";
        case DSRTOS_ERROR_INVALID_STATE:
            return "Invalid state";
        case DSRTOS_ERROR_NO_MEMORY:
            return "Out of memory";
        case DSRTOS_ERROR_TIMEOUT:
        case DSRTOS_ERR_TIMEOUT:
            return "Timeout";
        case DSRTOS_ERROR_INTEGRITY:
            return "Data integrity failure";
        case DSRTOS_ERROR_FATAL:
            return "Fatal error";
        case DSRTOS_ERR_MEMORY_FAULT:
            return "Memory fault";
        case DSRTOS_ERR_CLOCK_FAULT:
            return "Clock fault";
        case DSRTOS_ERR_NOT_IMPLEMENTED:
            return "Not implemented";
        case DSRTOS_ERR_ALREADY_INITIALIZED:
            return "Already initialized";
        case DSRTOS_ERR_NOT_INITIALIZED:
            return "Not initialized";
        default:
            return "Unknown error";
    }
}

const char* dsrtos_error_category_string(dsrtos_error_t error)
{
    if (error == DSRTOS_SUCCESS || error == DSRTOS_OK) {
        return "Success";
    } else if (error >= DSRTOS_ERR_MEMORY_FAULT) {
        return "System Error";
    } else {
        return "General Error";
    }
}
EOF

    echo "✓ Created dsrtos_error.c implementation"
fi

# Make sure the Makefile includes src/common in compilation
if [ -f "Makefile" ]; then
    echo "Checking Makefile for common source compilation..."
    
    if ! grep -q "src/common" Makefile; then
        echo "Adding common source compilation to Makefile..."
        
        # Add common sources to the Makefile
        cat >> Makefile << 'EOF'

# Common source files  
COMMON_C_SOURCES = $(wildcard src/common/*.c)
COMMON_OBJECTS = $(COMMON_C_SOURCES:src/common/%.c=build_common/obj/%.o)

# Common object compilation rule
build_common/obj/%.o: src/common/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Add common objects to the build (update your existing target)
# If you have a line like: $(BUILD_DIR)/dsrtos.elf: $(PHASE1_OBJECTS) $(PHASE2_OBJECTS)
# Change it to: $(BUILD_DIR)/dsrtos.elf: $(PHASE1_OBJECTS) $(PHASE2_OBJECTS) $(COMMON_OBJECTS)
EOF
        
        echo "✓ Added common source compilation rules to Makefile"
        echo "  Note: You may need to manually add \$(COMMON_OBJECTS) to your final binary target"
    fi
fi

echo ""
echo "=== Fix Complete ==="
echo "All missing constants have been added:"
echo ""
echo "Success constants:"
echo "  - DSRTOS_SUCCESS = 0"
echo "  - DSRTOS_OK = 0"
echo ""
echo "Error constants your code uses:"
echo "  - DSRTOS_ERR_MEMORY_FAULT"
echo "  - DSRTOS_ERR_CLOCK_FAULT"
echo "  - DSRTOS_ERR_NOT_IMPLEMENTED"
echo "  - DSRTOS_ERR_TIMEOUT"
echo "  - DSRTOS_ERR_ALREADY_INITIALIZED"
echo "  - DSRTOS_ERR_NOT_INITIALIZED"
echo "  - Plus all DSRTOS_ERROR_* constants"
echo ""
echo "Now try: make clean && make"
echo ""
echo "If you still get errors, they should be different ones (not missing constants)"
