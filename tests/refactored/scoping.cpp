#include <QObject>

class BaseClass : public QObject {
    Q_OBJECT
public:
    BaseClass() : obj(new QObject()) {}
Q_SIGNALS:
    void sig();
protected:
    QObject* obj;

    class InnerClass1 : public QObject {
        //Q_OBJECT //error: Meta object features not supported for nested classes
    public:
        InnerClass1() {
            // no need to qualify with BaseClass:: since we are inside that class
            connect(this, &InnerClass1::objectNameChanged, obj, &QObject::deleteLater);
            connect(obj, &QObject::objectNameChanged, this, &InnerClass1::deleteLater);
        }
        QObject* obj = new QObject();
    };
};

namespace NameSpace {

class Class1 : public BaseClass {
    Q_OBJECT
public:
    Class1() {
        // no need to qualify with NameSpace:: since we are inside that namespace
        connect(this, &Class1::sig, obj, &QObject::deleteLater);
        connect(obj, &QObject::objectNameChanged, this, &Class1::deleteLater);
    }

    class InnerClass2 : public BaseClass {
        //Q_OBJECT //error: Meta object features not supported for nested classes
    public:
        InnerClass2() {
            // no need to qualify with NameSpace::Class1:: since we are inside that class
            connect(this, &InnerClass2::sig, obj, &QObject::deleteLater);
            connect(obj, &QObject::objectNameChanged, this, &InnerClass2::deleteLater);
        }
    };
};

namespace NestedNameSpace {
// same thing again, just this time nested
class Class2 : public BaseClass {
    Q_OBJECT
public:
    Class2() {
        // no need to qualify with NameSpace::NestedNameSpace:: since we are inside that namespace
        connect(this, &Class2::sig, obj, &QObject::deleteLater);
        connect(obj, &QObject::objectNameChanged, this, &Class2::deleteLater);
    }

    class InnerClass3 : public BaseClass {
        //Q_OBJECT //error: Meta object features not supported for nested classes
    public:
        InnerClass3() {
            // no need to qualify with NameSpace::Class1:: since we are inside that class
            connect(this, &InnerClass3::sig, obj, &QObject::deleteLater);
            connect(obj, &QObject::objectNameChanged, this, &InnerClass3::deleteLater);
        }
    };
};

} // namespace NestedNameSpace
} // namespace NameSpace

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

#include "scoping.moc"
