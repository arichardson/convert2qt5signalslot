#include "testCommon.h"


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
static const char* input = R"delim(
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
    QObject::disconnect(myObject, SIGNAL(objectNameChanged(QString)), 0, 0);
    myObject->disconnect(SIGNAL(objectNameChanged(QString))); // same as this
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), 0); // and this
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), 0, 0); // and this

    // disconnect everything from myObject connected to receiver
    QObject::disconnect(myObject, 0, receiver, 0);
    myObject->disconnect(receiver); // same as this
    myObject->disconnect(0, receiver); // and this
    myObject->disconnect(0, receiver, 0); // and this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, SLOT(deleteLater()));
    myObject->disconnect(receiver, SLOT(deleteLater())); // same as this
    myObject->disconnect(0, receiver, SLOT(deleteLater())); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater()));
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater())); // same as this
}
)delim";

//TODO: myObject->disconnect(myObject, &QObject::objectNameChanged, 0, 0) -> QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0)
// call that do not contain SIGNAL()/SLOT() don't have to be converted!
static const char* output = R"delim(
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

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
}
)delim";

int main() {
    return testMain(input, output, 9, 9);
}
