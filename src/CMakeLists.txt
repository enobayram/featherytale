cmake_minimum_required (VERSION 2.8.11)
project(feedthebird)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

set ( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(Zopflify)
include(SvgToFrames)

find_package ( Eigen3 REQUIRED )
find_package ( Boost REQUIRED )
find_package ( ORX COMPONENTS Scroll REQUIRED)
find_package ( CURL 7.47 REQUIRED )
find_package ( OpenSSL REQUIRED )
find_package ( ZLIB REQUIRED )

IF(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions("-march=native")
  add_definitions("-Wall -Wno-unused-variable -Wno-unused-but-set-variable")
  add_definitions("-D__cplusplus=201103L")
  SET (CMAKE_CXX_COMPILER_ARG1 -std=c++11)
ELSE()
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
  add_definitions(-DEIGEN_DONT_ALIGN)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE .)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG .)
  file(COPY ${ORX_DLLS} DESTINATION .)
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

add_definitions(-DPC_BUILD=1 -DSANDBOX)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D__orxDEBUG__")
set(CMAKE_CXX_FLAGS_FASTBUILD "-O1 -g0 -DNDEBUG")

add_custom_command(OUTPUT .phony-build_description
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DescribeBuild.cmake)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    .
    ${CURL_INCLUDE_DIRS}
    ${ORX_SCROLL_INCLUDE_DIR}
    ${ORX_INCLUDE_DIR} 
    ${EIGEN3_INCLUDE_DIR} 
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

file (GLOB_RECURSE SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

configure_file(execute_in_sibling_folder.ini airhockey.ini COPYONLY)

add_executable(airhockey ${SRCS} .phony-build_description)

target_link_libraries (airhockey ${ORX_LIBRARY} ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})

zopflify_all(${CMAKE_CURRENT_SOURCE_DIR}/../data/image)
