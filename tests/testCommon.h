#pragma once

#include <string>

#define BASE_DIR "/the/path/to/be/refactored/"
#define FILE_NAME BASE_DIR "input.cpp"
#define INCLUDE_DIR "/fake/system/include/path/"
#define FAKE_QOBJECT_H_NAME INCLUDE_DIR "qobjectdefs.h"

int testMain(std::string input, std::string expected, int found, int converted);

