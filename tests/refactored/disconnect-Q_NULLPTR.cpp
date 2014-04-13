#include <QObject>

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
    // TODO: contribute a patch to Qt to allow passing null as second argument
    QObject::disconnect(myObject, static_cast<void(QObject::*)()>(0), receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, static_cast<void(QObject::*)()>(Q_NULLPTR), receiver, &QObject::deleteLater); // same as this
    QObject::disconnect(myObject, static_cast<void(QObject::*)()>(0), receiver, &QObject::deleteLater); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater);
    QObject::disconnect(myObject, &QObject::objectNameChanged, receiver, &QObject::deleteLater); // same as this
    return 0;
}
