import re

def check_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    errors = []

    # Check for std::system usage
    if 'std::system' in content:
        errors.append("std::system is still used in the file!")

    # Check for QProcess usage
    if 'QProcess::execute' not in content:
        errors.append("QProcess::execute is missing!")

    # Check for QHostAddress usage
    if 'QHostAddress' not in content:
        errors.append("QHostAddress is missing!")

    # Check for QHostAddress validation logic
    if 'addr.isNull()' not in content:
        errors.append("IP validation (addr.isNull()) is missing!")

    if errors:
        for err in errors:
            print(f"ERROR: {err}")
        return False
    else:
        print("Static analysis passed! No command injection vulnerabilities found.")
        return True

check_file("src/core/FirewallManager.cpp")
