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
    // TODO: contribute a patch to Qt to allow passing null as second argument
    QObject::disconnect(myObject, 0, receiver, SLOT(deleteLater()));
    myObject->disconnect(receiver, SLOT(deleteLater())); // same as this
    myObject->disconnect(0, receiver, SLOT(deleteLater())); // and this

    // disconnect everything from myObject::objectNameChanged connected to receiver::deleteLater
    QObject::disconnect(myObject, SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater()));
    myObject->disconnect(SIGNAL(objectNameChanged(QString)), receiver, SLOT(deleteLater())); // same as this
    return 0;
}
