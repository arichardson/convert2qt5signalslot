#include <QObject>

class Base : public QObject {
    Q_OBJECT
public:
    Base();
Q_SIGNALS:
    void sig_int(int); //nothing special, used when testing slots
    void sig_ccp(const char*); //nothing special, used when testing slots
    void sig_void(); //nothing special, used when testing slots

    void overloadedSig(int);
    void overloadedSig(const char*);
    void sigOverloadedDerived();
public Q_SLOTS:
    void slot(); //nothing special, used when testing signals

    virtual void slotOverride(int);
    virtual void slotOverloadedVirtual();
    void slotOverloadedDerived(int); // now the same just not virtual
    void slotHide(const char*);
};

class Derived : public Base {
    Q_OBJECT
public:
    Derived();
Q_SIGNALS:
    using Base::sigOverloadedDerived; // This is needed so that connection works with &Derived::slotOverloadedDerived
    void sigOverloadedDerived(int);
public Q_SLOTS:
    virtual void slotOverride(int) override;
    using Base::slotOverloadedVirtual; // silence warning
    virtual void slotOverloadedVirtual(int);
    using Base::slotOverloadedDerived; // This is needed so that connection works with &Derived::slotOverloadedDerived
    void slotOverloadedDerived(const char*);
    void slotHide(const char*);
};

int main() {
    Base* base = new Base();
    Derived* derived = new Derived();
    //check overloaded signals (need disambiguation)
    QObject::connect(base, static_cast<void(Base::*)(int)>(&Base::overloadedSig), base, &Base::slot);
    QObject::connect(derived, static_cast<void(Derived::*)(int)>(&Derived::overloadedSig), base, &Base::slot);
    QObject::connect(base, static_cast<void(Base::*)(const char*)>(&Base::overloadedSig), derived, &Derived::slot);
    QObject::connect(derived, static_cast<void(Derived::*)(const char*)>(&Derived::overloadedSig), derived, &Derived::slot);

    // this only needs disambiguation when connecting via Derived*
    QObject::connect(base, &Base::sigOverloadedDerived, base, &Base::slot);
    QObject::connect(derived, static_cast<void(Derived::*)()>(&Derived::sigOverloadedDerived), base, &Base::slot);
    QObject::connect(derived, static_cast<void(Derived::*)(int)>(&Derived::sigOverloadedDerived), base, &Base::slot);

    //check overriden slots (-> no disambiguation in both cases)
    QObject::connect(base, &Base::sig_int, base, &Base::slotOverride);
    QObject::connect(base, &Base::sig_int, derived, &Derived::slotOverride);

    // this only needs disambiguation when connecting to Derived*
    QObject::connect(base, &Base::sig_int, base, &Base::slotOverloadedDerived);
    QObject::connect(base, &Base::sig_ccp, derived, static_cast<void(Derived::*)(const char*)>(&Derived::slotOverloadedDerived));
    QObject::connect(base, &Base::sig_int, derived, static_cast<void(Derived::*)(int)>(&Derived::slotOverloadedDerived));

    // this only needs disambiguation when connecting to Derived*
    QObject::connect(base, &Base::sig_void, base, &Base::slotOverloadedVirtual);
    QObject::connect(base, &Base::sig_void, derived, static_cast<void(Derived::*)()>(&Derived::slotOverloadedVirtual));
    QObject::connect(base, &Base::sig_int, derived, static_cast<void(Derived::*)(int)>(&Derived::slotOverloadedVirtual));

    // No need for disambiguation, they hide each other and are no overloads
    QObject::connect(base, &Base::sig_ccp, base, &Base::slotHide);
    QObject::connect(base, &Base::sig_ccp, derived, &Derived::slotHide);
    return 0;
}
