
cmake_minimum_required(VERSION 3.20)

project(crox-data-structures LANGUAGES C)

add_library(${PROJECT_NAME} STATIC)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/matrix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/greedy_matrix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/standard_matrix.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ipv4_address.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)


target_link_libraries(${PROJECT_NAME}
    PUBLIC
        crox-base
)

if(CROX_BUILD_TESTS)
    add_executable(${PROJECT_NAME}_tests)

    target_sources(${PROJECT_NAME}_tests
        PRIVATE
            test/main.c
    )

    target_link_libraries(${PROJECT_NAME}_tests
        PUBLIC
            ${PROJECT_NAME}
            munit
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()

