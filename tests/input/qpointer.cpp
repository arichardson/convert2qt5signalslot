#include <QObject>
#include <QPointer>

int main() {
    QObject* obj = new QObject();
    QPointer<QObject> objQPointer(new QObject());
    // QPointer has an implicit conversion operator that is somehow not used with the new syntax
    // therefore the new syntax needs to explicitly call .data()
    QObject::connect(obj, SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    QObject::connect(objQPointer, SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    QObject::connect(objQPointer, SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    objQPointer->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    objQPointer->connect(objQPointer, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));

    QObject::disconnect(obj, SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    QObject::disconnect(objQPointer, SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    QObject::disconnect(objQPointer, SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    objQPointer->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    objQPointer->disconnect(SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    objQPointer->disconnect(obj, SLOT(deleteLater()));
    objQPointer->disconnect(objQPointer, SLOT(deleteLater()));
    return 0;
}

#include "qpointer.moc"
