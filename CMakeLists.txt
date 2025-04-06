cmake_minimum_required(VERSION 3.10)

project(reaction VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
    add_compile_options(/GR-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
    add_compile_options(-fno-rtti -std=c++20)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/include/reaction/*.h)
target_sources(${PROJECT_NAME} INTERFACE ${HEADERS})

file(GLOB EXAMPLE_SOURCES ${PROJECT_SOURCE_DIR}/example/*.cpp)
foreach(example_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_file} NAME_WE)
    add_executable(${example_name} ${example_file})
    target_link_libraries(${example_name} PRIVATE ${PROJECT_NAME})
endforeach()

find_package(GTest)
if(GTest_FOUND)
    enable_testing()
    file(GLOB TEST_SOURCES ${PROJECT_SOURCE_DIR}/test/*.cpp)
    add_executable(runTests ${TEST_SOURCES})
    target_link_libraries(runTests PRIVATE GTest::GTest GTest::Main ${PROJECT_NAME})
    add_test(NAME reactionTest COMMAND runTests)
else()
    message(WARNING "GTest not found, skipping tests.")
endif()