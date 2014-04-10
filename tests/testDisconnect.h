#pragma once

static const char* disconnectInput = R"delim(
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

    //disconnect everything from myObject::objectNameChanged connected to receiver
    QObject::disconnect(myObject, SIGNAL(objectNameChanged(QString)), receiver, 0);
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), receiver); // same as this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, SLOT(deleteLater()));
    myObject->disconnect(receiver, SLOT(deleteLater())); // same as this
    myObject->disconnect(0, receiver, SLOT(deleteLater())); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater()));
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater())); // same as this
}
)delim";

// calls that do not contain SIGNAL()/SLOT() don't have to be converted!
static const char* disconnectOutputNullptr = R"delim(
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
    QObject::disconnect(myObject, &QObject::objectNameChanged, nullptr, nullptr); // same as this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, nullptr); // and this
    QObject::disconnect(myObject, &QObject::objectNameChanged, 0, 0); // and this

    // disconnect everything from myObject connected to receiver
    QObject::disconnect(myObject, 0, receiver, 0);
    myObject->disconnect(receiver); // same as this
    myObject->disconnect(0, receiver); // and this
    myObject->disconnect(0, receiver, 0); // and this

    //disconnect everything from myObject::objectNameChanged connected to receiver
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, 0);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, nullptr); // same as this

    // disconnect everything from myObject connected to receiver::deleteLater
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, nullptr, receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
}
)delim";
