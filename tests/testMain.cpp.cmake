#include "testCommon.h"

int main() {
    return testMain(readFile(TEST_SOURCE_DIR "/input/@FILE_NAME@"),
            readFile(TEST_SOURCE_DIR "/refactored/@FILE_NAME@"), @MATCHES_FOUND@, @MATCHES_CONVERTED@);
}
