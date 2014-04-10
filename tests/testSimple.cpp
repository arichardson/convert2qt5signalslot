#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/simple.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/simple.cpp"), 6, 6);
}
