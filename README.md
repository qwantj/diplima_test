# diplima_test

## VS Code extensions

- C/C++ Extension Pack (ms-vscode.cpptools-extension-pack)
- CMake Tools (ms-vscode.cmake-tools)
- Qt Tools (tonka3000.qtvsctools)

## Local path setup

The `.vscode` configuration assumes MSYS2 MinGW is installed at `C:/msys64/mingw64`
and vcpkg dependencies at `C:/vcpkg/installed/x64-mingw-static`. Update those paths
to match your local setup if needed.

## Windows-only setup checklist

To continue development on Windows (MinGW x64) you need to install Qt locally.
Recommended steps:

1. Install **MSYS2** and ensure `C:/msys64/mingw64/bin` is on PATH.
2. Install **Qt 6 (MinGW 64-bit)** via the Qt Online Installer.
   - Set `Qt6_DIR` or add the Qt install prefix to `CMAKE_PREFIX_PATH`
     (e.g. `C:/Qt/6.6.2/mingw_64/lib/cmake/Qt6`).
3. Install dependencies with **vcpkg**:
   - `vcpkg install pcapplusplus:x64-mingw-static`
   - `vcpkg install onnxruntime:x64-mingw-static` (optional, for ML inference)
4. Configure CMake:
  - ```
    cmake -S . -B build -G "MinGW Makefiles" ^
      -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
      -DCMAKE_PREFIX_PATH=C:/Qt/6.6.2/mingw_64
    ```

## MinGW runtime note

CMake enables `ENABLE_PORTABLE_RUNTIME` to add `-static-libgcc -static-libstdc++` for MinGW builds.
If you turn it off, ensure `C:/msys64/mingw64/bin` is on PATH when launching `dos_detector.exe`.
