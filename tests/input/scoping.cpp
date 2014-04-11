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
            connect(this, SIGNAL(objectNameChanged()), obj, SLOT(deleteLater()));
            connect(obj, SIGNAL(objectNameChanged()), this, SLOT(deleteLater()));
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
        connect(this, SIGNAL(sig()), obj, SLOT(deleteLater()));
        connect(obj, SIGNAL(objectNameChanged()), this, SLOT(deleteLater()));
    }

    class InnerClass2 : public BaseClass {
        //Q_OBJECT //error: Meta object features not supported for nested classes
    public:
        InnerClass2() {
            // no need to qualify with NameSpace::Class1:: since we are inside that class
            connect(this, SIGNAL(sig()), obj, SLOT(deleteLater()));
            connect(obj, SIGNAL(objectNameChanged()), this, SLOT(deleteLater()));
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
        connect(this, SIGNAL(sig()), obj, SLOT(deleteLater()));
        connect(obj, SIGNAL(objectNameChanged()), this, SLOT(deleteLater()));
    }

    class InnerClass3 : public BaseClass {
        //Q_OBJECT //error: Meta object features not supported for nested classes
    public:
        InnerClass3() {
            // no need to qualify with NameSpace::Class1:: since we are inside that class
            connect(this, SIGNAL(sig()), obj, SLOT(deleteLater()));
            connect(obj, SIGNAL(objectNameChanged()), this, SLOT(deleteLater()));
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
    connect(this, SIGNAL(objectNameChanged(QString)), foo1, SLOT(someSlot()));
    connect(this, SIGNAL(objectNameChanged(QString)), foo2, SLOT(someSlot()));
    connect(this, SIGNAL(objectNameChanged(QString)), foo3, SLOT(someSlot()));

    using namespace NS3; //now we no longer need to prepend the namespace NS3
    connect(this, SIGNAL(objectNameChanged(QString)), foo3, SLOT(someSlot()));
    using NS2::Foo2; //Foo2 no longer needs the namespace
    connect(this, SIGNAL(objectNameChanged(QString)), foo2, SLOT(someSlot()));
    //however NS2::Bar2 does
    connect(this, SIGNAL(objectNameChanged(QString)), bar2, SLOT(someSlot()));

    //test overloads
    connect(this, SIGNAL(objectNameChanged(QString)), foo1, SLOT(overloaded()));
    connect(this, SIGNAL(objectNameChanged(QString)), foo1, SLOT(overloaded(const QString&)));
    using namespace NS1;
    connect(this, SIGNAL(objectNameChanged(QString)), foo1, SLOT(overloaded(const QString&)));
}

using namespace NS3;
using NS2::Foo2;

void Test::globalUsingDirectives() {
    //just the same as the local test, however with using directives in the global scope
    connect(this, SIGNAL(objectNameChanged(QString)), foo1, SLOT(someSlot()));
    connect(this, SIGNAL(objectNameChanged(QString)), foo2, SLOT(someSlot()));
    connect(this, SIGNAL(objectNameChanged(QString)), bar2, SLOT(someSlot()));
    connect(this, SIGNAL(objectNameChanged(QString)), foo3, SLOT(someSlot()));
}

int main() {
    return 0;
}

#include "scoping.moc"
