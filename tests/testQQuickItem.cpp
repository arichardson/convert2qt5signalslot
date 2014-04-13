#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/qquickitem.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/qquickitem.cpp"), 12, 3);
}
