include(FetchContent)

# Download from github
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add the main code as a library
add_library(threadpool
    ../src/threadpool.cpp
)

# Define test executable
add_executable(
    threadpool_test
    threadpool.t.cpp
)

# Link your code library and the GTest main target 
# GTest::gtest_main automatically provides a standard main() entry point
target_link_libraries(
    threadpool_test
    PRIVATE
    GTest::gtest_main
    threadpool
)

# Enable automatic test discovery in CMake
include(GoogleTest)
gtest_discover_tests(threadpool_test)
