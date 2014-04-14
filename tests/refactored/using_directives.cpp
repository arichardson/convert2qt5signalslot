#include <QObject>

namespace Namespace {
class Foo : public QObject {
    Q_OBJECT
public:
    Foo() = default;
Q_SIGNALS:
    void sig();
    void sig(int);
public:
    class InnerClass : public QObject {
        // Q_OBJECT // moc disallows this for inner classes
    public:
        InnerClass() = default;
    };
};

namespace Nested {
class Bar : public QObject {
    Q_OBJECT
public:
    Bar() = default;
Q_SIGNALS:
    void sig();
    void sig(int);
};
}
}

static Namespace::Foo* foo = new Namespace::Foo();
static Namespace::Foo::InnerClass* inner= new Namespace::Foo::InnerClass();
static Namespace::Nested::Bar* bar = new Namespace::Nested::Bar();

namespace UsingNamespace {
    using namespace Namespace;
    void test() {
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig),
                foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig),
                bar, &Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Nested::Bar::*)()>(&Nested::Bar::sig),
                bar, &Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Nested::Bar::*)(int)>(&Nested::Bar::sig),
                foo, &Foo::deleteLater);
        QObject::connect(inner, &Foo::InnerClass::objectNameChanged,
                inner, &Foo::InnerClass::deleteLater);
    }
}
namespace UsingNamespaceAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)()>(&Namespace::Nested::Bar::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)(int)>(&Namespace::Nested::Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
    using namespace Namespace;
}
namespace UsingNested {
    using namespace Namespace::Nested;
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)()>(&Bar::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)(int)>(&Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
}
namespace UsingNamespaceAndNested {
    using namespace Namespace::Nested;
    using namespace Namespace;
    void test() {
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig),
                foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)()>(&Bar::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)(int)>(&Bar::sig),
                foo, &Foo::deleteLater);
        QObject::connect(inner, &Foo::InnerClass::objectNameChanged,
                inner, &Foo::InnerClass::deleteLater);
    }
}
namespace UsingFoo {
    using Namespace::Foo;
    void test() {
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig),
                foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)()>(&Namespace::Nested::Bar::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)(int)>(&Namespace::Nested::Bar::sig),
                foo, &Foo::deleteLater);
        QObject::connect(inner, &Foo::InnerClass::objectNameChanged,
                inner, &Foo::InnerClass::deleteLater);
    }
}
namespace UsingBar {
    using Namespace::Nested::Bar;
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)()>(&Bar::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)(int)>(&Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
}
namespace UsingFooAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)()>(&Namespace::Nested::Bar::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)(int)>(&Namespace::Nested::Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
    using Namespace::Foo;
}
namespace UsingBarAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)()>(&Namespace::Nested::Bar::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)(int)>(&Namespace::Nested::Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
    using Namespace::Nested::Bar;
}
namespace UsingFooAndBar {
    using Namespace::Foo;
    using Namespace::Nested::Bar;
    void test() {
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig),
                foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)()>(&Bar::sig),
                bar, &Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Bar::*)(int)>(&Bar::sig),
                foo, &Foo::deleteLater);
        QObject::connect(inner, &Foo::InnerClass::objectNameChanged,
                inner, &Foo::InnerClass::deleteLater);
    }
}
namespace UsingFooAndBarAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)()>(&Namespace::Nested::Bar::sig),
                bar, &Namespace::Nested::Bar::deleteLater);
        QObject::connect(bar, static_cast<void(Namespace::Nested::Bar::*)(int)>(&Namespace::Nested::Bar::sig),
                foo, &Namespace::Foo::deleteLater);
        QObject::connect(inner, &Namespace::Foo::InnerClass::objectNameChanged,
                inner, &Namespace::Foo::InnerClass::deleteLater);
    }
    using Namespace::Foo;
    using Namespace::Nested::Bar;
}

int main() {
    return 0;
}

#include "using_directives.moc"
