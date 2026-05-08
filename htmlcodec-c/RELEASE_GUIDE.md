# Release & Distribution Guide

## Overview

The project now builds three key artifacts:
1. **Static Library** (`htmlcodec-c-lib.a` / `.lib`) - For static linking
2. **Shared Library** (`htmlcodec-c-shared.dll` / `.so` / `.dylib`) - For dynamic linking
3. **Executable** (`htmlcodec-c`) - Command-line tool

## Building Locally

### All Artifacts (Debug + Release)
```bash
cmake -B out/build
cmake --build out/build
```

### Release Build Only
```bash
cmake -B out/build -DCMAKE_BUILD_TYPE=Release
cmake --build out/build
```

### Install to System/Export
```bash
cmake --install out/build --prefix /usr/local
```

## GitHub Actions: Automated Builds & Releases

### Continuous Integration
Every push to `main` and pull requests trigger:
- Builds on Windows (MSVC) and Linux (GCC/Clang)
- Debug and Release configurations
- Runs full test suite
- Uploads Release artifacts as build artifacts (30-day retention)

### Creating a Release Package

1. **Tag a release in git:**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. **GitHub Actions automatically:**
   - Builds Windows Release artifacts
   - Builds Linux Release artifacts
   - Creates a GitHub Release page with all binaries attached

3. **Version Naming:**
   - Stable: `v1.0.0`, `v2.1.3`, etc.
   - Pre-release (alpha): `v1.0.0-alpha`
   - Pre-release (beta): `v1.0.0-beta`

### Release Artifacts Included

**Windows Package:**
- `htmlcodec-c.exe` - Command-line executable
- `htmlcodec-c-shared.dll` - Dynamic library
- `htmlcodec-c-shared.lib` - Import library (for linking)

**Linux Package:**
- `htmlcodec-c` - Command-line executable
- `libhtmlcodec-c-shared.so` - Shared library

## Using the Shared Library

### Windows (C/C++)
```c
#include "cmdline.h"
#include "html-codec.h"

// Link against htmlcodec-c-shared.lib
// Distribute htmlcodec-c-shared.dll with your app
```

### Linux/macOS (C)
```c
#include "cmdline.h"
#include "html-codec.h"

// Compile: gcc app.c -o app -lhtmlcodec-c-shared
// Or set LD_LIBRARY_PATH to library location at runtime
```

### CMake (Recommended)
```cmake
find_library(HTMLCODEC_SHARED htmlcodec-c-shared)
target_link_libraries(myapp PRIVATE ${HTMLCODEC_SHARED})
```

## Public API Headers

The following headers are installed with the shared library:
- `htmlcodec-c.h` - Main header
- `cmdline.h` - Command-line utilities
- `html-codec.h` / `html-tokenizer.h` - HTML encoding
- `css-codec.h` / `css-tokenizer.h` - CSS encoding
- `nl-en-codec.h` / `nl-en-tokenizer.h` - Natural language encoding
- `cl-javascript-codec.h` / `cl-javascript-en-tokenizer.h` - JavaScript encoding
- `nl-en-us-hyphenator.h` - Hyphenation utilities

## Downloading Releases

Visit the [GitHub Releases](../../releases) page to download pre-built binaries for your platform.

## Platform Export Handling

### Windows (MSVC)
- Uses `WINDOWS_EXPORT_ALL_SYMBOLS` for automatic DLL export
- Symbol visibility automatically handled in shared library

### Linux/macOS (GCC/Clang)
- Uses `-fvisibility=hidden` for controlled symbol visibility
- All public API symbols are exported

## Troubleshooting

### Symbol not found when using shared library
- Ensure the shared library is in your library path
- On Linux: `export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH`
- On macOS: `export DYLD_LIBRARY_PATH=./lib:$DYLD_LIBRARY_PATH`
- On Windows: Place DLL in same directory as executable or in PATH

### Build fails for particular platform
- Check GitHub Actions logs: Settings → Actions → Workflow runs
- Local reproduction: Use matching CMakePresets (e.g., `linux-x64-debug`)
