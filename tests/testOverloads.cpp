#include "testCommon.h"

static const char* input = R"delim(
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QComboBox* box;
};

Test::Test() {
    connect(box, SIGNAL(activated(int)), box, SLOT(deleteLater()));
    connect(box, SIGNAL(activated(const QString&)), SLOT(deleteLater()));
    //this will fail to compile, but the compiler should suggest const QString&
    //In the string based connect version both are identical
    //TODO maybe fix this issue
    connect(box, SIGNAL(activated(QString)), SLOT(deleteLater()));
    
    //check that it also works in the SLOT part
    connect(this, SIGNAL(objectNameChanged(QString)), box, SLOT(setFocus()));
}
)delim";

static const char* output = R"delim(
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>

class Test : public QObject {
    Q_OBJECT
public:
    Test();
private:
    QComboBox* box;
};

Test::Test() {
    connect(box, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), box, &QComboBox::deleteLater);
    connect(box, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::activated), this, &Test::deleteLater);
    //this will fail to compile, but the compiler should suggest const QString&
    //In the string based connect version both are identical
    //TODO maybe fix this issue
    connect(box, static_cast<void(QComboBox::*)(QString)>(&QComboBox::activated), this, &Test::deleteLater);
    
    //check that it also works in the SLOT part
    connect(this, &Test::objectNameChanged, box, static_cast<void(QComboBox::*)()>(&QComboBox::setFocus));
}
)delim";

int main() {
    return testMain(input, output);
}

