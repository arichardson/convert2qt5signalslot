#include "testCommon.h"
#include <llvm/Support/raw_ostream.h>

/** from the Qt docs:

Disconnect everything connected to an object's signals:

    disconnect(myObject, 0, 0, 0);

equivalent to the non-static overloaded function

    myObject->disconnect();

Disconnect everything connected to a specific signal:

    disconnect(myObject, SIGNAL(mySignal()), 0, 0);

equivalent to the non-static overloaded function

    myObject->disconnect(SIGNAL(mySignal()));

Disconnect a specific receiver:

    disconnect(myObject, 0, myReceiver, 0);

equivalent to the non-static overloaded function

    myObject->disconnect(myReceiver);

 */

int main() {
    if (!codeCompiles(readFile(TEST_SOURCE_DIR "/input/disconnect.cpp"))
            || !codeCompiles(readFile(TEST_SOURCE_DIR "/refactored/disconnect.cpp"))) {
        llvm::errs() << "Error : code does not compile!";
        return 1;
    }
    return testMain(readFile(TEST_SOURCE_DIR "/input/disconnect.cpp"),
            readFile(TEST_SOURCE_DIR "/refactored/disconnect.cpp"), 11, 11);
}
