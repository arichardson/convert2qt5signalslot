#include <QObject>
#include <QString>

class TestPrivate {
public:
    static int privateCount;
    void privateSlot() {
        privateCount++;
        fprintf(stderr, "privateSlot() called. Count = %d\n", privateCount);
    }
    void privateSlotOverloaded(int i) {
        privateCount++;
        fprintf(stderr, "privateSlotOverloaded(%d) called. Count = %d\n", i, privateCount);
    }
    void privateSlotOverloaded(const char* str) {
        privateCount++;
        fprintf(stderr, "privateSlotOverloaded(%s) called. Count = %d\n", str, privateCount);
    }
    void privateSlotOverloaded(const char* str, double d) {
        privateCount++;
        fprintf(stderr, "privateSlotOverloaded(%s, %f) called. Count = %d\n", str, d, privateCount);
    }
};

int TestPrivate::privateCount = 0;


class Test : public QObject {
    Q_OBJECT
public:
    Test() {
        QObject::connect(this, &Test::sig, [this]() { this->d->privateSlot(); });
    }
    virtual ~Test();
Q_SIGNALS:
    void sig();
    void sig2(int);
    void sig3(const char*);
    void sig4(const char*, double);
public Q_SLOTS:
    void publicSlot() { }
public: // so that it stays compilable, I guess in general only "this" will be captured and not other variables
    TestPrivate* d = new TestPrivate();
private:
    Q_PRIVATE_SLOT(d, void privateSlot())
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(int i2))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(const char* str2))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(const char* str2, double d2))
};

Test::~Test() {
    delete d;
}

int main(int argc, char** argv) {
    Test* test = new Test();
    if (TestPrivate::privateCount != 0) { abort(); }
    emit test->sig();
    if (TestPrivate::privateCount != 1) { abort(); }

    QObject obj;
    QObject::connect(&obj, &QObject::objectNameChanged, [&test]() { test->d->privateSlot(); });
    obj.setObjectName("foo");
    if (TestPrivate::privateCount != 2) { abort(); }

    QObject::connect(test, &Test::sig2, [&test](int i2) { test->d->privateSlotOverloaded(i2); });
    test->sig2(1);
    if (TestPrivate::privateCount != 3) { abort(); }
    QObject::connect(test, &Test::sig3, [&test](const char* str2) { test->d->privateSlotOverloaded(str2); });
    test->sig3("foo");
    if (TestPrivate::privateCount != 4) { abort(); }

    QObject::connect(test, &Test::sig4, [&test](const char* str2, double d2) { test->d->privateSlotOverloaded(str2, d2); });
    test->sig4("foo", 2.2);
    if (TestPrivate::privateCount != 5) { abort(); }

    test->disconnect();
    // 1 arg -> 0 arg
    QObject::connect(test, &Test::sig2, [&test](int i2) { test->d->privateSlot(); });
    QObject::connect(test, &Test::sig3, [&test](const char* str2) { test->d->privateSlot(); });
    test->sig2(2);
    if (TestPrivate::privateCount != 6) { abort(); }
    test->sig3("bar");
    if (TestPrivate::privateCount != 7) { abort(); }

    // 2 arg -> 1 arg
    QObject::connect(test, &Test::sig4, [&test](const char* str2, double d2) { test->d->privateSlotOverloaded(str2); });

    test->sig4("bar", 3.3);
    if (TestPrivate::privateCount != 8) { abort(); }

    // 2 arg -> 0 arg
    QObject::connect(test, &Test::sig4, [&test](const char* str2, double d2) { test->d->privateSlot(); });
    test->sig4("baz", 3.3); // should call 2 slots
    if (TestPrivate::privateCount != 10) { abort(); }

    return TestPrivate::privateCount == 10;
}

#include "q_private_slot.moc"
