#include <QObject>
#include <QUrl>

namespace X {
    class Y;
}

namespace Used {
    class U;
}

using namespace Used;

class Foo : public QObject {
    Q_OBJECT
public:
    Foo() = default;
Q_SIGNALS:
    void ref(QUrl&);
    void ref(); // force disambiguation
    void constRef(const QUrl&);
    void constRef(); // force disambiguation
    void ptr(QUrl*);
    void ptr(); // force disambiguation
    void constPtr(const QUrl*);
    void constPtr(); // force disambiguation
    void complex(const X::Y* const* volatile const* const);
    void complex(); // force disambiguation
    void usedComplex(const U* const* volatile const* const);
    void usedComplex();
public Q_SLOTS:
    void overloadedSlot(QUrl&) {
        printf("Called overloadedSlot(QUrl&)\n");
        callCount[0]++;
    }
    void overloadedSlot(const QUrl&) {
        printf("Called overloadedSlot(const QUrl&)\n");
        callCount[1]++;
    }
    void overloadedSlot(QUrl*) {
        printf("Called overloadedSlot(QUrl*)\n");
        callCount[2]++;
    }
    void overloadedSlot(const QUrl*) {
        printf("Called overloadedSlot(const QUrl*)\n");
        callCount[3]++;
    }
    void overloadedSlot() {}
private:
    int callCount[4] = { 0, 0, 0, 0 };

public:
    void checkCallCount(int c1, int c2, int c3, int c4) {
        if (c1 != callCount[0] || c2 != callCount[1] || c3 != callCount[2] || c4 != callCount[3]) {
            fprintf(stderr, "Call count mismatch: (%d, %d, %d, %d) vs (%d, %d, %d, %d)\n",
                callCount[0], callCount[1], callCount[2], callCount[3], c1, c2, c3, c4);
            exit(1);
        }
    }
};

int main() {
    QUrl url;
    QUrl& r = url;
    const QUrl& cr = url;
    QUrl* p = &url;
    const QUrl* cp = &url;
    Foo* foo = new Foo;

    // exact matches
    QObject::connect(foo, static_cast<void(Foo::*)(const QUrl*)>(&Foo::constPtr), foo, static_cast<void(Foo::*)(const QUrl*)>(&Foo::overloadedSlot));
    QObject::connect(foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::constRef), foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::overloadedSlot));
    QObject::connect(foo, static_cast<void(Foo::*)(QUrl*)>(&Foo::ptr), foo, static_cast<void(Foo::*)(QUrl*)>(&Foo::overloadedSlot));
    QObject::connect(foo, static_cast<void(Foo::*)(QUrl&)>(&Foo::ref), foo, static_cast<void(Foo::*)(QUrl&)>(&Foo::overloadedSlot));
    // added const in slot doesn't work!
    //QObject::connect(foo, SIGNAL(ptr(QUrl*)), foo, SLOT(overloadedSlot(const QUrl*)));
    //QObject::connect(foo, SIGNAL(ref(QUrl&)), foo, SLOT(overloadedSlot(const QUrl&)));

    foo->ref(r);
    foo->checkCallCount(1, 0, 0, 0);
    foo->constRef(cr);
    foo->checkCallCount(1, 1, 0, 0);
    foo->ptr(p);
    foo->checkCallCount(1, 1, 1, 0);
    foo->constPtr(cp);
    foo->checkCallCount(1, 1, 1, 1);

    foo->disconnect();

    // without normalization QUrl would be mapped to QUrl& or QUrl* since that is only one char different!!

    /* QObject::connect: Incompatible sender/receiver arguments
     *         Foo::ref(QUrl&) --> Foo::overloadedSlot(QUrl)
     */
    // QObject::connect(foo, SIGNAL(ref(QUrl&)), foo, SLOT(overloadedSlot(QUrl)));
    QObject::connect(foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::constRef), foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::overloadedSlot)); // normalization: QUrl is equiv to const QUrl&
    QObject::connect(foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::constRef), foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::overloadedSlot)); // normalization: QUrl is equiv to const QUrl&
    QObject::connect(foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::constRef), foo, static_cast<void(Foo::*)(const QUrl&)>(&Foo::overloadedSlot)); // normalization: QUrl is equiv to const QUrl&

    foo->constRef(cr);
    foo->checkCallCount(1, 4, 1, 1);

    // make sure the type printing works correctly
    QObject::connect(foo, static_cast<void(Foo::*)(const X::Y* const* volatile const* const)>(&Foo::complex), foo, static_cast<void(Foo::*)()>(&Foo::overloadedSlot));
    QObject::connect(foo, static_cast<void(Foo::*)(const U* const* volatile const* const)>(&Foo::usedComplex), foo, static_cast<void(Foo::*)()>(&Foo::overloadedSlot));

    return 0;
}

#include "normalized_signatures.moc"
