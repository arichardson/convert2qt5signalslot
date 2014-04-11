#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/scoping.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/scoping.cpp"), 23, 23);
}
