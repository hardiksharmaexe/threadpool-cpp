cmake_minimum_required(VERSION 3.14)

project("threadpool-cpp")

# Include application source code
add_subdirectory(src)

# Enable CMake's test runner framework
enable_testing()

# Include the tests subdirectory
add_subdirectory(tests)
