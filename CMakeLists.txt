project(convert2qt5signalslot)
cmake_minimum_required(VERSION 2.8.11)

include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake) 
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Clang 3.5 MODULE REQUIRED)
find_package(Qt5Core 5.3.0 CONFIG REQUIRED)

# LLVM_DEFINITIONS are not added to the imported targets...
add_definitions(${LLVM_DEFINITIONS})
include_directories(${CLANG_INCLUDE_DIRS})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  message(STATUS "Compiling using clang++")
  add_compile_options(-std=c++11 -fno-rtti -Wall -pedantic -Weverything -Wno-global-constructors -Wno-c++98-compat -Wno-exit-time-destructors -Wno-shadow -Wno-padded)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "Compiling using g++")
  add_compile_options(-std=c++11 -fno-rtti -Wall -pedantic)
else()
  message(FATAL_ERROR "Using unknown compiler, don't know how to activate C++11")
endif()

option(USE_OLD_CXX11_ABI "Whether to build with GCC 5 old C++11 ABI. Might be needed depending on how the clang libs were built" OFF)
if(USE_OLD_CXX11_ABI)
  add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
endif()

#get path to clang internal include dir (while that is not fixed in clang)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

# we have to make sure these libraries are in the right order so that linking with static libs works
set(REQUIRED_CLANG_LIBS Clang::Tooling)
if(${Clang_VERSION} VERSION_GREATER 3.5.0)
  list(APPEND REQUIRED_CLANG_LIBS Clang::ToolingCore)
endif()
list(APPEND REQUIRED_CLANG_LIBS
  Clang::Rewrite
  Clang::ASTMatchers
  Clang::Frontend
  Clang::Parse
  Clang::Sema
  Clang::Analysis
  Clang::Serialization
  Clang::Driver
  Clang::Edit
  Clang::Lex
  Clang::AST
  Clang::Basic
  LLVMCodeGen
  LLVMObject
  LLVMOption
  LLVMSupport
)

add_library(converter STATIC Qt5SignalSlotSyntaxConverter.cpp ClangUtils.cpp PreProcessorCallback.cpp)
target_link_libraries(converter PUBLIC Qt5::Core ${REQUIRED_CLANG_LIBS})

add_executable(convert2qt5signalslot main.cpp)
target_link_libraries(convert2qt5signalslot converter)
install(TARGETS convert2qt5signalslot RUNTIME DESTINATION bin)

option(BUILD_TESTING "Whether to build the unit tests" OFF)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif(BUILD_TESTING)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
