#include "testCommon.h"

static const char* input = R"delim(
#include "fake_qobject.h"

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* other;
};

Test::Test() {
    //this is calling the static method QObject::connect()
    connect(this, SIGNAL(objectNameChanged(QString)), other, SLOT(deleteLater()));
    //same thing, but use a connection type
    connect(this, SIGNAL(objectNameChanged(QString)), other, SLOT(deleteLater()), Qt::QueuedConnection);
    //here we are using the inline member version
    connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    //same thing, but use a connection type
    connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()), Qt::QueuedConnection);

    //handle line breaks
    connect(this,
        SIGNAL(objectNameChanged(QString)),
        SLOT(deleteLater())
    );
    //same thing, but use a connection type
    connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()),
        Qt::QueuedConnection);
}
)delim";

static const char* output = R"delim(
#include "fake_qobject.h"

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* other;
};

Test::Test() {
    //this is calling the static method QObject::connect()
    connect(this, &Test::objectNameChanged, other, &QObject::deleteLater);
    //same thing, but use a connection type
    connect(this, &Test::objectNameChanged, other, &QObject::deleteLater, Qt::QueuedConnection);
    //here we are using the inline member version
    connect(this, &Test::objectNameChanged, this, &Test::deleteLater);
    //same thing, but use a connection type
    connect(this, &Test::objectNameChanged, this, &Test::deleteLater, Qt::QueuedConnection);

    //handle line breaks
    connect(this,
        &Test::objectNameChanged,
        this, &Test::deleteLater
    );
    //same thing, but use a connection type
    connect(this, &Test::objectNameChanged, this, &Test::deleteLater,
        Qt::QueuedConnection);
}
)delim";

int main() {
    return testMain(input, output);
}

