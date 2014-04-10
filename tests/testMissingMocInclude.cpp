#include "testCommon.h"

const char* test1 = "#include \"foo.moc\"";
const char* test2 = "#include \"moc_foo.cpp\"";
const char* test3 = "#include \"foo_moc.cpp\"";

const char* base = R"--(
#include <qobjectdefs.h>

int main() {
    QObject* obj = new QObject();
    obj->connect(obj, SIGNAL(objectNameChanged(QString)), SLOT(deleteLater()));
    return 0;
}

)--";

const char* baseOut = R"--(
#include <qobjectdefs.h>

int main() {
    QObject* obj = new QObject();
    QObject::connect(obj, &QObject::objectNameChanged, obj, &QObject::deleteLater);
    return 0;
}

)--";

int main() {
    std::string inBase(base);
    std::string outBase(baseOut);
    int ret = testMainWithoutCompileCheck(inBase + test1, outBase + test1, 1, 1);
    if (ret != 0)
        return ret;
    ret = testMainWithoutCompileCheck(inBase + test2, outBase + test2, 1, 1);
    if (ret != 0)
        return ret;
    return testMainWithoutCompileCheck(inBase + test3, outBase + test3, 1, 1);
}
