# Creates the same `cairo` target as FindCairo.cmake, but linking cairo and
# all of its dependencies statically, using the --static information from
# pkg-config. The static libraries come from e.g. a vcpkg static triplet,
# made visible through PKG_CONFIG_PATH.

find_package(PkgConfig REQUIRED)
pkg_check_modules(Cairo REQUIRED cairo)

if(NOT Cairo_STATIC_LIBRARIES)
  message(FATAL_ERROR "pkg-config reported no static libraries for cairo")
endif()

# Resolve the library names against the pkg-config library directories so
# the static archives are linked by absolute path on every platform (MSVC
# does not understand the -l flags in Cairo_STATIC_LDFLAGS). Names that do
# not resolve, such as system libraries (m, dl, gdi32), pass through for
# the linker to find.
set(Cairo_RESOLVED_LIBRARIES "")
foreach(name IN LISTS Cairo_STATIC_LIBRARIES)
  find_library(Cairo_STATIC_LIB_${name}
    NAMES ${name}
    HINTS ${Cairo_STATIC_LIBRARY_DIRS}
  )
  if(Cairo_STATIC_LIB_${name})
    list(APPEND Cairo_RESOLVED_LIBRARIES "${Cairo_STATIC_LIB_${name}}")
  else()
    list(APPEND Cairo_RESOLVED_LIBRARIES "${name}")
  endif()
endforeach()

add_library(cairo INTERFACE)

# Also expose the parent include directories (e.g. include/ next to
# include/cairo/) so the <cairo/cairo.h> include form works
set(Cairo_PARENT_INCLUDE_DIRS "")
foreach(dir IN LISTS Cairo_STATIC_INCLUDE_DIRS)
  get_filename_component(parent "${dir}" DIRECTORY)
  list(APPEND Cairo_PARENT_INCLUDE_DIRS "${parent}")
endforeach()
list(REMOVE_DUPLICATES Cairo_PARENT_INCLUDE_DIRS)

target_include_directories(cairo SYSTEM INTERFACE
  ${Cairo_STATIC_INCLUDE_DIRS}
  ${Cairo_PARENT_INCLUDE_DIRS}
)
target_compile_options(cairo INTERFACE ${Cairo_STATIC_CFLAGS_OTHER})

# Group the two-token "-framework Foo" flags (macOS) with SHELL: so
# CMake's link option de-duplication cannot collapse the repeated
# -framework tokens into "-framework Foo Bar"
set(Cairo_LINK_OPTIONS "")
set(Cairo_PENDING_FRAMEWORK FALSE)
foreach(flag IN LISTS Cairo_STATIC_LDFLAGS_OTHER)
  if(flag STREQUAL "-framework")
    set(Cairo_PENDING_FRAMEWORK TRUE)
  elseif(Cairo_PENDING_FRAMEWORK)
    list(APPEND Cairo_LINK_OPTIONS "SHELL:-framework ${flag}")
    set(Cairo_PENDING_FRAMEWORK FALSE)
  else()
    list(APPEND Cairo_LINK_OPTIONS "${flag}")
  endif()
endforeach()

# The archives in dependency order; the remaining flags carry e.g. the
# macOS framework options
target_link_libraries(cairo INTERFACE ${Cairo_RESOLVED_LIBRARIES})
target_link_options(cairo INTERFACE ${Cairo_LINK_OPTIONS})

message(STATUS "Static cairo ${Cairo_VERSION}: ${Cairo_RESOLVED_LIBRARIES}")
