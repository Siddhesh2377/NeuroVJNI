# NeuroV JNI Libraries

This module contains all the native JNI code and build scripts required for NeuroV's Android runtime.

## Overview

This directory includes:

* Native C++ sources for performing on-device inference using `llama.cpp`
* Precompiled shared libraries (`.so`) for multiple Android ABIs
* CMake build system integration
* Android NDK toolchain support

## Structure

```
NeuroVerse/
├── smollm/                     # Android module with JNI integration
│   └── src/
│       └── main/
│           ├── cpp/           # Native C++ source code
│           └── jniLibs/       # Output directories for .so files (per ABI)
├── build_android_libs.sh      # Script to build JNI libraries for all Android ABIs
├── external/
│   └── llama.cpp/             # llama.cpp source as a Git submodule
└── CMakeLists.txt             # CMake config for JNI builds
```

## Supported Architectures

This module supports:

* `armeabi-v7a`
* `arm64-v8a`
* `x86`
* `x86_64`

## Dependencies

* Android NDK (r21 or higher)
* CMake (>= 3.22)
* Ninja (recommended)
* Git (for submodules)

## Build Instructions

1. Ensure you have NDK installed and `ANDROID_NDK_HOME` set.
2. Run the build script:

   ```bash
   ./build_android_libs.sh
   ```
3. The resulting `.so` libraries will be placed in `smollm/src/main/jniLibs/<ABI>/`

## Notes

* `llama.cpp` is added as a submodule at a specific commit to ensure stability.
* The native code is optimized for multiple CPU architectures.
* This JNI module is meant to be integrated with the main NeuroV Android application.
