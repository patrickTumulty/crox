
cmake_minimum_required(VERSION 3.10)

project(crox-ipc LANGUAGES C)

add_library(${PROJECT_NAME} STATIC)

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/unix_socket_client.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/unix_socket_server.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/unix_socket_shared.c
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
    PRIVATE
        crox-logging
)

if(CROX_BUILD_TESTS)
    find_package(Threads REQUIRED)

    add_executable(${PROJECT_NAME}_tests)

    target_sources(${PROJECT_NAME}_tests
        PRIVATE
            test/main.c
    )

    target_link_libraries(${PROJECT_NAME}_tests
        PUBLIC
            ${PROJECT_NAME}
            crox-logging
            munit
            Threads::Threads
    )

    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()
