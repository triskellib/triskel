if (ENABLE_CAIRO)
  target_sources(triskel PRIVATE cairo.cpp)
endif()

if (ENABLE_IMGUI)
  target_sources(triskel PRIVATE imgui.cpp)
  add_subdirectory(external)
endif()
