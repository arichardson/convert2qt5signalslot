#include <QObject>

namespace Namespace {
class Foo : public QObject {
    Q_OBJECT
public:
    Foo() = default;
Q_SIGNALS:
    void sig1();
    void sig2();
    void sig2(int);
};
}

static Namespace::Foo* foo = new Namespace::Foo();

namespace UsingNamespace {
    using namespace Namespace;
    void test() {
        QObject::connect(foo, &Foo::sig1, foo, &Foo::deleteLater);
        QObject::connect(foo, &Foo::sig1, foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig2), foo, &Foo::deleteLater);
    }
}

namespace UsingNamespaceAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, &Namespace::Foo::sig1, foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, &Namespace::Foo::sig1, foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
    }
    using namespace Namespace;
}

namespace UsingFoo {
    using Namespace::Foo;
    void test() {
        QObject::connect(foo, &Foo::sig1, foo, &Foo::deleteLater);
        QObject::connect(foo, &Foo::sig1, foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)()>(&Foo::sig2), foo, &Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Foo::*)(int)>(&Foo::sig2), foo, &Foo::deleteLater);
    }
}
namespace UsingFooAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, &Namespace::Foo::sig1, foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, &Namespace::Foo::sig1, foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)()>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
        QObject::connect(foo, static_cast<void(Namespace::Foo::*)(int)>(&Namespace::Foo::sig2), foo, &Namespace::Foo::deleteLater);
    }
    using Namespace::Foo;
}

int main() {
    return 0;
}

#include "using_directives.moc"
