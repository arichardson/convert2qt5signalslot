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
    //here receiver is "this"
    connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    //ensure that the receiver is correctly set ("q")
    q->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    //now try with a function call
    someObj()->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
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
    //here receiver is "this"
    connect(this, &Test::objectNameChanged, this, &Test::deleteLater);
    //ensure that the receiver is correctly set ("q")
    q->connect(this, &Test::objectNameChanged, q, &QObject::deleteLater);
    //now try with a function call
    someObj()->connect(this, &Test::objectNameChanged, someObj(), &QObject::deleteLater);
}
)delim";

int main() {
    return testMain(input, output, 3, 3);
}

