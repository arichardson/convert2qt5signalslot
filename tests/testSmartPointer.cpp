#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/smart_pointer.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/smart_pointer.cpp"), 36, 36);
}
