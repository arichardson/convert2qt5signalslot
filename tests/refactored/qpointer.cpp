#include <QObject>
#include <QPointer>

int main() {
    QObject* obj = new QObject();
    QPointer<QObject> objQPointer(new QObject());
    // QPointer has an implicit conversion operator that is somehow not used with the new syntax
    // therefore the new syntax needs to explicitly call .data()
    QObject::connect(obj, &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::connect(obj, &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);

    QObject::disconnect(obj, &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), static_cast<void(QObject::*)()>(nullptr), obj, &QObject::deleteLater);
    QObject::disconnect(objQPointer.data(), static_cast<void(QObject::*)()>(nullptr), objQPointer.data(), &QObject::deleteLater);
    return 0;
}

#include "qpointer.moc"
