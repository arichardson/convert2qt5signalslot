#include <QObject>
#include <QString>
#include <memory>

class TestPrivate {
public:
    void privateSlot() {
        fprintf(stderr, "privateSlot() called\n");
    }
};

class Test : public QObject {
    Q_OBJECT
public:
    Test() : d(new TestPrivate()) {
        connect(this, SIGNAL(sig()), SLOT(privateSlot()));
    }
    virtual ~Test();
Q_SIGNALS:
    void sig();
public: // so that it stays compilable, I guess in general this will happen inside the class so it can stay private
    QScopedPointer<TestPrivate> d;
private:
    Q_PRIVATE_SLOT(d, void privateSlot())
};

class Test2 : public QObject {
    Q_OBJECT
public:
    Test2();
    virtual ~Test2();
Q_SIGNALS:
    void sig2();
public:
    class Private;
    QScopedPointer<Private> d;
private:
    Q_PRIVATE_SLOT(d, void privateSlot2())
};

class Test2::Private {
public:
    void privateSlot2() {
        fprintf(stderr, "privateSlot2() called\n");
    }
};

Test2::Test2(): QObject(), d(new Private())
{
    connect(this, SIGNAL(sig2()), SLOT(privateSlot2()));
}

Test2::~Test2(){}
Test::~Test() {}

int main(int argc, char** argv) {
    Test* test = new Test();
    Test2* test2 = new Test2();
    QObject obj;
    QObject::connect(&obj, SIGNAL(objectNameChanged(QString)), test, SLOT(privateSlot()));
    QObject::connect(&obj, SIGNAL(objectNameChanged(QString)), test2, SLOT(privateSlot2()));
    obj.setObjectName("foo");
}

#include "q_private_slot_smart_ptr.moc"
