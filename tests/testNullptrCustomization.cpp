#include "testCommon.h"
#include "testDisconnect.h"

#include <llvm/Support/CommandLine.h>


static const char* disconnectOutputNULL = R"delim(
#include <qobjectdefs.h>

int main() {
    QObject* myObject = new QObject();
    QObject* receiver = new QObject();

    // disconnect everything from myObject
    QObject::disconnect(myObject, 0, 0, 0);
    myObject->disconnect(); // same as this
    // myObject->disconnect(0); // ambiguous!
    // myObject->disconnect(0, 0); // ambiguous!
    myObject->disconnect(0, 0, 0); // and this

    // disconnect everything from myObject::objectNameChanged
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, NULL, NULL); // same as this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, NULL); // and this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // and this

    // disconnect everything from myObject connected to receiver
    QObject::disconnect(myObject, 0, receiver, 0);
    myObject->disconnect(receiver); // same as this
    myObject->disconnect(0, receiver); // and this
    myObject->disconnect(0, receiver, 0); // and this

    //disconnect everything from myObject::objectNameChanged connected to receiver
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, NULL); // same as this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, NULL, receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
}
)delim";

static const char* disconnectOutput0 = R"delim(
#include <qobjectdefs.h>

int main() {
    QObject* myObject = new QObject();
    QObject* receiver = new QObject();

    // disconnect everything from myObject
    QObject::disconnect(myObject, 0, 0, 0);
    myObject->disconnect(); // same as this
    // myObject->disconnect(0); // ambiguous!
    // myObject->disconnect(0, 0); // ambiguous!
    myObject->disconnect(0, 0, 0); // and this

    // disconnect everything from myObject::objectNameChanged
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // same as this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // and this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // and this

    // disconnect everything from myObject connected to receiver
    QObject::disconnect(myObject, 0, receiver, 0);
    myObject->disconnect(receiver); // same as this
    myObject->disconnect(0, receiver); // and this
    myObject->disconnect(0, receiver, 0); // and this

    //disconnect everything from myObject::objectNameChanged connected to receiver
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, 0); // same as this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
}
)delim";

static const char* disconnectOutputQ_NULLPTR = R"delim(
#include <qobjectdefs.h>

int main() {
    QObject* myObject = new QObject();
    QObject* receiver = new QObject();

    // disconnect everything from myObject
    QObject::disconnect(myObject, 0, 0, 0);
    myObject->disconnect(); // same as this
    // myObject->disconnect(0); // ambiguous!
    // myObject->disconnect(0, 0); // ambiguous!
    myObject->disconnect(0, 0, 0); // and this

    // disconnect everything from myObject::objectNameChanged
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, Q_NULLPTR, Q_NULLPTR); // same as this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, Q_NULLPTR); // and this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // and this

    // disconnect everything from myObject connected to receiver
    QObject::disconnect(myObject, 0, receiver, 0);
    myObject->disconnect(receiver); // same as this
    myObject->disconnect(0, receiver); // and this
    myObject->disconnect(0, receiver, 0); // and this

    //disconnect everything from myObject::objectNameChanged connected to receiver
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, Q_NULLPTR); // same as this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, Q_NULLPTR, receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
}
)delim";

extern llvm::cl::opt<std::string> nullPtrString;

int main() {
    if (testMain(disconnectInput, disconnectOutputNullptr, 11, 11) != 0)
        return 1;
    nullPtrString = "NULL";
    if (testMain(disconnectInput, disconnectOutputNULL, 11, 11) != 0)
        return 1;
    nullPtrString = "0";
    if (testMain(disconnectInput, disconnectOutput0, 11, 11) != 0)
        return 1;
    nullPtrString = "Q_NULLPTR";
    if (testMain(disconnectInput, disconnectOutputQ_NULLPTR, 11, 11) != 0)
        return 1;
    return 0;
}
