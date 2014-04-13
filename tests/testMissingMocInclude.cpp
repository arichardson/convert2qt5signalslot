#include "testCommon.h"

const char* test1 = "#include \"foo.moc\"";
const char* test2 = "#include \"moc_foo.cpp\"";
const char* test3 = "#include \"foo_moc.cpp\"";

const char* base = R"--(
#include <QObject>

int main() {
    QObject* obj = new QObject();
    obj->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    return 0;
}

)--";

const char* baseOut = R"--(
#include <QObject>

int main() {
    QObject* obj = new QObject();
    QObject::connect(obj, &QObject::objectNameChanged, obj, &QObject::deleteLater);
    return 0;
}

)--";

const char* mocCode = "struct moc {};";
const char* mocCode2 = "#error moc code";

int main() {
    std::string inBase(base);
    std::string outBase(baseOut);
    bool ok = codeCompiles(inBase + test1, {{BASE_DIR "foo.moc", mocCode}}) && codeCompiles(outBase + test1, {{BASE_DIR "foo.moc", mocCode}})
            && testMain(inBase + test1, outBase + test1, 1, 1) == 0
            && codeCompiles(inBase + test2, {{BASE_DIR "moc_foo.cpp", mocCode}}) && codeCompiles(outBase + test2, {{BASE_DIR "moc_foo.cpp", mocCode}})
            && testMain(inBase + test2, outBase + test2, 1, 1) == 0
            && codeCompiles(inBase + test3, {{BASE_DIR "foo_moc.cpp", mocCode}}) && codeCompiles(outBase + test3, {{BASE_DIR "foo_moc.cpp", mocCode}})
            && testMain(inBase + test3, outBase + test3, 1, 1) == 0;
    return ok ? 0 : 1;
}
