#include "testCommon.h"
#include "testDisconnect.h"

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
    return testMain(disconnectInput, disconnectOutputNullptr, 11, 11);
}
