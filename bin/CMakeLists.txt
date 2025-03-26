if (BUILD_BENCH OR BUILD_IMG OR BUILD_GUI)
    find_package(gflags)
endif()

if(BUILD_BENCH)
    message(STATUS "Building triskel-bench")
    add_subdirectory(bench)
endif()

if (BUILD_IMG)
    message(STATUS "Building triskel-img")
    add_subdirectory(img)
endif()

if(BUILD_GUI)
    message(STATUS "Building triskel-gui")
    add_subdirectory(gui)
    endif()

if (BUILD_WASM)
    message(STATUS "Building triskel-wasm")
    add_subdirectory(wasm)
endif()

