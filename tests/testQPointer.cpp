#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/qpointer.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/qpointer.cpp"), 5, 5);
}
