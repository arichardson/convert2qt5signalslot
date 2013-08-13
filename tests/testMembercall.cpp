#include "testCommon.h"

static const char* input = R"delim(
#include <QtCore/QObject>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj();
    QObject* q;
};

Test::Test() {
    //here receiver is "this"
    connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    //ensure that the receiver is correctly set ("q")
    q->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    //no try with a function call
    someObj()->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
}
)delim";

static const char* output = R"delim(
#include <QtCore/QObject>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj();
    QObject* q;
};

Test::Test() {
    //here receiver is "this"
    connect(this, &Test::objectNameChanged, this, &Test::deleteLater);
    //ensure that the receiver is correctly set ("q")
    q->connect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    //no try with a function call
    someObj()->connect(this, &Test::objectNameChanged, someObj(), &QObject::deleteLater);
}
)delim";

int main() {
    return testMain(input, output);
}

