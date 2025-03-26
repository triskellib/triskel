project(triskel-wasm
    VERSION 1.0.0
    DESCRIPTION "WASM version of the triskel CFG layout library"
    LANGUAGES CXX C
)

add_executable(triskel-wasm main.cpp)

include_directories($ENV{EMSDK}/upstream/emscripten/system/include)

target_link_libraries(triskel-wasm PRIVATE
  triskel
  embind
)

if (ENABLE_LINTING)
  find_program(CLANG_TIDY NAMES "clang-tidy" REQUIRED)
    set_target_properties(triskel-img PROPERTIES
      CXX_CLANG_TIDY ${CLANG_TIDY}
  )
endif()

if (WASM_DEBUG)
  set_target_properties(triskel-wasm PROPERTIES LINK_FLAGS "-g4 -gsource-map -sSAFE_HEAP=1 -sASSERTIONS=2 -sSTACK_OVERFLOW_CHECK=2 -sEXCEPTION_DEBUG=1 -sDEMANGLE_SUPPORT=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s TOTAL_STACK=512MB -s INITIAL_MEMORY=1GB -s MAXIMUM_MEMORY=4GB")
  target_link_options(triskel-wasm PRIVATE
    --emit-tsd "triskel-wasm.d.ts"
  )
else()
  set_target_properties(triskel-wasm PROPERTIES COMPILER_FLAGS "-O3")
  set_target_properties(triskel-wasm PROPERTIES LINK_FLAGS "-s WASM=1 -s ALLOW_MEMORY_GROWTH=1")
  target_link_options(triskel-wasm PRIVATE
    --emit-tsd "triskel-wasm.d.ts"
  )
endif()