import os
import re

def verify():
    # Check TcpServer.cpp
    with open('src/common/TcpServer.cpp', 'r') as f:
        content = f.read()
        if 'QHostAddress::Any' in content:
            print("FAILED: QHostAddress::Any found in TcpServer.cpp")
            return False
        if 'listen(QHostAddress(address), port)' not in content:
            print("FAILED: Dynamic address binding not found in TcpServer.cpp")
            return False

    # Check TcpServer.hpp
    with open('src/common/TcpServer.hpp', 'r') as f:
        content = f.read()
        if 'bool startListening(const QString& address = "127.0.0.1", quint16 port = 50050);' not in content:
            print("FAILED: New startListening signature not found in TcpServer.hpp")
            return False

    # Check ConfigManager.hpp
    with open('src/common/ConfigManager.hpp', 'r') as f:
        content = f.read()
        if 'std.string tcpBindHost = "127.0.0.1";' not in content and 'std::string tcpBindHost = "127.0.0.1";' not in content:
             print("FAILED: tcpBindHost not found in ConfigManager.hpp")
             return False

    # Check collector_main.cpp
    with open('src/collector_main.cpp', 'r') as f:
        content = f.read()
        if '"tcp-host"' not in content:
            print("FAILED: --tcp-host option not found in collector_main.cpp")
            return False
        if 'tcpServer.startListening(tcpHost, tcpPort)' not in content:
            print("FAILED: tcpHost not passed to startListening in collector_main.cpp")
            return False

    print("PASSED: Static verification successful.")
    return True

if __name__ == "__main__":
    if verify():
        exit(0)
    else:
        exit(1)
