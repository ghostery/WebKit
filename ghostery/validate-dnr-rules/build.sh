#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEBKIT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
SDIR="${WEBKIT_ROOT}/Source"
CE_DIR="${SDIR}/WebCore/contentextensions"

# Detect platform
if [[ "$(uname)" == "Darwin" ]]; then
    BDIR="${WEBKIT_ROOT}/WebKitBuild/Debug"
    if [ ! -d "$BDIR/usr/local/include/wtf" ]; then
        echo "Error: WebKit not built. Run: Tools/Scripts/build-webkit --debug" >&2
        exit 1
    fi
    WTF_INCLUDE="$BDIR/usr/local/include"
    JSC_INCLUDE="$BDIR/JavaScriptCore.framework/PrivateHeaders"
    ICU_FLAGS="-licucore"
else
    # Linux: use WebKit source tree headers directly
    WTF_INCLUDE="$SDIR/WTF"
    JSC_INCLUDE="$SDIR/JavaScriptCore"
    ICU_FLAGS="$(pkg-config --libs icu-uc 2>/dev/null || echo '-licuuc -licudata')"
fi

mkdir -p "$BUILD_DIR" /tmp/validate-dnr-includes

ln -sfn "$CE_DIR" /tmp/validate-dnr-includes/WebCore
ln -sfn "$JSC_INCLUDE" /tmp/validate-dnr-includes/JavaScriptCore

CC_FLAGS=(
    -std=c++2b
    -w
    -fno-exceptions
    -O2
    -I "$SCRIPT_DIR/src"
    -I "$WTF_INCLUDE"
    -isystem /tmp/validate-dnr-includes
    -I "$CE_DIR"
    -DENABLE_CONTENT_EXTENSIONS=1
    -DWEBCORE_EXPORT=
)

SOURCES=(
    "$CE_DIR/URLFilterParser.cpp"
    "$CE_DIR/CombinedURLFilters.cpp"
    "$CE_DIR/CombinedFiltersAlphabet.cpp"
    "$CE_DIR/NFA.cpp"
    "$SCRIPT_DIR/src/stubs.cpp"
    "$SCRIPT_DIR/src/main.cpp"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
    obj="$BUILD_DIR/$(basename "$src" .cpp).o"
    echo "Compiling $(basename "$src")..."
    clang++ "${CC_FLAGS[@]}" -c -o "$obj" "$src"
    OBJECTS+=("$obj")
done

echo "Linking..."
clang++ -o "$BUILD_DIR/validate-dnr-rules" "${OBJECTS[@]}" \
    $ICU_FLAGS \
    -lc++

echo "Built: $BUILD_DIR/validate-dnr-rules"
echo "Binary size: $(du -h "$BUILD_DIR/validate-dnr-rules" | cut -f1)"
