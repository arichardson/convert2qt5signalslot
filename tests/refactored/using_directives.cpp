#include <QObject>

namespace Namespace {
class Foo : public QObject {
    Q_OBJECT
public:
    Foo() = default;
Q_SIGNALS:
    void sig();
    void sig(int);
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
    }
    using Namespace::Foo;
    using Namespace::Nested::Bar;
}

int main() {
    return 0;
}

#include "using_directives.moc"
