add_executable(test-simple testSimple.cpp testCommon.cpp ../Qt5SignalSlotSyntaxConverter.cpp)
target_link_libraries(test-simple ${CLANG_LIBS} ${LLVM_LIBS_CORE} pthread)
add_test(NAME simple-cxx COMMAND test-simple)