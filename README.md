# diplima_test

## VS Code extensions

- C/C++ Extension Pack (ms-vscode.cpptools-extension-pack)
- CMake Tools (ms-vscode.cmake-tools)
- Qt Tools (tonka3000.qtvsctools)

## Local path setup

The `.vscode` configuration assumes MSYS2 MinGW is installed at `C:/msys64/mingw64`
and vcpkg dependencies at `C:/vcpkg/installed/x64-mingw-static`. Update those paths
to match your local setup if needed.

## MinGW runtime note

CMake enables `ENABLE_PORTABLE_RUNTIME` to add `-static-libgcc -static-libstdc++` for MinGW builds.
If you turn it off, ensure `C:/msys64/mingw64/bin` is on PATH when launching `dos_detector.exe`.
