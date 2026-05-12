import os
import re

def verify():
    hpp_path = 'src/monitor_ui/EventHistoryWidget.hpp'
    cpp_path = 'src/monitor_ui/EventHistoryWidget.cpp'

    # Check .hpp
    with open(hpp_path, 'r') as f:
        hpp_content = f.read()
        if 'enum class BucketType' not in hpp_content:
            print("FAILED: BucketType enum not found in .hpp")
            return False
        if 'static constexpr int NUM_BUCKETS = 48;' not in hpp_content:
            print("FAILED: NUM_BUCKETS constant not found in .hpp")
            return False
        if 'std::vector<BucketType> hourBuckets_;' not in hpp_content:
            print("FAILED: hourBuckets_ type not updated in .hpp")
            return False

    # Check .cpp
    with open(cpp_path, 'r') as f:
        cpp_content = f.read()
        if 'hourBuckets_.resize(NUM_BUCKETS, BucketType::Empty);' not in cpp_content:
            print("FAILED: hourBuckets_ resize not using constants in .cpp")
            return False
        if 'BucketType::Attack' not in cpp_content or 'BucketType::Benign' not in cpp_content:
            print("FAILED: BucketType enum values not used in .cpp")
            return False
        if '48' in cpp_content:
            # Check if 48 is still there as a magic number in relevant places
            # It might still be in comments or strings, but should not be in the logic we touched
            lines = cpp_content.splitlines()
            for i, line in enumerate(lines):
                if '48' in line and 'NUM_BUCKETS' not in line:
                     # Exclude 48 if it's not part of the logic we refactored
                     # In the original code, it was used in for loops and divisions
                     if 'i < 48' in line or '/ 48' in line:
                         print(f"FAILED: Magic number 48 still found at line {i+1}: {line}")
                         return False

    print("PASSED: Static verification of BucketType refactoring successful.")
    return True

if __name__ == "__main__":
    if verify():
        exit(0)
    else:
        exit(1)
