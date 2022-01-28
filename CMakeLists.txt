cmake_minimum_required(VERSION 2.8)
project(textx)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Note on ASAN/UBSAN
# I had problems with coroutines... however with valgrind it seems ok.
# --> to be observed!
#
add_compile_options(
  "-fcoroutines"
#  "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>"
#  "$<$<CONFIG:DEBUG>:-fsanitize=address>"
#  "$<$<CONFIG:DEBUG>:-fsanitize=undefined>"
)
add_link_options(
#  "$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>"
#  "$<$<CONFIG:DEBUG>:-fsanitize=address>"
#  "$<$<CONFIG:DEBUG>:-fsanitize=undefined>"
)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

include(Version) 

find_path(CATCH_INCLUDE_PATH catch.hpp REQUIRED)

message("CATCH_INCLUDE_PATH=${CATCH_INCLUDE_PATH}")
add_definitions(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

FIND_PACKAGE( Boost COMPONENTS regex REQUIRED )


include(CTest)
enable_testing()

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist "${curdir}/${child}")
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE TST test/*.cpp)
file(GLOB EXAMPLES examples/*.cpp)
SUBDIRLIST(EXAMPLE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/examples")

add_library(${PROJECT_NAME} ${SRC} ${version_file})
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
target_link_libraries(${PROJECT_NAME} Boost::regex)

set(EXE_NAME "${PROJECT_NAME}.exe")
foreach(CPPFILE ${EXAMPLES})
    get_filename_component(EXE_NAME ${CPPFILE} NAME_WLE)
    add_executable(${EXE_NAME} ${CPPFILE})
    target_include_directories(${EXE_NAME} PRIVATE src)
    target_include_directories(${EXE_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
    target_link_libraries(${EXE_NAME} ${PROJECT_NAME})
endforeach()

#message("EXAMPLE_DIRS=${EXAMPLE_DIRS}")
foreach(CPPDIR ${EXAMPLE_DIRS})
    get_filename_component(EXE_NAME ${CPPDIR} NAME_WLE)
    #MESSAGE("creating ${EXE_NAME} from ${CPPDIR}")
    file(GLOB_RECURSE EXESRC ${CPPDIR}/src/*.cpp)    
    add_executable(${EXE_NAME} ${EXESRC})
    target_include_directories(${EXE_NAME} PRIVATE src)
    target_include_directories(${EXE_NAME} PRIVATE ${CPPDIR}/src)
    target_include_directories(${EXE_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
    target_link_libraries(${EXE_NAME} ${PROJECT_NAME})
endforeach()

set(UNITTEST "${PROJECT_NAME}_unit_tests")
add_executable(${UNITTEST} ${TST})
target_include_directories(${UNITTEST} PRIVATE src)
target_include_directories(${UNITTEST} PRIVATE test)
target_include_directories(${UNITTEST} PRIVATE ${CATCH_INCLUDE_PATH})
target_include_directories(${UNITTEST} PRIVATE ${CPPCORO_INCLUDE_PATH})
target_link_libraries(${UNITTEST} ${PROJECT_NAME})

add_test(NAME unittest COMMAND ${UNITTEST})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
