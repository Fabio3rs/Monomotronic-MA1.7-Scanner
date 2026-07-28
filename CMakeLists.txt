cmake_minimum_required(VERSION 3.10)
project(Monomotronic-MA17-Scanner VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard (required for std::span, std::string::starts_with)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimizations
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )

    # NASA P10 Compliance: Enable AddressSanitizer for development
    option(ENABLE_ASAN "Enable AddressSanitizer for memory safety checks" OFF)
    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Thread support (required for std::thread, std::mutex)
find_package(Threads REQUIRED)

# ========== ECU Serial Library ==========

set(ECU_SERIAL_SOURCES
    src/ECUSerial/ECULinkConfig.cpp
    src/ECUSerial/ECUSerialCLI.cpp
    src/ECUSerial/ECUMonomotronic.cpp
    src/ECUSerial/SerialPort.cpp
    src/ECUSerial/ECUHealthMetrics.cpp
    src/ECUSerial/SensorDecoders.cpp
)

set(ECU_SERIAL_HEADERS
    src/ECUSerial/ECULinkConfig.h
    src/ECUSerial/ECUSerialCLI.h
    src/ECUSerial/ECUMonomotronic.h
    src/ECUSerial/SerialPort.h
    src/ECUSerial/ECUHealthMetrics.h
    src/ECUSerial/CircleMTIO.hpp
    src/ECUSerial/SensorDecoders.h
)

# Create static library for reuse
add_library(ecuserial_lib STATIC
    ${ECU_SERIAL_SOURCES}
    ${ECU_SERIAL_HEADERS}
)

target_include_directories(ecuserial_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECUSerial
)

target_link_libraries(ecuserial_lib PUBLIC
    Threads::Threads
)

# ========== Main Console Application ==========

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/ECUSerial/ECUSerial.cpp")
    add_executable(ecuserial
        src/ECUSerial/ECUSerial.cpp
    )

    target_link_libraries(ecuserial PRIVATE
        ecuserial_lib
    )

    # Installation
    install(TARGETS ecuserial
        RUNTIME DESTINATION bin
    )
endif()

# ========== DLL/Shared Library (Windows Only) ==========

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/dll/MonomotronicMA17.cpp")
    add_library(monomotronic_dll SHARED
        src/dll/MonomotronicMA17.cpp
    )

    target_link_libraries(monomotronic_dll PRIVATE
        ecuserial_lib
    )

    set_target_properties(monomotronic_dll PROPERTIES
        OUTPUT_NAME "MonomotronicMA17"
        PREFIX ""  # Remove "lib" prefix
    )

    install(TARGETS monomotronic_dll
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# ========== Build Information ==========

message(STATUS "")
message(STATUS "Monomotronic MA1.7 Scanner - Build Configuration")
message(STATUS "================================================")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "NASA P10 Compliance: YES")
message(STATUS "AddressSanitizer: ${ENABLE_ASAN}")
message(STATUS "")
message(STATUS "New NASA P10-Compliant Files:")
message(STATUS "  - CircleMTIO.hpp (Fixed-size circular buffer)")
message(STATUS "  - ECUHealthMetrics.{h,cpp} (Health metrics)")
message(STATUS "  - ECUResponseCollection (Fixed-size packet collections)")
message(STATUS "")
message(STATUS "Build Targets:")
if(TARGET ecuserial)
    message(STATUS "  - ecuserial (Console application)")
endif()
if(TARGET monomotronic_dll)
    message(STATUS "  - monomotronic_dll (Shared library)")
endif()
message(STATUS "  - ecuserial_lib (Static library)")
message(STATUS "================================================")
message(STATUS "")

add_subdirectory(scanner_glfw)

# ========== Testing (Optional) ==========

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()

    add_executable(ecuserial_tests
        tests/test_ESP32OptionalFallback.cpp
        tests/test_ESP32OptionalStd.cpp
        tests/test_ECULinkConfig.cpp
        tests/test_ECUSerialCLI.cpp
    )

    target_link_libraries(ecuserial_tests PRIVATE
        ecuserial_lib
        GTest::gtest
        GTest::gtest_main
    )

    add_test(NAME ecuserial_tests COMMAND ecuserial_tests)
endif()

# ========== Documentation ==========

# Build commands are documented in CLAUDE.md:
# Linux:   cmake -B build && cmake --build build
# Windows: cmake -B build && cmake --build build --config Release
#
# NASA P10 Validation:
# cmake -B build -DENABLE_ASAN=ON && cmake --build build
# valgrind --tool=massif ./build/ecuserial
