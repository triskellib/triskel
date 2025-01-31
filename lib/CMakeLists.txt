project(triskel
    VERSION 1.0.0
    DESCRIPTION "Triskel: A graphing library for Control Flow Graphs"
    LANGUAGES CXX C)

# if(ENABLE_IMGUI)
add_library(triskel STATIC)
# else()
#   add_library(triskel SHARED)
# endif()
set_property(TARGET triskel PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory(src)

target_include_directories(triskel PUBLIC include)

target_link_libraries(triskel PRIVATE fmt::fmt)

if(ENABLE_LLVM)
  target_link_libraries(triskel PUBLIC LLVM)
  target_compile_definitions(triskel PUBLIC TRISKEL_LLVM)
endif()

if (ENABLE_IMGUI)
  target_compile_definitions(triskel PUBLIC TRISKEL_IMGUI)
  target_link_libraries(triskel PUBLIC imgui::imgui)
endif()

if (ENABLE_CAIRO)
  target_compile_definitions(triskel PUBLIC TRISKEL_CAIRO)
  target_link_libraries(triskel PRIVATE cairo)
endif()

if (ENABLE_LINTING)
  find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
    set_target_properties(triskel PROPERTIES
      CXX_CLANG_TIDY ${CLANG_TIDY}
  )
endif()

install(TARGETS triskel LIBRARY DESTINATION triskel)
