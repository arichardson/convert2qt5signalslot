project(convert2qt5signalslot)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake) 

find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

link_directories( ${LLVM_LIB_DIR} )
include_directories(${CLANG_INCLUDE_DIRS})

add_definitions("-std=c++11 -fno-rtti")

add_executable(convert2qt5signalslot Qt5SignalSlotSyntaxConverter.cpp)
target_link_libraries(convert2qt5signalslot ${CLANG_LIBS} ${LLVM_LIBS_CORE} pthread)
install(TARGETS convert2qt5signalslot RUNTIME DESTINATION bin)

enable_testing()

add_test(NAME simple
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
	COMMAND python ./convertertest.py "${CMAKE_BINARY_DIR}/convert2qt5signalslot" simple.cpp)

add_test(NAME overloaded
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
	COMMAND python ./convertertest.py "${CMAKE_BINARY_DIR}/convert2qt5signalslot" overloaded.cpp)

add_test(NAME namespaces
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
	COMMAND python ./convertertest.py "${CMAKE_BINARY_DIR}/convert2qt5signalslot" namespaces.cpp)
