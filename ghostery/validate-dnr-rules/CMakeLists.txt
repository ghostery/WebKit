# This file is included from the WebKit root CMakeLists.txt via add_subdirectory.
# It expects the full WebKit CMake infrastructure to be available (WTF target, etc.)
#
# Build with:
#   cmake -B build -G Ninja -DPORT=GTK -DENABLE_TOOLS=OFF -DENABLE_WEBCORE=OFF \
#         -DENABLE_WEBKIT=OFF -DENABLE_CONTENT_EXTENSIONS=ON <webkit-root>
#   cmake --build build --target validate-dnr-rules

set(CE_DIR "${CMAKE_SOURCE_DIR}/Source/WebCore/contentextensions")
set(JSC_DIR "${CMAKE_SOURCE_DIR}/Source/JavaScriptCore")
set(TOOL_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(CE_SOURCES
    ${CE_DIR}/URLFilterParser.cpp
    ${CE_DIR}/CombinedURLFilters.cpp
    ${CE_DIR}/CombinedFiltersAlphabet.cpp
    ${CE_DIR}/NFA.cpp
)

set(TOOL_SOURCES
    ${TOOL_DIR}/src/main.cpp
    ${TOOL_DIR}/src/stubs.cpp
)

add_executable(validate-dnr-rules ${TOOL_SOURCES} ${CE_SOURCES})

target_compile_definitions(validate-dnr-rules PRIVATE
    ENABLE_CONTENT_EXTENSIONS=1
    WEBCORE_EXPORT=
    WEBCORE_TESTSUPPORT_EXPORT=
    STATICALLY_LINKED_WITH_WTF=1
)

target_include_directories(validate-dnr-rules PRIVATE
    ${TOOL_DIR}/src
    ${CE_DIR}
    ${WTF_DIR}
    ${WTF_DIR}/wtf
    ${CMAKE_BINARY_DIR}
    ${WTF_DERIVED_SOURCES_DIR}
)

# Create include bridge so <WebCore/...> and <JavaScriptCore/...> resolve
set(INCLUDE_BRIDGE_DIR "${CMAKE_BINARY_DIR}/validate-dnr-includes")
file(MAKE_DIRECTORY "${INCLUDE_BRIDGE_DIR}")
file(CREATE_LINK "${CE_DIR}" "${INCLUDE_BRIDGE_DIR}/WebCore" SYMBOLIC)
file(CREATE_LINK "${JSC_DIR}" "${INCLUDE_BRIDGE_DIR}/JavaScriptCore" SYMBOLIC)
target_include_directories(validate-dnr-rules SYSTEM PRIVATE ${INCLUDE_BRIDGE_DIR})

target_link_libraries(validate-dnr-rules PRIVATE WTF)

find_package(ICU REQUIRED COMPONENTS uc)
target_link_libraries(validate-dnr-rules PRIVATE ICU::uc)
