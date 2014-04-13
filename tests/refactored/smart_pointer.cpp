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

    // QScopedPointer member calls
    QScopedPointer<QObject> qScopedPtr(new QObject());
    QObject::connect(obj, &QObject::objectNameChanged, qScopedPtr.data(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, qScopedPtr.data(), &QObject::deleteLater);
    QObject::disconnect(qScopedPtr.data(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(qScopedPtr.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(qScopedPtr.data(), static_cast<void(QObject::*)()>(nullptr), obj, &QObject::deleteLater);
    QObject::disconnect(qScopedPtr.data(), static_cast<void(QObject::*)()>(nullptr), objQPointer.data(), &QObject::deleteLater);

    // QSharedPointer member calls
    QSharedPointer<QObject> qSharedPtr(new QObject());
    QObject::connect(obj, &QObject::objectNameChanged, qSharedPtr.data(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, qSharedPtr.data(), &QObject::deleteLater);
    QObject::disconnect(qSharedPtr.data(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(qSharedPtr.data(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(qSharedPtr.data(), static_cast<void(QObject::*)()>(nullptr), obj, &QObject::deleteLater);
    QObject::disconnect(qSharedPtr.data(), static_cast<void(QObject::*)()>(nullptr), objQPointer.data(), &QObject::deleteLater);

    // std::unique_ptr member calls
    std::unique_ptr<QObject> stdUniquePtr(new QObject());
    QObject::connect(obj, &QObject::objectNameChanged, stdUniquePtr.get(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, stdUniquePtr.get(), &QObject::deleteLater);
    QObject::disconnect(stdUniquePtr.get(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(stdUniquePtr.get(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(stdUniquePtr.get(), static_cast<void(QObject::*)()>(nullptr), obj, &QObject::deleteLater);
    QObject::disconnect(stdUniquePtr.get(), static_cast<void(QObject::*)()>(nullptr), objQPointer.data(), &QObject::deleteLater);

    // std::shared_ptr member calls
    std::shared_ptr<QObject> stdSharedPtr = std::make_shared<QObject>();
    QObject::connect(obj, &QObject::objectNameChanged, stdSharedPtr.get(), &QObject::deleteLater);
    QObject::connect(objQPointer.data(), &QObject::objectNameChanged, stdSharedPtr.get(), &QObject::deleteLater);
    QObject::disconnect(stdSharedPtr.get(), &QObject::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(stdSharedPtr.get(), &QObject::objectNameChanged, objQPointer.data(), &QObject::deleteLater);
    QObject::disconnect(stdSharedPtr.get(), static_cast<void(QObject::*)()>(nullptr), obj, &QObject::deleteLater);
    QObject::disconnect(stdSharedPtr.get(), static_cast<void(QObject::*)()>(nullptr), objQPointer.data(), &QObject::deleteLater);
    return 0;
}

#include "smart_pointer.moc"
