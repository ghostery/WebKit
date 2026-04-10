# Plan: Standalone static validate-dnr-rules binary

## Goal

A single statically-linked binary (~5-10MB) that validates DNR rulesets against WebKit's actual URLFilterParser and content extension compiler. Runs on Linux (x86_64, arm64) and macOS without any runtime dependencies.

## Key principle: no code duplication

All WebKit code (content extensions, YARR, WTF) is compiled directly from its original location in the WebKit source tree. The CMake build references files by path — nothing is copied or forked. This means our fixes to URLFilterParser.cpp etc. are automatically picked up, and the validator always tests against the exact same code Safari uses.

New code is limited to: a CMakeLists.txt, a CLI main.cpp, a small C++ DNR translator (reimplemented from the ObjC original since ObjC isn't portable), and a CSS parser stub.

## Architecture

```
validate-dnr-rules (static binary)
├── DNR translator (new C++ code, ~200 lines)
│   └── JSON DNR rule → WebKit content blocker JSON mapping
├── WebCore content extensions (extracted C++)
│   ├── ContentExtensionParser — parses content blocker JSON
│   ├── URLFilterParser — validates regex/URL filter patterns
│   ├── ContentExtensionCompiler — compiles to DFA bytecode
│   ├── NFA/DFA pipeline — NFA→DFA→minimize→bytecode
│   └── CombinedURLFilters — merges URL filter patterns
├── YARR parser (from JavaScriptCore, header-heavy)
│   └── YarrParser.h — regex parsing, template-based, no JSC runtime needed
└── WTF (WebKit Template Framework)
    └── String, Vector, HashMap, JSON, Expected, etc.
```

## Files needed

### Content extensions core (~16 .cpp files)

From `Source/WebCore/contentextensions/`:
```
ContentExtensionRule.cpp
ContentExtensionError.cpp
ContentExtensionParser.cpp          # needs CSS parser stub
ContentExtensionCompiler.cpp
ContentExtensionStringSerialization.cpp
URLFilterParser.cpp                 # our main validation target
CombinedURLFilters.cpp
CombinedFiltersAlphabet.cpp
NFA.cpp
NFAToDFA.cpp
DFA.cpp
DFANode.cpp
DFAMinimizer.cpp
DFACombiner.cpp
DFABytecodeCompiler.cpp
SerializedNFA.cpp
CompiledContentExtension.cpp
```

NOT needed (DOM/browser integration):
- ContentExtensionsBackend.cpp
- ContentExtensionStyleSheet.cpp
- ContentExtension.cpp
- DFABytecodeInterpreter.cpp (runtime matching only)

### YARR parser (~5 .cpp files)

From `Source/JavaScriptCore/yarr/`:
```
YarrParser.h          # header-only template, the main dependency
YarrPattern.cpp
YarrFlags.cpp
YarrErrorCode.cpp
YarrUnicodeProperties.cpp
YarrCanonicalizeUCS2.cpp
```

The YARR parser works standalone via a template delegate pattern — URLFilterParser implements the `YarrSyntaxCheckable` concept. No JSC VM or runtime needed.

### WTF (header-heavy, ~10 .cpp files)

From `Source/WTF/wtf/`:
- String types: `WTF::String`, `StringView`, `CString`, `StringBuilder`
- Containers: `Vector`, `HashMap`, `HashSet`, `Deque`
- JSON: `JSONValues.cpp` (WTF's built-in JSON parser)
- Utilities: `Expected`, `Hasher`, `OptionSet`, `ASCIICType`
- Platform: `FileHandle`, `FileSystem` (for serialized NFA I/O, can be stubbed)

WTF is designed to be the standalone foundation layer — it compiles independently from WebCore.

### New code (~200 lines)

**DNR translator (C++)**: reimplement the JSON mapping from `_WKWebExtensionDeclarativeNetRequestRule.mm`:
- Action type mapping: `block` → `"block"`, `redirect` → `"redirect"`, etc.
- Resource type mapping: `main_frame` → `"top-document"`, `script` → `"script"`, `object` → `"other"`, etc.
- Condition mapping: `urlFilter`/`regexFilter` → `"url-filter"`, `domains` → `"if-domain"`, etc.
- Rule expansion: split rules with multiple requestDomains/requestMethods

**main.cpp**: CLI argument parsing, JSON file I/O, error reporting (similar to current shell script's embedded ObjC).

### CSS parser stub

`ContentExtensionParser.cpp` validates CSS selectors in "css-display-none" actions. For DNR validation (which never produces CSS selectors), we need a stub:

```cpp
// Stub that always returns "valid" — DNR rules don't use CSS selectors
bool isValidCSSSelector(const String&) { return true; }
```

## Build system

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(validate-dnr-rules CXX)
set(CMAKE_CXX_STANDARD 20)

# Point to WebKit source tree
set(WEBKIT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/..")

# WTF
add_subdirectory(${WEBKIT_SOURCE_DIR}/Source/WTF wtf)

# Collect content extension sources
file(GLOB CE_SOURCES ${WEBKIT_SOURCE_DIR}/Source/WebCore/contentextensions/*.cpp)
list(FILTER CE_SOURCES EXCLUDE REGEX "Backend|StyleSheet|ContentExtension\\.cpp|Interpreter")

# YARR sources
set(YARR_SOURCES
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore/yarr/YarrPattern.cpp
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore/yarr/YarrFlags.cpp
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore/yarr/YarrErrorCode.cpp
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore/yarr/YarrUnicodeProperties.cpp
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore/yarr/YarrCanonicalizeUCS2.cpp
)

# Tool sources
set(TOOL_SOURCES
    src/main.cpp
    src/dnr_translator.cpp
    src/css_parser_stub.cpp
)

add_executable(validate-dnr-rules ${TOOL_SOURCES} ${CE_SOURCES} ${YARR_SOURCES})
target_link_libraries(validate-dnr-rules PRIVATE WTF)
target_include_directories(validate-dnr-rules PRIVATE
    ${WEBKIT_SOURCE_DIR}/Source/WebCore
    ${WEBKIT_SOURCE_DIR}/Source/JavaScriptCore
    ${WEBKIT_SOURCE_DIR}/Source/WTF
)

# Static linking
set_target_properties(validate-dnr-rules PROPERTIES LINK_FLAGS "-static")
```

This is a rough sketch — the actual CMake will need more work to handle WTF's platform abstractions and generated headers.

## Build & distribution

### Building

```bash
cd ghostery/validate-dnr-rules
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# Result: build/validate-dnr-rules (single static binary)
```

### Cross-platform CI

GitHub Actions workflow:
```yaml
jobs:
  build:
    strategy:
      matrix:
        os: [macos-latest, ubuntu-latest]
        arch: [x86_64, arm64]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      - run: cmake --build build
      - uses: actions/upload-artifact@v4
        with:
          name: validate-dnr-rules-${{ matrix.os }}-${{ matrix.arch }}
          path: build/validate-dnr-rules
```

### Homebrew formula

```ruby
class ValidateDnrRules < Formula
  desc "Validate Safari Declarative Net Request rulesets against WebKit's engine"
  homepage "https://github.com/ghostery/WebKit"
  # Download prebuilt binary from GitHub Releases
  url "https://github.com/ghostery/WebKit/releases/download/v1.0/validate-dnr-rules-macos-arm64.tar.gz"
  sha256 "..."

  def install
    bin.install "validate-dnr-rules"
  end
end
```

## Effort estimate

| Task | Complexity | Notes |
|---|---|---|
| CMake build for WTF standalone | Medium | WTF has its own CMake, may need tweaks |
| Extract content extensions sources | Easy | Just file selection |
| CSS parser stub | Easy | ~10 lines |
| DNR translator in C++ | Easy | ~200 lines, direct port from ObjC |
| main.cpp CLI | Easy | ~100 lines |
| Fix compilation issues | Medium | Missing includes, platform ifdefs |
| CI pipeline | Easy | Standard GitHub Actions |
| **Total** | **~2-3 days** | Most time on WTF build integration |

## Risks

1. **WTF standalone build complexity** — WTF generates headers (`wtf/PlatformHave.h`, `wtf/PlatformEnable.h`) via CMake. Getting this right outside the full WebKit build is the main risk.
2. **ContentExtensionParser CSS dependency** — if stubbing isn't clean enough, may need to extract more WebCore CSS code.
3. **Platform ifdefs** — content extensions code has `#if ENABLE(CONTENT_EXTENSIONS)` guards everywhere. Need to ensure this is defined.
4. **Static linking on macOS** — fully static binaries aren't supported on macOS (system libraries must be dynamic). On macOS it would be "mostly static" with dynamic libc/libSystem. On Linux, fully static with musl is possible.

## Alternative: simpler approach

If the CMake extraction proves too painful, a simpler alternative:

1. Build WebKit GTK port on Linux (`Tools/Scripts/build-webkit --gtk`)
2. Write the tool as a GLib/C++ program linking against the built WebCore
3. Distribute the binary + libWebCore.so

This trades binary size and portability for build simplicity, since the GTK port's CMake already handles all the dependency resolution.
