#include <QObject>

namespace NS1 {
class Foo1  : public QObject {
    Q_OBJECT
public:
    void someSlot() {}
    // to make sure that the static_cast also qualifies members
    void overloaded(const QString&) {}
    void overloaded() {}
};
}

namespace NS2 {
class Foo2 : public QObject {
    Q_OBJECT
public:
    void someSlot() {}
};
class Bar2 : public QObject {
    Q_OBJECT
public:
    void someSlot() {}
};
}

namespace NS3 {
class Foo3 : public QObject {
    Q_OBJECT
public:
    void someSlot() {}
};
}

class Test : public QObject {
    Q_OBJECT
public:
    void globalUsingDirectives();
    void localUsingDirectives();
private:
    NS1::Foo1* foo1;
    NS2::Foo2* foo2;
    NS2::Bar2* bar2;
    NS3::Foo3* foo3;
};

void Test::localUsingDirectives() {
    connect(this, &Test::objectNameChanged, foo1, &NS1::Foo1::someSlot);
    connect(this, &Test::objectNameChanged, foo2, &NS2::Foo2::someSlot);
    connect(this, &Test::objectNameChanged, foo3, &NS3::Foo3::someSlot);

    using namespace NS3; //now we no longer need to prepend the namespace NS3
    connect(this, &Test::objectNameChanged, foo3, &Foo3::someSlot);
    using NS2::Foo2; //Foo2 no longer needs the namespace
    connect(this, &Test::objectNameChanged, foo2, &Foo2::someSlot);
    //however NS2::Bar2 does
    connect(this, &Test::objectNameChanged, bar2, &NS2::Bar2::someSlot);

    //test overloads
    connect(this, &Test::objectNameChanged, foo1, static_cast<void(NS1::Foo1::*)()>(&NS1::Foo1::overloaded));
    connect(this, &Test::objectNameChanged, foo1, static_cast<void(NS1::Foo1::*)(const QString&)>(&NS1::Foo1::overloaded));
    using namespace NS1;
    connect(this, &Test::objectNameChanged, foo1, static_cast<void(Foo1::*)(const QString&)>(&Foo1::overloaded));
}

using namespace NS3;
using NS2::Foo2;

void Test::globalUsingDirectives() {
    //just the same as the local test, however with using directives in the global scope
    connect(this, &Test::objectNameChanged, foo1, &NS1::Foo1::someSlot);
    connect(this, &Test::objectNameChanged, foo2, &Foo2::someSlot);
    connect(this, &Test::objectNameChanged, bar2, &NS2::Bar2::someSlot);
    connect(this, &Test::objectNameChanged, foo3, &Foo3::someSlot);
}

int main() {
    return 0;
}
