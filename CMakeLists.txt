cmake_minimum_required(VERSION 3.16)

project(simpidlog_cpp VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(simpidlog STATIC
    src/simpidlog.cpp
)

add_library(simpidlog::simpidlog ALIAS simpidlog)

target_include_directories(simpidlog
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(simpidlog PUBLIC cxx_std_17)

add_executable(simpidlog_example
    examples/basic.cpp
)

target_link_libraries(simpidlog_example
    PRIVATE
        simpidlog
)

if(BUILD_TESTING)
    add_executable(simpidlog_test
        tests/test_simpidlog.cpp
    )

    target_link_libraries(simpidlog_test
        PRIVATE
            simpidlog
    )

    add_test(NAME simpidlog_test COMMAND simpidlog_test)
endif()
