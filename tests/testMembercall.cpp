#include "testCommon.h"

static const char* input = R"delim(
#include "fake_qobject.h"

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj();
    QObject* q;
};

Test::Test() {
    //here connect receiver/disconnect sender is "this"
    connect(q, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    disconnect(SIGNAL(objectNameChanged(QString)), q, SLOT(deleteLater()));
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
)delim";

static const char* output = R"delim(
#include "fake_qobject.h"

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj();
    QObject* q;
};

Test::Test() {
    //here connect receiver/disconnect sender is "this"
    connect(q, &QObject::objectNameChanged, this, &Test::deleteLater);
    disconnect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    //ensure that the connect receiver/disconnect sender is correctly set ("q")
    q->connect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    q->disconnect(q, &QObject::objectNameChanged, this, &Test::deleteLater);
    //now try with a function call
    someObj()->connect(this, &Test::objectNameChanged, someObj(), &QObject::deleteLater);
    someObj()->disconnect(someObj(), &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject value;
    value.connect(this, &Test::objectNameChanged, &value, &QObject::deleteLater);
    value.disconnect(&value, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject& ref = *q;
    ref.connect(this, &Test::objectNameChanged, &ref, &QObject::deleteLater);
    ref.disconnect(&ref, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject*& ptrRef = q;
    ptrRef->connect(this, &Test::objectNameChanged, ptrRef, &QObject::deleteLater);
    ptrRef->disconnect(ptrRef, &QObject::objectNameChanged, this, &Test::deleteLater);

    QObject** ptrPtr = &q;
    (*ptrPtr)->connect(this, &Test::objectNameChanged, (*ptrPtr), &QObject::deleteLater);
    (*ptrPtr)->disconnect((*ptrPtr), &QObject::objectNameChanged, this, &Test::deleteLater);

    // dereferenced pointer
    (*q).connect(this, &Test::objectNameChanged, &(*q), &QObject::deleteLater);
    (*q).disconnect(&(*q), &QObject::objectNameChanged, this, &Test::deleteLater);
}
)delim";

int main() {
    return testMain(input, output, 16, 16);
}

