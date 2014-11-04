find_package(Boost REQUIRED)
find_package(Qt5Quick 5.3.0 CONFIG REQUIRED)
find_package(Qt5Test 5.3.0 CONFIG REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

set(QT5_REQUIRED_COMPILER_FLAGS "")
foreach(_flag ${Qt5Quick_COMPILE_DEFINITIONS})
    set(QT5_REQUIRED_COMPILER_FLAGS "${QT5_REQUIRED_COMPILER_FLAGS} \"-D${_flag}\", ")
endforeach()
foreach(_flag ${Qt5Quick_INCLUDE_DIRS})
    set(QT5_REQUIRED_COMPILER_FLAGS "${QT5_REQUIRED_COMPILER_FLAGS} \"-isystem\", \"${_flag}\",")
endforeach()
foreach(_flag ${Qt5Quick_EXECUTABLE_COMPILE_FLAGS})
    set(QT5_REQUIRED_COMPILER_FLAGS "${QT5_REQUIRED_COMPILER_FLAGS} \"${_flag}\",")
endforeach()
configure_file(config-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-tests.h @ONLY)

add_library(testCommon STATIC testCommon.cpp) #only compile once, not for every test
target_link_libraries(testCommon converter)

function(converter_unit_test _name)
  add_executable(test-${_name} test${_name}.cpp)
  target_link_libraries(test-${_name} testCommon)
  add_test(NAME ${_name}-cxx COMMAND test-${_name})
endfunction()

set(CMAKE_AUTOMOC ON)

function(build_old_and_new _name)
  add_executable(${_name}-qt4style input/${_name}.cpp)
  target_link_libraries(${_name}-qt4style Qt5::Core)
  add_executable(${_name}-qt5style refactored/${_name}.cpp)
  target_link_libraries(${_name}-qt5style Qt5::Core)
endfunction()

function(simple_converter_unit_test _name _found_matches _converted_matches)
  set(FILE_NAME ${_name}.cpp)
  set(MATCHES_FOUND ${_found_matches})
  set(MATCHES_CONVERTED ${_converted_matches})
  configure_file(testMain.cpp.cmake ${CMAKE_CURRENT_BINARY_DIR}/test_${_name}.cpp @ONLY)
  add_executable(test-${_name} ${CMAKE_CURRENT_BINARY_DIR}/test_${_name}.cpp)
  build_old_and_new(${_name})
  target_link_libraries(test-${_name} testCommon)
  add_test(NAME ${_name}-cxx COMMAND test-${_name})
endfunction()

simple_converter_unit_test(simple 6 6)
simple_converter_unit_test(scoping 23 23)
simple_converter_unit_test(membercall 22 22)
simple_converter_unit_test(smart_pointer 36 36)
simple_converter_unit_test(using_directives 50 50)
simple_converter_unit_test(qquickitem 12 3)
simple_converter_unit_test(overloads 17 17)
simple_converter_unit_test(normalized_signatures 9 9)
simple_converter_unit_test(q_private_slot 10 10)
simple_converter_unit_test(q_private_slot_smart_ptr 4 4)
simple_converter_unit_test(template_class 3 3)
simple_converter_unit_test(enum_scoping 2 2)
add_test(NAME q_private_slot-qt4style COMMAND q_private_slot-qt4style) # make sure slots are called 10 times
add_test(NAME q_private_slot-qt5style COMMAND q_private_slot-qt5style) # make sure slots are called 10 times
add_test(NAME normalized_signatures-qt4style COMMAND normalized_signatures-qt4style)
add_test(NAME normalized_signatures-qt5style COMMAND normalized_signatures-qt5style)

simple_converter_unit_test(disconnect 11 11) # This one does not work, there is no signal = nullptr overload for QObject::disconnect

converter_unit_test(MissingMocInclude)
converter_unit_test(NullptrCustomization)

target_link_libraries(qquickitem-qt5style Qt5::Quick)
target_link_libraries(qquickitem-qt4style Qt5::Quick)
