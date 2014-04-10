#include "testCommon.h"

//TODO: are virtual signals okay? If so test that here as well

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/overloads.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/overloads.cpp"), 17, 17);
}

