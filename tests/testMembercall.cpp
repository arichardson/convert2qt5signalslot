#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/membercall.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/membercall.cpp"), 22, 22);
}
