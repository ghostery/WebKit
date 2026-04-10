#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
WEBKIT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BDIR="${WEBKIT_ROOT}/WebKitBuild/Debug"
SDIR="${WEBKIT_ROOT}/Source"
CE_DIR="${SDIR}/WebCore/contentextensions"

if [ ! -d "$BDIR/usr/local/include/wtf" ]; then
    echo "Error: WebKit not built. Run: Tools/Scripts/build-webkit --debug" >&2
    exit 1
fi

mkdir -p "$BUILD_DIR" /tmp/validate-dnr-includes

ln -sfn "$CE_DIR" /tmp/validate-dnr-includes/WebCore
ln -sfn "$BDIR/JavaScriptCore.framework/PrivateHeaders" /tmp/validate-dnr-includes/JavaScriptCore

CC_FLAGS=(
    -std=c++2b
    -w
    -fno-exceptions
    -O2
    -I "$SCRIPT_DIR/src"
    -I "$BDIR/usr/local/include"
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
    -F "$BDIR" \
    -framework JavaScriptCore \
    -licucore \
    -Wl,-rpath,"$BDIR" \
    -lc++

DYLD_FRAMEWORK_PATH="$BDIR" "$BUILD_DIR/validate-dnr-rules" --help 2>/dev/null || true
echo ""
echo "Built: $BUILD_DIR/validate-dnr-rules"
echo "Run with: DYLD_FRAMEWORK_PATH=$BDIR $BUILD_DIR/validate-dnr-rules <rules.json>"
