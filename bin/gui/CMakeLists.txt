project(triskel-gui
    VERSION 1.0.0
    DESCRIPTION "ImGui for the triskel CFG layout library"
    LANGUAGES CXX C
)

add_executable(triskel-gui main.cpp)

FetchContent_Declare(
    LIEF
    GIT_REPOSITORY https://github.com/lief-project/LIEF.git
    GIT_TAG        7e61aa2e56d67b46a0b055363a4cb4fbe4662ef8 # 0.16.3
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(LIEF)

target_link_libraries(triskel-gui PRIVATE
  triskel
  fmt::fmt
  gflags
  imgui::imgui
  imgui::imgui_impl_glfw
  imgui::imgui_impl_opengl3
  glfw
  OpenGL::GL
  GLEW::GLEW
  LIEF::LIEF
)

include(FindPkgConfig)
pkg_check_modules(CAPSTONE REQUIRED capstone)
target_include_directories(triskel-gui PRIVATE ${CAPSTONE_INCLUDE_DIRS})
target_link_libraries(triskel-gui PRIVATE ${CAPSTONE_LIBRARIES})


add_subdirectory(disas)
add_subdirectory(external)
add_subdirectory(gui)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_dependencies(triskel-gui copy_assets)
target_compile_definitions(triskel-gui PRIVATE ASSET_PATH="${CMAKE_CURRENT_BINARY_DIR}/assets")