#!/bin/bash
# fix_all_phase2_errors.sh

echo "Fixing all compilation errors..."

# 1. Fix dsrtos_error_t if not defined
if ! grep -q "typedef.*dsrtos_error_t" include/common/dsrtos_error.h; then
    # Find where dsrtos_result_t is defined and add error_t after it
    line=$(grep -n "typedef.*dsrtos_result_t" include/common/dsrtos_error.h | cut -d: -f1)
    if [ ! -z "$line" ]; then
        sed -i "${line}a\\typedef dsrtos_result_t dsrtos_error_t;" include/common/dsrtos_error.h
    else
        echo "typedef int32_t dsrtos_error_t;" >> include/common/dsrtos_error.h
    fi
fi

# 2. Fix version macro redefinitions with proper guards
for header in include/phase2/dsrtos_kernel_init.h; do
    if [ -f "$header" ]; then
        # Create temp file with fixed version macros
        awk '
        /#define DSRTOS_VERSION_MAJOR/ {
            if (!printed_major) {
                print "#ifndef DSRTOS_VERSION_MAJOR"
                print $0
                print "#endif"
                printed_major=1
                next
            }
        }
        /#define DSRTOS_VERSION_MINOR/ {
            if (!printed_minor) {
                print "#ifndef DSRTOS_VERSION_MINOR"
                print $0
                print "#endif"
                printed_minor=1
                next
            }
        }
        /#define DSRTOS_VERSION_PATCH/ {
            if (!printed_patch) {
                print "#ifndef DSRTOS_VERSION_PATCH"
                print $0
                print "#endif"
                printed_patch=1
                next
            }
        }
        {print}
        ' "$header" > "$header.tmp"
        mv "$header.tmp" "$header"
    fi
done

# 3. Add missing error codes
if ! grep -q "DSRTOS_SUCCESS" include/common/dsrtos_error.h; then
    cat >> include/common/dsrtos_error.h << 'EOFERR'

/* Standard return codes */
#define DSRTOS_SUCCESS                  0
#define DSRTOS_ERROR_INVALID_STATE     -1
#define DSRTOS_ERROR_INVALID_PARAM     -2
#define DSRTOS_ERROR_NO_MEMORY         -3
#define DSRTOS_ERROR_TIMEOUT           -4
EOFERR
fi

# 4. Fix #endif without #if by checking header guard
for header in include/phase2/*.h; do
    if [ -f "$header" ]; then
        # Count #if* and #endif
        if_count=$(grep -c "^#if\|^#ifndef\|^#ifdef" "$header")
        endif_count=$(grep -c "^#endif" "$header")
        
        if [ "$endif_count" -gt "$if_count" ]; then
            echo "Fixing $header - removing extra #endif"
            # Remove the last #endif if it's extra
            head -n -1 "$header" > "$header.tmp"
            mv "$header.tmp" "$header"
        fi
    fi
done

echo "Fixes applied!"
