#!/bin/bash
set -e

export ANDROID_NDK_HOME="$HOME/Android/Sdk/ndk/29.0.13599879"
PROJECT_DIR="$(pwd)"
BUILD_ROOT="$PROJECT_DIR/build"
JNI_LIBS_DIR="$PROJECT_DIR/smollm/src/main/jniLibs"
BUILD_TYPE=Release

ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

mkdir -p "$BUILD_ROOT"
mkdir -p "$JNI_LIBS_DIR"

for ABI in "${ABIS[@]}"; do
  echo -e "\n\033[1;32m=== Building for $ABI ===\033[0m"

  BUILD_DIR="$BUILD_ROOT/build-${ABI}"
  ABI_OUT_DIR="$JNI_LIBS_DIR/$ABI"
  mkdir -p "$ABI_OUT_DIR"

  cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM=26 \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -G Ninja

  # Build main libraries
  cmake --build "$BUILD_DIR" --target smollm
  cmake --build "$BUILD_DIR" --target ggufreader

  # Build optimized variants (only for arm64-v8a)
  if [[ "$ABI" == "arm64-v8a" ]]; then
    OPT_TARGETS=(
      smollm_v8
      smollm_v8_2_fp16
      smollm_v8_2_fp16_dotprod
      smollm_v8_4_fp16_dotprod
      smollm_v8_4_fp16_dotprod_sve
      smollm_v8_4_fp16_dotprod_i8mm
      smollm_v8_4_fp16_dotprod_i8mm_sve
    )
    for target in "${OPT_TARGETS[@]}"; do
      echo -e "\033[0;36m→ Building optional target: $target\033[0m"
      cmake --build "$BUILD_DIR" --target "$target" || echo -e "\033[1;33m⚠️ Skipped optional target: $target\033[0m"
    done
  fi

  # Copy all .so files for this ABI
  find "$BUILD_DIR" -name "*.so" -exec cp {} "$ABI_OUT_DIR/" \;
  echo -e "\033[1;34m→ Copied all .so files to $ABI_OUT_DIR\033[0m"
done

echo -e "\n\033[1;32m✅ All ABIs and targets built successfully. Build folders in ./build/\033[0m"
