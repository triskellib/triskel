cmake_minimum_required(VERSION 3.30)

include(CMakeDependentOption)
include(FetchContent)

project(Triskel)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_LLVM      "Adds utilities to convert LLVM functions to triskel graphs"    OFF)
option(ENABLE_IMGUI     "Adds utilities to display graphs in imgui"                     OFF)
option(ENABLE_CAIRO     "Adds utilities to create SVG/PNG using cairo"                  OFF)

option(ENABLE_LINTING   "Linting"                                                       OFF)
option(ENABLE_TESTING   "Tests"                                                         OFF)

cmake_dependent_option(BUILD_BINDINGS  "Builds the python bindings"                OFF "ENABLE_CAIRO; NOT ENABLE_LLVM; NOT ENABLE_IMGUI" OFF)
cmake_dependent_option(BUILD_BENCH     "Builds the binary used for benchmarking"   OFF "ENABLE_LLVM" OFF)
cmake_dependent_option(BUILD_GUI       "Builds the gui binary"                     OFF "ENABLE_LLVM; ENABLE_IMGUI" OFF)
cmake_dependent_option(BUILD_IMG       "Builds the img binary"                     OFF "ENABLE_LLVM; ENABLE_CAIRO" OFF)


# libfmt
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG        0c9fce2ffefecfdce794e1859584e25877b7b592
    FIND_PACKAGE_ARGS
) # 11.0.2
FetchContent_MakeAvailable(fmt)
message(STATUS "Found fmt version ${fmt_VERSION}")

if(ENABLE_TESTING)
    message(STATUS "Building tests")
    find_package(GTest CONFIG REQUIRED)
    add_subdirectory(test)
endif()

if (ENABLE_CAIRO)
    message(STATUS "Building with cairo support")
    include(${PROJECT_SOURCE_DIR}/cmake/FindCairo.cmake)
endif()

if(ENABLE_LLVM)
    message(STATUS "Building with LLVM support")
    find_package(LLVM CONFIG REQUIRED)
    if(LLVM_LINK_LLVM_DYLIB)
        set(llvm_libs LLVM)
    else()
        llvm_map_components_to_libnames(llvm_libs
            support core irreader
            bitreader bitwriter
            passes asmprinter
            aarch64info aarch64desc aarch64codegen aarch64asmparser
            armcodegen armasmparser
            interpreter mcjit
            nvptxdesc
            x86info x86codegen x86asmparser
            sparccodegen sparcasmparser
            webassemblydesc)
    endif()

    message(STATUS "LLVM Libraries: ${llvm_libs}")
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

    string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_PACKAGE_VERSION})
    list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VERSION)
    list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VERSION)

    set(LLVM_MAJOR_VERSION "${LLVM_MAJOR_VERSION}")
    set(LLVM_MINOR_VERSION "${LLVM_MINOR_VERSION}")
endif()

if (ENABLE_IMGUI)
    message(STATUS "Building with ImGui support")
    find_package(imgui REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
endif()

if(BUILD_BINDINGS)
    message(STATUS "Building bindings")
    add_subdirectory(bindings)
endif()

add_subdirectory(bin)
add_subdirectory(lib)
