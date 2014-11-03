#include <QObject>

class Test : public QObject {
    Q_OBJECT
public:
    enum FooEnum {
        FooOne,
        FooTwo
    };
    Q_ENUMS(FooEnum)
    Test() = default;
Q_SIGNALS:
    void foo(const char* s);
    void foo(FooEnum e);
private:
    QObject* other;
};

int main() {
    Test test;
    QObject::connect(&test, static_cast<void(Test::*)(const char*)>(&Test::foo), &test, &Test::deleteLater);
    QObject::connect(&test, static_cast<void(Test::*)(Test::FooEnum)>(&Test::foo), &test, &Test::deleteLater);
    return 0;
}

#include "enum_scoping.moc"
