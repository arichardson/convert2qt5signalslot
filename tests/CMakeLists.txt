add_library(testCommonObjs OBJECT testCommon.cpp) #only compile once, not for every test

macro(converter_unit_test _name)
  add_executable(test-${_name} test${_name}.cpp $<TARGET_OBJECTS:testCommonObjs> $<TARGET_OBJECTS:converterObjs>)
  target_link_libraries(test-${_name} ${CLANG_LIBS} ${LLVM_LIBS_CORE} pthread)
  add_test(NAME ${_name}-cxx COMMAND test-${_name})
endmacro()

converter_unit_test(Simple)
converter_unit_test(Overloads)
converter_unit_test(Namespaces)
converter_unit_test(Membercall)
