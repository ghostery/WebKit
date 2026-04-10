cmake_minimum_required(VERSION 3.20)
project(validate-dnr-rules CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WEBKIT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Use WebKit's CMake infrastructure for WTF
set(WTF_DIR "${WEBKIT_ROOT}/Source/WTF")
set(JavaScriptCore_SCRIPTS_DIR "${WEBKIT_ROOT}/Source/JavaScriptCore/Scripts")

# Include WebKit's top-level CMake options
include("${WEBKIT_ROOT}/Source/cmake/WebKitCommon.cmake" OPTIONAL)

# Build WTF as a dependency
if (NOT TARGET WTF)
    add_subdirectory("${WEBKIT_ROOT}/Source/WTF" "${CMAKE_BINARY_DIR}/WTF")
endif()

set(CE_DIR "${WEBKIT_ROOT}/Source/WebCore/contentextensions")
set(JSC_DIR "${WEBKIT_ROOT}/Source/JavaScriptCore")

# Content extension sources (compiled from the WebKit tree, not copied)
set(CE_SOURCES
    ${CE_DIR}/URLFilterParser.cpp
    ${CE_DIR}/CombinedURLFilters.cpp
    ${CE_DIR}/CombinedFiltersAlphabet.cpp
    ${CE_DIR}/NFA.cpp
)

# Our tool sources
set(TOOL_SOURCES
    src/main.cpp
    src/stubs.cpp
)

add_executable(validate-dnr-rules ${TOOL_SOURCES} ${CE_SOURCES})

target_compile_definitions(validate-dnr-rules PRIVATE
    ENABLE_CONTENT_EXTENSIONS=1
    WEBCORE_EXPORT=
    WEBCORE_TESTSUPPORT_EXPORT=
    STATICALLY_LINKED_WITH_WTF=1
)

target_include_directories(validate-dnr-rules PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CE_DIR}
)

# Create include symlinks so <WebCore/...> and <JavaScriptCore/...> resolve
set(INCLUDE_BRIDGE_DIR "${CMAKE_BINARY_DIR}/include-bridge")
file(MAKE_DIRECTORY "${INCLUDE_BRIDGE_DIR}")
file(CREATE_LINK "${CE_DIR}" "${INCLUDE_BRIDGE_DIR}/WebCore" SYMBOLIC)
file(CREATE_LINK "${JSC_DIR}" "${INCLUDE_BRIDGE_DIR}/JavaScriptCore" SYMBOLIC)
target_include_directories(validate-dnr-rules SYSTEM PRIVATE ${INCLUDE_BRIDGE_DIR})

target_link_libraries(validate-dnr-rules PRIVATE WTF)

find_package(ICU REQUIRED COMPONENTS uc)
target_link_libraries(validate-dnr-rules PRIVATE ICU::uc)
