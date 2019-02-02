cmake_minimum_required (VERSION 2.8.9)

#--------------------------------------------------------------------------------------------------#

project (netcode)

#--------------------------------------------------------------------------------------------------#

enable_testing()

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
  if (INTERNAL_DOC)
    set(GEN_INTERNAL_DOC "YES")
  else ()
    set(GEN_INTERNAL_DOC "NO")
  endif ()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY
                )
  add_custom_target(doc
                    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating API documentation with Doxygen" VERBATIM
                    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/Doxyfile.in
                   )
endif()

#--------------------------------------------------------------------------------------------------#

option (GF_COMPLETE_ROOT "Path to gf-complete")

find_path(GF_COMPLETE_INCLUDE_DIR gf_complete.h PATHS "${GF_COMPLETE_ROOT}/include")
if (NOT GF_COMPLETE_INCLUDE_DIR)
  message(FATAL_ERROR "gf-complete headers not found")
endif ()

find_library(GF_COMPLETE_LIBRARY libgf_complete.a PATHS "${GF_COMPLETE_ROOT}/lib")
if (NOT GF_COMPLETE_LIBRARY)
  message(FATAL_ERROR "gf-complete library not found")
endif ()
message(STATUS "Found ${GF_COMPLETE_LIBRARY}")

#--------------------------------------------------------------------------------------------------#

find_package(BOOST 1.68 REQUIRED)

#--------------------------------------------------------------------------------------------------#

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
if (COVERAGE AND CMAKE_BUILD_TYPE MATCHES Debug AND CMAKE_COMPILER_IS_GNUCXX)
  include(CodeCoverage)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
  set(LCOV_EXCLUDE "'/boost/*'" "'/c++/*'")
  setup_target_for_coverage(coverage tests coverage)
endif()

#--------------------------------------------------------------------------------------------------#

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif ()

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wconversion ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-fvisibility=hidden -fvisibility-inlines-hidden ${CMAKE_CXX_FLAGS_RELEASE}")

#--------------------------------------------------------------------------------------------------#

# Workaround for CMake removing isystem flag on Mac OS X with GCC
# See https://public.kitware.com/Bug/view.php?id=10837
set(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
set(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

include_directories(SYSTEM "${GF_COMPLETE_INCLUDE_DIR}")
include_directories(SYSTEM "${Boost_INCLUDE_DIR}")
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/ext")
include_directories(SYSTEM "${PROJECT_SOURCE_DIR}/ext/asio")
include_directories("${PROJECT_SOURCE_DIR}")

#--------------------------------------------------------------------------------------------------#

add_subdirectory(doc/examples)
add_subdirectory(examples)
add_subdirectory(netcode)
add_subdirectory(tests)
add_subdirectory(tools)

#--------------------------------------------------------------------------------------------------#
