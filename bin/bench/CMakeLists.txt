project(triskel-bench
    VERSION 1.0.0
    DESCRIPTION "Benchmarking binary for the triskel CFG layout library"
    LANGUAGES CXX C
)

add_executable(triskel-bench main.cpp)

target_link_libraries(triskel-bench PRIVATE
  triskel
  fmt::fmt
  gflags
)

if (ENABLE_LINTING)
  find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
    set_target_properties(triskel-bench PROPERTIES
      CXX_CLANG_TIDY ${CLANG_TIDY}
  )
endif()
