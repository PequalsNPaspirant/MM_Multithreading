cmake_minimum_required(VERSION 3.3 FATAL_ERROR)
cmake_policy(SET CMP0091 NEW)

set(PROJECT_NAME MM_Multithreading)
project(${PROJECT_NAME})

message("building project ${PROJECT_NAME}")
message("CMAKE_GENERATOR: ${CMAKE_GENERATOR}")

#set(Boost_USE_STATIC_LIBS OFF)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_USE_STATIC_RUNTIME OFF)

#add_definitions(-DBOOST_ALL_DYN_LINK)
#add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

#find_package(Boost 1.6 COMPONENTS date_time filesystem iostreams REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)

# Compilation Dependencies:
#include_directories(${Boost_INCLUDE_DIRS})
#link_libraries(${Boost_LIBRARIES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/mpir_forked/lib/x64/Release)

# Link Dependencies:
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/mpir_forked/lib/x64/Release)
#link_libraries(mpir.lib)
#link_libraries(mpirxx.lib)

#Microsoft Z3 Solver
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/microsoft-z3-solver/src/api)
#link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/microsoft-z3-solver/build)
#link_libraries(libz3.lib)

# Compile flags:
IF(WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -NODEFAULTLIB:LIBCMTD /ignore:4099")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /EHa /Zm2000")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -fexceptions -fnon-call-exceptions -Wall -g -fdiagnostics-color=always")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -fPIC")
ENDIF(WIN32)

# Build Targets:
file(GLOB_RECURSE SRC
     "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/../src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SRC})
#add_library(${PROJECT_NAME} ${SRC})

set_property(TARGET ${PROJECT_NAME} PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  
#set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
#set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_MESSAGES ON)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)


