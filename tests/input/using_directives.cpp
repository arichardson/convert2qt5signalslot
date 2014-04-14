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
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingNamespaceAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
    using namespace Namespace;
}
namespace UsingNested {
    using namespace Namespace::Nested;
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingNamespaceAndNested {
    using namespace Namespace::Nested;
    using namespace Namespace;
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingFoo {
    using Namespace::Foo;
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingBar {
    using Namespace::Nested::Bar;
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingFooAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
    using Namespace::Foo;
}
namespace UsingBarAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
    using Namespace::Nested::Bar;
}
namespace UsingFooAndBar {
    using Namespace::Foo;
    using Namespace::Nested::Bar;
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
}
namespace UsingFooAndBarAfterFunc {
    // using directive is after function -> have to qualify
    void test() {
        QObject::connect(foo, SIGNAL(sig(int)),
                foo, SLOT(deleteLater()));
        bar->connect(foo, SIGNAL(sig()),
                SLOT(deleteLater()));
        QObject::connect(bar, SIGNAL(sig()),
                bar, SLOT(deleteLater()));
        foo->connect(bar, SIGNAL(sig(int)),
                SLOT(deleteLater()));
    }
    using Namespace::Foo;
    using Namespace::Nested::Bar;
}

int main() {
    return 0;
}

#include "using_directives.moc"
