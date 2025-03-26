project(triskel_test)

enable_testing()

add_executable(triskel_test)

target_sources(triskel_test PRIVATE
  triskel_test.cpp
)

target_link_libraries(triskel_test PRIVATE
triskel
fmt::fmt
GTest::gtest_main
GTest::gtest
)

add_subdirectory(analysis)
add_subdirectory(graph)
add_subdirectory(datatypes)


include(GoogleTest)
gtest_discover_tests(triskel_test)
