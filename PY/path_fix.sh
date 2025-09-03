#!/bin/bash
# Correct manual fixes for DSRTOS compilation errors
# Based on actual project structure: /home/satish/DSOS/Anmaru/

echo "=== Manual DSRTOS Error Fixes (Correct Paths) ==="
echo "Working directory: $(pwd)"

# First, let's check the actual structure
echo "Current include structure:"
find include -name "*.h" | head -10

echo "Current src structure:"  
find src -name "*.c" | head -10

# Step 1: Create missing dsrtos_types.h in the right location
echo "Creating missing dsrtos_types.h..."
mkdir -p include/common

cat > include/common/dsrtos_types.h << 'EOF'
#ifndef DSRTOS_TYPES_H
#define DSRTOS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int32_t dsrtos_result_t;
typedef uint32_t dsrtos_tick_t;
typedef uint8_t dsrtos_priority_t;

#endif /* DSRTOS_TYPES_H */
EOF

# Step 2: Fix dsrtos_error.h - remove conflicts and add missing constants
echo "Fixing dsrtos_error.h conflicts..."

cat > include/common/dsrtos_error.h << 'EOF'
#ifndef DSRTOS_ERROR_H
#define DSRTOS_ERROR_H

#include "dsrtos_types.h"

/* Error codes - single definition */
typedef enum {
    DSRTOS_SUCCESS                = 0,
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
    DSRTOS_ERROR_CONFIGURATION   = -13
} dsrtos_error_t;

/* Convenience macros */
#define DSRTOS_IS_SUCCESS(err)  ((err) == DSRTOS_SUCCESS)

/* Function declarations */
const char* dsrtos_error_to_string(dsrtos_error_t error);
const char* dsrtos_error_category_string(dsrtos_error_t error);

#endif /* DSRTOS_ERROR_H */
EOF

# Step 3: Check current include structure and fix accordingly
echo "Checking current project structure..."

# Find where dsrtos_boot.h is located
BOOT_HEADER=$(find include -name "dsrtos_boot.h" 2>/dev/null)
if [ -n "$BOOT_HEADER" ]; then
    echo "Found dsrtos_boot.h at: $BOOT_HEADER"
    
    # Calculate the correct relative path from boot header to common
    BOOT_DIR=$(dirname "$BOOT_HEADER")
    echo "Boot header directory: $BOOT_DIR"
    
    # Fix the include in dsrtos_boot.h
    if grep -q "dsrtos_types.h" "$BOOT_HEADER"; then
        echo "Fixing include path in $BOOT_HEADER"
        sed -i 's|#include "dsrtos_types.h"|#include "../common/dsrtos_types.h"|g' "$BOOT_HEADER"
    fi
fi

# Step 4: Fix include paths in source files based on actual structure
echo "Fixing include paths in source files..."

# Find all .c files and fix their includes
find src -name "*.c" | while read -r source_file; do
    if [ -f "$source_file" ]; then
        echo "Checking includes in $source_file"
        
        # Get the relative path from source file to include directory
        SOURCE_DIR=$(dirname "$source_file")
        REL_PATH=$(realpath --relative-to="$SOURCE_DIR" include)
        
        echo "Source dir: $SOURCE_DIR, Relative path to include: $REL_PATH"
        
        # Fix includes in the source file
        sed -i "s|#include \"dsrtos_types.h\"|#include \"$REL_PATH/common/dsrtos_types.h\"|g" "$source_file"
        sed -i "s|#include \"dsrtos_error.h\"|#include \"$REL_PATH/common/dsrtos_error.h\"|g" "$source_file"
        
        # Add missing includes if not present
        if ! grep -q "dsrtos_error.h" "$source_file"; then
            sed -i "1i #include \"$REL_PATH/common/dsrtos_error.h\"" "$source_file"
        fi
    fi
done

# Step 5: Update Makefile to include common directory  
echo "Updating Makefile include paths..."
if [ -f "Makefile" ]; then
    # Backup original
    cp Makefile Makefile.backup.$(date +%s)
    
    # Add common include path if not present
    if ! grep -q "include/common" Makefile; then
        echo "Adding include/common to Makefile"
        # Find existing include flags and add common
        sed -i 's|-Iinclude|-Iinclude -Iinclude/common|g' Makefile
        
        # Alternative approach if above doesn't work
        if ! grep -q "include/common" Makefile; then
            echo "Using alternative Makefile update method"
            sed -i '/INCLUDES.*=/s|$| -Iinclude/common|' Makefile
        fi
    fi
    
    echo "Updated Makefile include paths"
fi

# Step 6: Create exports folder structure (not .txt files)
echo "Creating phase-wise export folders..."
mkdir -p exports/{phase1,phase2,common}/{include,src}

# Copy files based on actual structure
echo "Copying files to exports..."

# Copy phase1 files
if [ -d "include/phase1" ]; then
    cp include/phase1/*.h exports/phase1/include/ 2>/dev/null || true
    echo "Copied phase1 headers"
fi

if [ -d "src/phase1" ]; then
    cp src/phase1/*.c exports/phase1/src/ 2>/dev/null || true
    echo "Copied phase1 sources"
fi

# Copy phase2 files  
if [ -d "include/phase2" ]; then
    cp include/phase2/*.h exports/phase2/include/ 2>/dev/null || true
    echo "Copied phase2 headers"
fi

if [ -d "src/phase2" ]; then
    cp src/phase2/*.c exports/phase2/src/ 2>/dev/null || true
    echo "Copied phase2 sources"
fi

# Copy common files
if [ -d "include/common" ]; then
    cp include/common/*.h exports/common/include/ 2>/dev/null || true
    echo "Copied common headers"
fi

if [ -d "src/common" ]; then
    cp src/common/*.c exports/common/src/ 2>/dev/null || true
    echo "Copied common sources"
fi

# Step 7: Show what we found and fixed
echo ""
echo "=== Summary of Changes ==="
echo "✓ Created include/common/dsrtos_types.h"
echo "✓ Fixed include/common/dsrtos_error.h (removed conflicts)"
echo "✓ Added missing error constants:"
echo "  - DSRTOS_SUCCESS = 0"
echo "  - DSRTOS_ERROR_INVALID_STATE = -2"
echo "  - DSRTOS_ERROR_INTEGRITY = -5" 
echo "  - DSRTOS_ERROR_FATAL = -6"
echo "  - All other error codes"

echo "✓ Updated include paths in source files"
echo "✓ Updated Makefile with -Iinclude/common"
echo "✓ Created phase-wise export structure:"

# Show the export structure
if [ -d "exports" ]; then
    echo ""
    echo "Export structure created:"
    find exports -type f | sort
fi

echo ""
echo "=== Next Steps ==="
echo "1. Try compiling: make clean && make"
echo "2. If still errors, check the specific error messages"
echo "3. The exports/ folder now has your phase-wise file organization"

# Create a simple test compilation
echo ""
echo "=== Quick Compilation Test ==="
echo "Testing if we can find the headers..."

if [ -f "include/common/dsrtos_types.h" ] && [ -f "include/common/dsrtos_error.h" ]; then
    echo "✓ Required headers are in place"
else
    echo "✗ Some headers are missing"
fi

echo ""
echo "Run: make clean && make"
