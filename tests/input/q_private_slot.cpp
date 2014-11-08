#include <QObject>
#include <QString>

namespace NS {
     class Used {};
}

using namespace NS;

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
    void privateSlotOverloaded(Used* u) {
        privateCount++;
        fprintf(stderr, "privateSlotOverloaded(%p) called. Count = %d\n", (void*)u, privateCount);
    }
    void privateSlotOverloaded(bool b) {
        privateCount++;
        fprintf(stderr, "privateSlotOverloaded(%s) called. Count = %d\n", b ? "true" : "false", privateCount);
    }
};

int TestPrivate::privateCount = 0;

class Test : public QObject {
    Q_OBJECT
public:
    Test() {
        connect(this, SIGNAL(sig()), SLOT(privateSlot()));
    }
    virtual ~Test();
    void ensureNoSuperflousThisAdded();
Q_SIGNALS:
    void sig();
    void sig2(int);
    void sig3(const char*);
    void sig4(const char*, double);
    void sig5(NS::Used*); // have to qualify here otherwise moc breaks
    void sig6(bool);
public Q_SLOTS:
    void publicSlot() { }
public: // so that it stays compilable, I guess in general only "this" will be captured and not other variables
    TestPrivate* d = new TestPrivate();
private:
    Q_PRIVATE_SLOT(d, void privateSlot())
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(int i2))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(const char* str2))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(const char* str2, double d2))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(NS::Used* u))
    Q_PRIVATE_SLOT(d, void privateSlotOverloaded(bool b))
};

Test::~Test() {
    delete d;
}

void Test::ensureNoSuperflousThisAdded() {
    connect(this, SIGNAL(sig3(const char*)), SLOT(privateSlotOverloaded(const char*)));
}

int main(int argc, char** argv) {
    Test* test = new Test();
    if (TestPrivate::privateCount != 0) { abort(); }
    emit test->sig(); // connected in constuructor
    if (TestPrivate::privateCount != 1) { abort(); }

    QObject obj;
    QObject::connect(&obj, SIGNAL(objectNameChanged(QString)), test, SLOT(privateSlot()));
    obj.setObjectName("foo");
    if (TestPrivate::privateCount != 2) { abort(); }

    test->connect(test, SIGNAL(sig2(int)), SLOT(privateSlotOverloaded(int)));
    test->sig2(1);
    if (TestPrivate::privateCount != 3) { abort(); }
    test->connect(test, SIGNAL(sig3(const char*)), SLOT(privateSlotOverloaded(const char*)));
    test->sig3("foo");
    if (TestPrivate::privateCount != 4) { abort(); }

    test->connect(test, SIGNAL(sig4(const char*, double)), SLOT(privateSlotOverloaded(const char*, double)));
    test->sig4("foo", 2.2);
    if (TestPrivate::privateCount != 5) { abort(); }

    test->disconnect();
    // 1 arg -> 0 arg
    test->connect(test, SIGNAL(sig2(int)), SLOT(privateSlot()));
    test->connect(test, SIGNAL(sig3(const char*)), SLOT(privateSlot()));
    test->sig2(2);
    if (TestPrivate::privateCount != 6) { abort(); }
    test->sig3("bar");
    if (TestPrivate::privateCount != 7) { abort(); }

    // 2 arg -> 1 arg
    test->connect(test, SIGNAL(sig4(const char*, double)), SLOT(privateSlotOverloaded(const char*)));
    test->sig4("bar", 3.3);
    if (TestPrivate::privateCount != 8) { abort(); }

    // 2 arg -> 0 arg
    test->connect(test, SIGNAL(sig4(const char*, double)), SLOT(privateSlot()));
    test->sig4("baz", 3.3); // should call 2 slots
    if (TestPrivate::privateCount != 10) { abort(); }

    // test using (namespace) directives are handled
    test->connect(test, SIGNAL(sig5(NS::Used*)), SLOT(privateSlotOverloaded(NS::Used*)));
    test->sig5((Used*)nullptr); // should call 1 slots
    if (TestPrivate::privateCount != 11) { abort(); }

    // test that bool doesn't change to _Bool
    test->connect(test, SIGNAL(sig6(bool)), SLOT(privateSlotOverloaded(bool)));
    test->sig6(true); // should call 1 slots

    if (TestPrivate::privateCount == 12) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

#include "q_private_slot.moc"
