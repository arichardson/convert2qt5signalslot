find_package(Boost)
find_package(Qt5Core CONFIG REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(TEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
configure_file(config-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-tests.h @ONLY)

add_library(testCommonObjs OBJECT testCommon.cpp) #only compile once, not for every test

function(converter_unit_test _name)
  add_executable(test-${_name} test${_name}.cpp $<TARGET_OBJECTS:testCommonObjs> $<TARGET_OBJECTS:converterObjs>)
  target_link_libraries(test-${_name} ${CLANG_LIBS} ${LLVM_LIBS_CORE} pthread)
  add_test(NAME ${_name}-cxx COMMAND test-${_name})
endfunction()

converter_unit_test(Simple)
converter_unit_test(Overloads)
converter_unit_test(Namespaces)
converter_unit_test(Membercall)
converter_unit_test(Disconnect)
converter_unit_test(MissingMocInclude)
converter_unit_test(NullptrCustomization)


set(CMAKE_AUTOMOC ON)

function(build_old_and_new _name)
	add_executable(${_name}-qt4style input/${_name}.cpp)
	target_link_libraries(${_name}-qt4style Qt5::Core)
	add_executable(${_name}-qt5style refactored/${_name}.cpp)
	target_link_libraries(${_name}-qt5style Qt5::Core)
endfunction()

build_old_and_new(simple)
build_old_and_new(membercall)
build_old_and_new(namespaces)
build_old_and_new(overloads)
# This one does not work, there is no signal = nullptr overload for QObject::disconnect
#build_old_and_new(disconnect)
