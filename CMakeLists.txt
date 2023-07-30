cmake_minimum_required(VERSION 2.8)
project(textx)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

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

find_path(CATCH_INCLUDE_PATH catch.hpp PATHS /usr/include/catch2 REQUIRED)

message("CATCH_INCLUDE_PATH=${CATCH_INCLUDE_PATH}")
#add_definitions(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

FIND_PACKAGE( Boost COMPONENTS regex program_options REQUIRED )


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
    MESSAGE(status "creating ${EXE_NAME} from ${CPPDIR}")

    set(LIB_NAME lib${EXE_NAME})
    file(GLOB_RECURSE LIBSRC ${CPPDIR}/src/*.cpp)    
    add_library(${LIB_NAME} ${LIBSRC})
    target_include_directories(${LIB_NAME} PRIVATE src)
    target_include_directories(${LIB_NAME} PRIVATE ${CPPDIR}/src)
    target_include_directories(${LIB_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
    target_link_libraries(${LIB_NAME} ${PROJECT_NAME})

    add_executable(${EXE_NAME} ${CPPDIR}/main.cpp)
    target_include_directories(${EXE_NAME} PRIVATE src)
    target_include_directories(${EXE_NAME} PRIVATE ${CPPDIR}/src)
    target_include_directories(${EXE_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
    target_link_libraries(${EXE_NAME} ${LIB_NAME} ${PROJECT_NAME} Boost::program_options)

    set(TEST_EXE_NAME ${EXE_NAME}_unit_tests)
    file(GLOB_RECURSE TEST_EXESRC ${CPPDIR}/test/*.cpp)    
    add_executable(${TEST_EXE_NAME} ${TEST_EXESRC})
    target_include_directories(${TEST_EXE_NAME} PRIVATE src ${CATCH_INCLUDE_PATH})
    target_include_directories(${TEST_EXE_NAME} PRIVATE ${CPPDIR}/src)
    target_include_directories(${TEST_EXE_NAME} PRIVATE ${CPPDIR}/test)
    target_include_directories(${TEST_EXE_NAME} PRIVATE ${CPPCORO_INCLUDE_PATH})
    target_link_libraries(${TEST_EXE_NAME} ${LIB_NAME} ${PROJECT_NAME})

    install(TARGETS ${EXE_NAME} RUNTIME DESTINATION bin)
    
endforeach()

set(UNITTEST "${PROJECT_NAME}_unit_tests")
add_executable(${UNITTEST} ${TST})
target_include_directories(${UNITTEST} PRIVATE src)
target_include_directories(${UNITTEST} PRIVATE test)
target_include_directories(${UNITTEST} PRIVATE ${CATCH_INCLUDE_PATH})
target_include_directories(${UNITTEST} PRIVATE ${CPPCORO_INCLUDE_PATH})
target_link_libraries(${UNITTEST} ${PROJECT_NAME})

add_test(NAME unittest COMMAND ${UNITTEST})

set(CPACK_PACKAGE_VERSION ${VERSION_SHORT})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "goto40")
set(CPACK_PACKAGE_VENDOR "")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
