project(pytriskel
  VERSION 0.3.0
  DESCRIPTION "Python bindings for the triskel CFG layout library"
  LANGUAGES CXX C)

find_package(Python 3.12 REQUIRED
  COMPONENTS
    Interpreter
    Development.Module
    Development.SABIModule
)

if(SKBUILD)
  # Use the nanobind provided by scikit-build-core
  find_package(nanobind CONFIG REQUIRED)
else()
  # NOTE: We allow fetching nanobind directly here for development. This allows
  # `cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to work as
  # expected and get proper language server support in editors like VSCode.

  # Extract the minimum nanobind version from pyproject.toml
  file(READ "${CMAKE_SOURCE_DIR}/pyproject.toml" PYPROJECT_TOML_CONTENT)
  if(PYPROJECT_TOML_CONTENT MATCHES "\"nanobind *[>=]= *([^\"]+)\"")
    set(NANOBIND_TAG "v${CMAKE_MATCH_1}")
  else()
    message(FATAL_ERROR "Could not find nanobind version in pyproject.toml")
  endif()

  message(STATUS "Fetching nanobind (${NANOBIND_TAG})...")
  FetchContent_Declare(nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind
    GIT_TAG        ${NANOBIND_TAG}
  )
  FetchContent_MakeAvailable(nanobind)
endif()

nanobind_add_module(
  # Name of the extension
  pytriskel

  # Target the stable ABI for Python 3.12+, which reduces
  # the number of binary wheels that must be built
  STABLE_ABI

  # Build libnanobind statically and merge it into the
  # extension (which itself remains a shared library)
  NB_STATIC

  # Extension sources
  pytriskel.cpp
)

target_link_libraries(pytriskel PRIVATE triskel)

# Generate type stubs for the extension module
nanobind_add_stub(
  pytriskel_stub
  MODULE pytriskel
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pytriskel.pyi
  PYTHON_PATH $<TARGET_FILE_DIR:pytriskel>
  MARKER_FILE py.typed
  DEPENDS pytriskel
)

# Install the extension as the pytriskel package:
#   pytriskel/__init__.py         (re-exports, kept in the repository)
#   pytriskel/pytriskel.abi3.so   (the extension module)
#   pytriskel/pytriskel.pyi       (generated type stubs)
#   pytriskel/py.typed            (PEP 561 marker)
install(TARGETS pytriskel
  LIBRARY DESTINATION pytriskel
  COMPONENT python
)
install(FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/pytriskel/__init__.py
  ${CMAKE_CURRENT_BINARY_DIR}/pytriskel.pyi
  ${CMAKE_CURRENT_BINARY_DIR}/py.typed
  DESTINATION pytriskel
  COMPONENT python
)
