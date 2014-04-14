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
        QObject::connect(foo, SIGNAL(sig1()), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig1()), SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2()), foo, SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2(int)), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2()), SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2(int)), SLOT(deleteLater()));
    }
}

namespace UsingNamespaceAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig1()), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig1()), SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2()), foo, SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2(int)), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2()), SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2(int)), SLOT(deleteLater()));
    }
    using namespace Namespace;
}

namespace UsingFoo {
    using Namespace::Foo;
    void test() {
        QObject::connect(foo, SIGNAL(sig1()), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig1()), SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2()), foo, SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2(int)), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2()), SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2(int)), SLOT(deleteLater()));
    }
}
namespace UsingFooAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig1()), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig1()), SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2()), foo, SLOT(deleteLater()));
        QObject::connect(foo, SIGNAL(sig2(int)), foo, SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2()), SLOT(deleteLater()));
        foo->connect(foo, SIGNAL(sig2(int)), SLOT(deleteLater()));
    }
    using Namespace::Foo;
}

int main() {
    return 0;
}

#include "using_directives.moc"
