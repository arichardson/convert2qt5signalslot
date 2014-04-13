#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QScopedPointer>
#include <memory>

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

    // QScopedPointer member calls
    QScopedPointer<QObject> qScopedPtr(new QObject());
    qScopedPtr->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    qScopedPtr->connect(objQPointer, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    qScopedPtr->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    qScopedPtr->disconnect(SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    qScopedPtr->disconnect(obj, SLOT(deleteLater()));
    qScopedPtr->disconnect(objQPointer, SLOT(deleteLater()));

    // QSharedPointer member calls
    QSharedPointer<QObject> qSharedPtr(new QObject());
    qSharedPtr->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    qSharedPtr->connect(objQPointer, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    qSharedPtr->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    qSharedPtr->disconnect(SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    qSharedPtr->disconnect(obj, SLOT(deleteLater()));
    qSharedPtr->disconnect(objQPointer, SLOT(deleteLater()));

    // std::unique_ptr member calls
    std::unique_ptr<QObject> stdUniquePtr(new QObject());
    stdUniquePtr->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    stdUniquePtr->connect(objQPointer, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    stdUniquePtr->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    stdUniquePtr->disconnect(SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    stdUniquePtr->disconnect(obj, SLOT(deleteLater()));
    stdUniquePtr->disconnect(objQPointer, SLOT(deleteLater()));

    // std::shared_ptr member calls
    std::shared_ptr<QObject> stdSharedPtr = std::make_shared<QObject>();
    stdSharedPtr->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    stdSharedPtr->connect(objQPointer, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    stdSharedPtr->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    stdSharedPtr->disconnect(SIGNAL(objectNameChanged(QString)), objQPointer, SLOT(deleteLater()));
    stdSharedPtr->disconnect(obj, SLOT(deleteLater()));
    stdSharedPtr->disconnect(objQPointer, SLOT(deleteLater()));
    return 0;
}

#include "smart_pointer.moc"
