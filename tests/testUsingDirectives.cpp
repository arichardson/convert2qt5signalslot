#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/using_directives.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/using_directives.cpp"), 24, 24);
}
