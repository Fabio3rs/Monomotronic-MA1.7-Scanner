cmake_minimum_required(VERSION 3.10)
project(imgui_example_glfw_vulkan C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
set(GLFW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw) # Set this to point to an up-to-date GLFW repo
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(SYSTEM ${GLFW_DIR}/include ${CMAKE_SOURCE_DIR}/src/ECUSerial)

# Dear ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
include_directories(SYSTEM ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
add_compile_definitions(PROJECT_DIR="${CMAKE_SOURCE_DIR}")

# Libraries
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})
find_package(Vulkan REQUIRED)
find_package(SQLite3 REQUIRED)
include_directories(${SQLite3_INCLUDE_DIRS})
#find_library(VULKAN_LIBRARY
  #NAMES vulkan vulkan-1)
#set(LIBRARIES "glfw;${VULKAN_LIBRARY}")
set(LIBRARIES "glfw;Vulkan::Vulkan;${FREETYPE_LIBRARIES};SQLite::SQLite3;ecuserial_lib")

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps)

file(GLOB sources *.cpp)
file(GLOB core_sources core/*.cpp)
file(GLOB screen_sources screens/*.cpp)
file(GLOB ui_sources ui/*.cpp)
file(GLOB ui_component_sources ui/components/*.cpp)
file(GLOB ui_modal_sources ui/modals/*.cpp)
file(GLOB data_sources data/*.cpp)
file(GLOB utils_sources utils/*.cpp)
set(test_sources tests/test_RecordingManager.cpp
                 core/RecordingManager.cpp
                 utils/FileIO.cpp)

add_executable(example_glfw_vulkan
    ${sources}
    ${core_sources}
    ${screen_sources}
    ${ui_sources}
    ${ui_component_sources}
    ${ui_modal_sources}
    ${data_sources}
    ${utils_sources}
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

target_link_libraries(example_glfw_vulkan ${LIBRARIES})

# Tests (RecordingManager only to avoid multiple gtest mains)
enable_testing()
add_executable(glfw_tests ${test_sources})
target_include_directories(glfw_tests PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
target_link_libraries(glfw_tests PRIVATE gtest gtest_main Threads::Threads)
add_test(NAME glfw_tests COMMAND glfw_tests)
