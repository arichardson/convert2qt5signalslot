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
    foo.connect(&foo, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    Bar<int> bar1;
    bar1.connect(&bar1, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    Bar<int, 42> bar2; // non-default second arg
    bar2.connect(&bar2, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
}

#include "template_class.moc"
