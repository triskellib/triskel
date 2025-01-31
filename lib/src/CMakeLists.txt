add_subdirectory(analysis)
add_subdirectory(graph)
add_subdirectory(layout)
add_subdirectory(renderer)

if(ENABLE_LLVM)
  add_subdirectory(llvm)
endif()

target_sources(triskel PRIVATE
  triskel.cpp
)