project(convert2qt5signalslot)
cmake_minimum_required(VERSION 2.8.11)

include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake) 
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(LLVM MODULE REQUIRED 3.4)
find_package(Clang MODULE REQUIRED 3.4)

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

execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs support
    OUTPUT_VARIABLE REQUIRED_LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(REQUIRED_CLANG_LIBS
	${CLANG_CLANGBASIC_LIB}
	${CLANG_CLANGFRONTEND_LIB}
	${CLANG_CLANGASTMATCHERS_LIB}
	${CLANG_CLANGLEX_LIB}
	${CLANG_CLANGSEMA_LIB}
	${CLANG_CLANGAST_LIB}
	${CLANG_CLANGTOOLING_LIB}
)

add_definitions(${LLVM_CFLAGS})

add_library(converterObjs OBJECT Qt5SignalSlotSyntaxConverter.cpp ClangUtils.cpp)

add_executable(convert2qt5signalslot $<TARGET_OBJECTS:converterObjs> main.cpp)
target_link_libraries(convert2qt5signalslot ${REQUIRED_CLANG_LIBS} ${REQUIRED_LLVM_LIBS})
install(TARGETS convert2qt5signalslot RUNTIME DESTINATION bin)

option(BUILD_TESTING "Whether to build the unit tests" OFF)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif(BUILD_TESTING)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
