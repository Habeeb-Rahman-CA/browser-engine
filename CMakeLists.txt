cmake_minimum_required(VERSION 3.25)

project(BrowserEngine
    VERSION 0.1.0
    DESCRIPTION "An educational browser engine built from scratch in C++20"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Build Options
# -----------------------------------------------------------------------------
option(BROWSER_BUILD_TESTS "Build test suite" ON)
option(BROWSER_BUILD_BENCHMARKS "Build benchmarks" ON)
option(BROWSER_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BROWSER_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(BROWSER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# -----------------------------------------------------------------------------
# Compiler Warning Flags
# -----------------------------------------------------------------------------
add_library(browser_compiler_flags INTERFACE)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(browser_compiler_flags INTERFACE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
    )
    if (BROWSER_WARNINGS_AS_ERRORS)
        target_compile_options(browser_compiler_flags INTERFACE -Werror)
    endif()

    if (BROWSER_ENABLE_ASAN)
        target_compile_options(browser_compiler_flags INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(browser_compiler_flags INTERFACE -fsanitize=address)
    endif()

    if (BROWSER_ENABLE_UBSAN)
        target_compile_options(browser_compiler_flags INTERFACE -fsanitize=undefined)
        target_link_options(browser_compiler_flags INTERFACE -fsanitize=undefined)
    endif()
endif()

# -----------------------------------------------------------------------------
# Dependency Resolution: SDL3
# -----------------------------------------------------------------------------
# Check for SDL3 library in standard paths and system library locations
find_library(SDL3_LIBRARY
    NAMES SDL3 libSDL3 libSDL3.so.0 libSDL3.so.0.4.16
    PATHS /usr/lib64 /usr/lib /usr/local/lib
)

# Search for SDL3 headers in system paths or bundled third_party
find_path(SDL3_INCLUDE_DIR
    NAMES SDL3/SDL.h
    PATHS
        /usr/include
        /usr/local/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sdl3/include
)

if (NOT SDL3_LIBRARY OR NOT SDL3_INCLUDE_DIR)
    message(FATAL_ERROR "SDL3 library or headers not found. "
                        "Please verify libSDL3 is installed or third_party/sdl3 headers exist.")
endif()

message(STATUS "Found SDL3 Include: ${SDL3_INCLUDE_DIR}")
message(STATUS "Found SDL3 Library: ${SDL3_LIBRARY}")

add_library(SDL3::SDL3 UNKNOWN IMPORTED)
set_target_properties(SDL3::SDL3 PROPERTIES
    IMPORTED_LOCATION "${SDL3_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
)

# -----------------------------------------------------------------------------
# Dependency Resolution: libcurl
# -----------------------------------------------------------------------------
find_library(CURL_LIBRARY
    NAMES curl libcurl libcurl.so.4 libcurl.so
    PATHS /usr/lib64 /usr/lib /usr/local/lib
)

if (CURL_LIBRARY)
    message(STATUS "Found CURL Library: ${CURL_LIBRARY}")
    add_library(CURL::libcurl UNKNOWN IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES
        IMPORTED_LOCATION "${CURL_LIBRARY}"
    )
endif()

# -----------------------------------------------------------------------------
# Browser Engine Core Library
# -----------------------------------------------------------------------------
set(BROWSER_ENGINE_SOURCES
    src/core/diagnostics.cpp
    src/core/serializer.cpp
    src/platform/window.cpp
    src/events/event_dispatcher.cpp
    src/shell/frame_scheduler.cpp
    src/shell/engine.cpp
    src/html/token.cpp
    src/html/tokenizer.cpp
    src/html/parser.cpp
    src/dom/node.cpp
    src/dom/text_node.cpp
    src/dom/element.cpp
    src/dom/document.cpp
    src/css/css_tokenizer.cpp
    src/css/css_parser.cpp
    src/style/computed_style.cpp
    src/style/selector_matcher.cpp
    src/style/style_resolver.cpp
    src/layout/layout_box.cpp
    src/layout/layout_tree_builder.cpp
    src/layout/layout_engine.cpp
    src/graphics/display_list.cpp
    src/graphics/display_list_builder.cpp
    src/graphics/rasterizer.cpp
    src/networking/url.cpp
    src/networking/resource_loader.cpp
    src/javascript/token.cpp
    src/javascript/lexer.cpp
    src/javascript/value.cpp
    src/javascript/environment.cpp
    src/javascript/parser.cpp
    src/javascript/interpreter.cpp
    src/javascript/dom_bindings.cpp
)

set(BROWSER_ENGINE_HEADERS
    include/browser/core/types.hpp
    include/browser/core/diagnostics.hpp
    include/browser/core/serializer.hpp
    include/browser/platform/event.hpp
    include/browser/platform/window.hpp
    include/browser/events/event_dispatcher.hpp
    include/browser/shell/frame_scheduler.hpp
    include/browser/shell/engine.hpp
    include/browser/html/token.hpp
    include/browser/html/tokenizer.hpp
    include/browser/html/parser.hpp
    include/browser/dom/node.hpp
    include/browser/dom/text_node.hpp
    include/browser/dom/comment_node.hpp
    include/browser/dom/element.hpp
    include/browser/dom/document.hpp
    include/browser/css/css_token.hpp
    include/browser/css/css_tokenizer.hpp
    include/browser/css/stylesheet.hpp
    include/browser/css/css_parser.hpp
    include/browser/style/computed_style.hpp
    include/browser/style/selector_matcher.hpp
    include/browser/style/style_resolver.hpp
    include/browser/layout/dimensions.hpp
    include/browser/layout/layout_box.hpp
    include/browser/layout/layout_tree_builder.hpp
    include/browser/layout/layout_engine.hpp
    include/browser/graphics/display_item.hpp
    include/browser/graphics/display_list.hpp
    include/browser/graphics/display_list_builder.hpp
    include/browser/graphics/rasterizer.hpp
    include/browser/networking/url.hpp
    include/browser/networking/resource.hpp
    include/browser/networking/resource_loader.hpp
    include/browser/javascript/token.hpp
    include/browser/javascript/lexer.hpp
    include/browser/javascript/ast.hpp
    include/browser/javascript/value.hpp
    include/browser/javascript/environment.hpp
    include/browser/javascript/parser.hpp
    include/browser/javascript/interpreter.hpp
    include/browser/javascript/dom_bindings.hpp
)

add_library(browser_core
    ${BROWSER_ENGINE_SOURCES}
    ${BROWSER_ENGINE_HEADERS}
)

target_link_libraries(browser_core
    PUBLIC
        browser_compiler_flags
        SDL3::SDL3
)

if (TARGET CURL::libcurl)
    target_link_libraries(browser_core PUBLIC CURL::libcurl)
endif()

target_include_directories(browser_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# -----------------------------------------------------------------------------
# Browser Shell Executable
# -----------------------------------------------------------------------------
add_executable(browser_shell
    src/shell/main.cpp
)

target_link_libraries(browser_shell
    PRIVATE
        browser_core
        browser_compiler_flags
)

# -----------------------------------------------------------------------------
# Unit & Integration Tests
# -----------------------------------------------------------------------------
if (BROWSER_BUILD_TESTS)
    enable_testing()

    add_executable(browser_tests
        tests/unit/test_diagnostics.cpp
        tests/unit/test_event_dispatcher.cpp
        tests/unit/test_frame_scheduler.cpp
        tests/unit/test_window.cpp
        tests/unit/test_engine.cpp
        tests/unit/test_html_tokenizer.cpp
        tests/unit/test_dom.cpp
        tests/unit/test_css.cpp
        tests/unit/test_layout.cpp
        tests/unit/test_rendering.cpp
        tests/unit/test_networking.cpp
        tests/unit/test_javascript.cpp
        tests/unit/test_dom_bindings.cpp
    )

    target_link_libraries(browser_tests
        PRIVATE
            browser_core
            browser_compiler_flags
    )

    target_include_directories(browser_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    add_test(NAME AllUnitTests COMMAND browser_tests)
endif()

# -----------------------------------------------------------------------------
# Benchmarks
# -----------------------------------------------------------------------------
add_executable(browser_benchmarks
    benchmarks/benchmark_pipeline.cpp
)

target_link_libraries(browser_benchmarks
    PRIVATE
        browser_core
        browser_compiler_flags
)

# -----------------------------------------------------------------------------
# Browser Server Executable (Production Web Host & Workbench)
# -----------------------------------------------------------------------------
add_executable(browser_server
    src/server/http_server.cpp
    src/server/main.cpp
)

target_link_libraries(browser_server
    PRIVATE
        browser_core
        browser_compiler_flags
)

# -----------------------------------------------------------------------------
# Install & Packaging Rules
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS browser_core browser_shell browser_server
    EXPORT BrowserEngineTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/browser
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY examples/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/browser-engine/examples
)

install(DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# CPack configuration
set(CPACK_PACKAGE_NAME "browser-engine")
set(CPACK_PACKAGE_VENDOR "BrowserEngineTeam")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Educational Browser Engine built from scratch in modern C++20")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_GENERATOR "TGZ")
include(CPack)

