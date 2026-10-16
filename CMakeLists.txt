
cmake_minimum_required(VERSION 3.20)

project(crox LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(${PROJECT_NAME} INTERFACE)

# Linked into shared plugin libraries, so its objects must be
# position-independent or the plugin link fails (R_X86_64_PC32 relocations).
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_subdirectory(crox-base)
add_subdirectory(crox-data-structures)
add_subdirectory(crox-logging)

target_link_libraries(${PROJECT_NAME} INTERFACE
    crox-base
    crox-data-structures
    crox-logging
)



