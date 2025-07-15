cmake_minimum_required(VERSION 3.22.1)
project(NeuroVerse)

set(ANDROID_PLATFORM 26)

# Set common options
set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_SYSTEM_VERSION 26)
set(CMAKE_ANDROID_ARCH_ABI ${ANDROID_ABI})
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_ANDROID_NDK}/build/cmake/android.toolchain.cmake")
set(CMAKE_ANDROID_STL_TYPE c++_static)
set(CMAKE_BUILD_TYPE Release)

# Require Android ABI and toolchain setup
if(NOT DEFINED ANDROID_ABI)
    message(FATAL_ERROR "ANDROID_ABI is not set. Please provide -DANDROID_ABI=... when configuring.")
endif()

# Include the actual library module
add_subdirectory(smollm/src/main/cpp)
