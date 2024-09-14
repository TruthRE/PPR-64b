cmake_minimum_required(VERSION 3.10)

# Set project name and version
project(PPR_64b VERSION 1.0)

# Add the executable
add_executable(PPR_64b main.c)

# Target Windows libraries (required for process and thread management)
target_link_libraries(PPR_64b PRIVATE psapi)

# Set C standard version
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Enable all warnings
if (MSVC)
    target_compile_options(PPR_64b PRIVATE /W4)
else()
    target_compile_options(PPR_64b PRIVATE -Wall -Wextra -pedantic)
endif()
