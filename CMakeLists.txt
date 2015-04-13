cmake_minimum_required (VERSION 2.8.9)

#--------------------------------------------------------------------------------------------------#

project (netcode)

#--------------------------------------------------------------------------------------------------#

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if (NOT COMPILER_SUPPORTS_CXX11)
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif ()

#--------------------------------------------------------------------------------------------------#

find_package (Threads)

#--------------------------------------------------------------------------------------------------#

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
                    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating API documentation with Doxygen" VERBATIM
                    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                   )
endif()

#--------------------------------------------------------------------------------------------------#

find_path(GF_COMPLETE_INCLUDE_DIR gf_complete.h)
if (NOT GF_COMPLETE_INCLUDE_DIR)
  message(FATAL_ERROR "gf-complete headers not found")
endif ()

find_library(GF_COMPLETE_LIBRARY libgf_complete.a)
if (NOT GF_COMPLETE_LIBRARY)
  message(FATAL_ERROR "gf-complete library not found")
endif ()
message(STATUS "Found ${GF_COMPLETE_LIBRARY}")

#--------------------------------------------------------------------------------------------------#

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if (COVERAGE AND CMAKE_BUILD_TYPE MATCHES Debug AND CMAKE_COMPILER_IS_GNUCXX)
  include(CodeCoverage)
  SET(CMAKE_CXX_FLAGS_DEBUG "--coverage")
  SET(LCOV_EXCLUDE "'/boost/*'" "'/c++/*'")
  setup_target_for_coverage(coverage tests coverage)
endif()

#--------------------------------------------------------------------------------------------------#

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fvisibility=hidden")

#--------------------------------------------------------------------------------------------------#

include_directories("${GF_COMPLETE_INCLUDE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/ext")
include_directories("${PROJECT_SOURCE_DIR}/ext/asio")
add_subdirectory(accelerator)
add_subdirectory(examples)
add_subdirectory(netcode)
add_subdirectory(tests)
add_subdirectory(tools)

#--------------------------------------------------------------------------------------------------#
