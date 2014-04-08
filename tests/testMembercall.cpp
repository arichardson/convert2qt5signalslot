#include "testCommon.h"

static const char* input = R"delim(
#include <qobjectdefs.h>

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
    connect(q, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    disconnect(SIGNAL(objectNameChanged(QString)), q, SLOT(deleteLater()));
    this->connect(q, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    this->disconnect(SIGNAL(objectNameChanged(QString)), q, SLOT(deleteLater()));
    //ensure that the connect receiver/disconnect sender is correctly set ("q")
    q->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    q->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));
    //now try with a function call
    someObj()->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    someObj()->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject value;
    value.connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    value.disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject& ref = *q;
    ref.connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    ref.disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject*& ptrRef = q;
    ptrRef->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    ptrRef->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject** ptrPtr = &q;
    (*ptrPtr)->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    (*ptrPtr)->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    // dereferenced pointer
    (*q).connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    (*q).disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));
}

void nonMember() {
    Test* test = nullptr;
    QObject* obj = nullptr;
    // now we need the QObject:: added
    test->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    obj->connect(test, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    test->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    obj->disconnect(SIGNAL(objectNameChanged(QString)), test, SLOT(deleteLater()));
}
)delim";

static const char* output = R"delim(
#include <qobjectdefs.h>

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

void nonMember() {
    Test* test = nullptr;
    QObject* obj = nullptr;
    // now we need the QObject:: added
    QObject::connect(obj, &QObject::objectNameChanged, test, &Test::deleteLater);
    QObject::connect(test, &Test::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(test, &Test::objectNameChanged, obj, &QObject::deleteLater);
    QObject::disconnect(obj, &QObject::objectNameChanged, test, &Test::deleteLater);
}
)delim";

int main() {
    return testMain(input, output, 22, 22);
}

