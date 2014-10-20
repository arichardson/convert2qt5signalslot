#include <QObject>

template<class T>
class Foo : public QObject {
public:
    T templateSlot() { return T(); };
};

template<class T, size_t s = 64>
class Bar : public QObject {
public:
    T templateSlot() { return T(s); };
};

int main() {
    Foo<int> foo;
    QObject::connect(&foo, &Foo<int>::objectNameChanged, &foo, &Foo<int>::templateSlot);
    Bar<int> bar1;
    QObject::connect(&bar1, &Bar<int>::objectNameChanged, &bar1, &Bar<int>::templateSlot);
    Bar<int, 42> bar2; // non-default second arg
    QObject::connect(&bar2, &Bar<int, 42>::objectNameChanged, &bar2, &Bar<int, 42>::templateSlot);
}

#include "template_class.moc"
