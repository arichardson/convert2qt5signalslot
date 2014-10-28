project(convert2qt5signalslot)
cmake_minimum_required(VERSION 2.8.11)

include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake) 
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Clang 3.5 MODULE REQUIRED)
find_package(Qt5Core 5.3.0 CONFIG REQUIRED)

link_directories( ${LLVM_LIB_DIR} )
include_directories(${CLANG_INCLUDE_DIRS})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  message(STATUS "Compiling using clang++")
  add_definitions("-std=c++11 -fno-rtti -Wall -pedantic -Weverything -Wno-global-constructors -Wno-c++98-compat -Wno-exit-time-destructors")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "Compiling using g++")
  add_definitions("-std=c++11 -fno-rtti -Wall -pedantic")
else()
  message(FATAL_ERROR "Using unknown compiler, don't know how to activate C++11")
endif()

add_library(converter STATIC Qt5SignalSlotSyntaxConverter.cpp ClangUtils.cpp)
target_link_libraries(converter
  LLVMSupport
  LLVMCore
  LLVMObject
  LLVMTarget
  LLVMAnalysis
  LLVMipa
  LLVMTransformUtils
  LLVMOption
  Clang::Tooling
  Clang::AST
  Clang::ASTMatchers
  Clang::Basic
  Clang::Frontend
  Clang::Lex
  Clang::Sema
  Clang::Rewrite
  Clang::Analysis
  Clang::Serialization
  Clang::Parse
  Clang::Driver
  Clang::Edit

  Qt5::Core
)

add_executable(convert2qt5signalslot main.cpp)
target_link_libraries(convert2qt5signalslot converter)
install(TARGETS convert2qt5signalslot RUNTIME DESTINATION bin)

option(BUILD_TESTING "Whether to build the unit tests" OFF)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif(BUILD_TESTING)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
