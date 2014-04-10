#include "testCommon.h"

#include <llvm/Support/CommandLine.h>

extern llvm::cl::opt<std::string> nullPtrString;

int main() {
    std::string disconnectInput = readFile(TEST_SOURCE_DIR "/input/disconnect.cpp");
    if (testMain(disconnectInput, readFile(TEST_SOURCE_DIR "/refactored/disconnect.cpp"), 11, 11) != 0)
        return 1;
    nullPtrString = "NULL";
    if (testMain(disconnectInput, readFile(TEST_SOURCE_DIR "/refactored/disconnect-NULL.cpp"), 11, 11) != 0)
        return 1;
    nullPtrString = "0";
    if (testMain(disconnectInput, readFile(TEST_SOURCE_DIR "/refactored/disconnect-0.cpp"), 11, 11) != 0)
        return 1;
    nullPtrString = "Q_NULLPTR";
    if (testMain(disconnectInput, readFile(TEST_SOURCE_DIR "/refactored/disconnect-Q_NULLPTR.cpp"), 11, 11) != 0)
        return 1;
    return 0;
}
