cmake_minimum_required(VERSION 2.8)
project(textx)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(Version) 

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Catch2/single_include/catch2/catch.hpp)
  set(CATCH_INCLUDE_PATH Catch2/single_include/catch2/)
else()
  find_path(CATCH_INCLUDE_PATH catch.hpp REQUIRED)
endif()

message("CATCH_INCLUDE_PATH=${CATCH_INCLUDE_PATH}")
add_definitions(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

FIND_PACKAGE( Boost COMPONENTS regex REQUIRED )


include(CTest)
enable_testing()

file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE TST test/*.cpp)
file(GLOB EXAMPLES examples/*.cpp)

add_library(${PROJECT_NAME} ${SRC} ${version_file})
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} Boost::regex)

set(EXE_NAME "${PROJECT_NAME}.exe")
foreach(CPPFILE ${EXAMPLES})
    get_filename_component(EXE_NAME ${CPPFILE} NAME_WLE)
    add_executable(${EXE_NAME} ${CPPFILE})
    target_include_directories(${PROJECT_NAME} PRIVATE src)
    target_link_libraries(${EXE_NAME} ${PROJECT_NAME})
endforeach()

set(UNITTEST "${PROJECT_NAME}_unit_tests")
add_executable(${UNITTEST} ${TST})
target_include_directories(${UNITTEST} PRIVATE src)
target_include_directories(${UNITTEST} PRIVATE test)
target_include_directories(${UNITTEST} PRIVATE ${CATCH_INCLUDE_PATH})
target_link_libraries(${UNITTEST} ${PROJECT_NAME})

add_test(NAME unittest COMMAND ${UNITTEST})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
