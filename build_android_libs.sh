#!/bin/bash
set -e

if [[ -z "$ANDROID_NDK_HOME" ]]; then
  echo "❌ ANDROID_NDK_HOME is not set"
  exit 1
fi

echo "✅ Using ANDROID_NDK_HOME = $ANDROID_NDK_HOME"

# Ensure ninja is in PATH
export PATH="/usr/bin:$PATH"

# BUILD_DIR will be inside root ./build/
ROOT_BUILD_DIR="$(pwd)/build"
PROJECT_DIR="$(pwd)"
JNI_LIBS_DIR="$PROJECT_DIR/smollm/src/main/jniLibs"
ABIS=("armeabi-v7a" "arm64-v8a" "x86" "x86_64")

for ABI in "${ABIS[@]}"; do
  echo -e "\n\033[1;32m=== Building for $ABI ===\033[0m"

  BUILD_DIR="$ROOT_BUILD_DIR/build-${ABI}"

  cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM=26 \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja

  cmake --build "$BUILD_DIR" --target smollm
  cmake --build "$BUILD_DIR" --target ggufreader

  ABI_OUT_DIR="$JNI_LIBS_DIR/$ABI"
  mkdir -p "$ABI_OUT_DIR"

  SMOLLM_SO=$(find "$BUILD_DIR" -name "libsmollm.so" | head -n 1)
  GGUF_SO=$(find "$BUILD_DIR" -name "libggufreader.so" | head -n 1)

  if [[ -f "$SMOLLM_SO" && -f "$GGUF_SO" ]]; then
    cp "$SMOLLM_SO" "$ABI_OUT_DIR/"
    cp "$GGUF_SO" "$ABI_OUT_DIR/"
    echo -e "\033[1;34m→ Copied .so files to $ABI_OUT_DIR\033[0m"
  else
    echo -e "\033[1;31m✗ Missing .so files for $ABI\033[0m"
    exit 1
  fi
done

echo -e "\n\033[1;32m✅ All ABIs built and copied successfully.\033[0m"
