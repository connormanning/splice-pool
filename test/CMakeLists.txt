add_executable(
    splice-pool-test
    stack.cpp
    object-pool.cpp
    auto-release.cpp
    unit.cpp)

target_link_libraries(splice-pool-test gtest gtest_main)

# We're overriding the test with a custom command for individual test output
# and colors, which cmake doesn't like.
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")

add_custom_target(
    test
    COMMAND splice-pool-test
    DEPENDS splice-pool-test
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

