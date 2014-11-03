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
    test.connect(&test, SIGNAL(foo(const char*)), SLOT(deleteLater()));
    test.connect(&test, SIGNAL(foo(Test::FooEnum)), SLOT(deleteLater()));
    return 0;
}

#include "enum_scoping.moc"
