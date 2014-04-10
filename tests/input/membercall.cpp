#include <QObject>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QObject* someObj() { return q; }
    QObject* q;
};

Test::Test() {
    q = new QObject(this);
    // inside QObject subclass -> don't have to prefix with QObject::

    // here connect receiver/disconnect sender is "this"
    connect(q, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    disconnect(SIGNAL(objectNameChanged(QString)), q, SLOT(deleteLater()));
    this->connect(q, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    this->disconnect(SIGNAL(objectNameChanged(QString)), q, SLOT(deleteLater()));
    //ensure that the connect receiver/disconnect sender is correctly set ("q")
    q->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    q->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));
    //now try with a function call
    someObj()->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    someObj()->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject value;
    value.connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    value.disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject& ref = *q;
    ref.connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    ref.disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject*& ptrRef = q;
    ptrRef->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    ptrRef->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    QObject** ptrPtr = &q;
    (*ptrPtr)->connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    (*ptrPtr)->disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));

    // dereferenced pointer
    (*q).connect(this, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    (*q).disconnect(SIGNAL(objectNameChanged(QString)), this, SLOT(deleteLater()));
}

int main() {
    Test* test = nullptr;
    QObject* obj = nullptr;
    // now we need the QObject:: added
    test->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    obj->connect(test, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    test->disconnect(SIGNAL(objectNameChanged(QString)), obj, SLOT(deleteLater()));
    obj->disconnect(SIGNAL(objectNameChanged(QString)), test, SLOT(deleteLater()));
    return 0;
}

#include "membercall.moc"
