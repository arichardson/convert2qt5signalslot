#include <QObject>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj();
    QObject* q;
};

Test::Test() {
    // inside QObject subclass -> don't have to prefix with QObject::

    // here connect receiver/disconnect sender is "this"
    connect(q, &QObject::objectNameChanged, this, &Test::deleteLater);
    disconnect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    connect(q, &QObject::objectNameChanged, this, &Test::deleteLater);
    disconnect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    //ensure that the connect receiver/disconnect sender is correctly set ("q")
    connect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    disconnect(q, &QObject::objectNameChanged, this, &Test::deleteLater);
    //now try with a function call
    connect(this, &Test::objectNameChanged, someObj(), &QObject::deleteLater);
    disconnect(someObj(), &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject value;
    connect(this, &Test::objectNameChanged, &value, &QObject::deleteLater);
    disconnect(&value, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject& ref = *q;
    connect(this, &Test::objectNameChanged, &ref, &QObject::deleteLater);
    disconnect(&ref, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject*& ptrRef = q;
    connect(this, &Test::objectNameChanged, ptrRef, &QObject::deleteLater);
    disconnect(ptrRef, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject** ptrPtr = &q;
    connect(this, &Test::objectNameChanged, (*ptrPtr), &QObject::deleteLater);
    disconnect((*ptrPtr), &QObject::objectNameChanged, this, &Test::deleteLater);

    // dereferenced pointer
    connect(this, &Test::objectNameChanged, &(*q), &QObject::deleteLater);
    disconnect(&(*q), &QObject::objectNameChanged, this, &Test::deleteLater);
}

int main() {
    Test* test = nullptr;
    QObject* obj = nullptr;
    // now we need the QObject:: added
    QObject::connect(obj, &QObject::objectNameChanged, test, &Test::deleteLater);
    QObject::connect(test, &Test::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(test, &Test::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(obj, &QObject::objectNameChanged, test, &Test::deleteLater);
    return 0;
}
