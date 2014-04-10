#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/namespaces.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/namespaces.cpp"), 13, 13);
}
