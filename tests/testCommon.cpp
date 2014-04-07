#include "testCommon.h"

#include "../Qt5SignalSlotSyntaxConverter.h"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <llvm/Support/Signals.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/FileSystemStatCache.h>

#include <boost/algorithm/string/split.hpp>

using namespace clang::tooling;
using llvm::outs;

#define BASE_DIR "/this/path/does/not/exist"

#define FILE_NAME BASE_DIR "/test-input.cpp"
#define FAKE_QOBJECT_H_NAME BASE_DIR "/fake_qobject.h"

static const char* fakeQObjectCode =
        R"delim(

#define Q_OBJECT
#define Q_SLOTS
#define Q_SIGNALS public
#define Q_STATIC_ASSERT_X(cond, msg) static_assert(cond, msg)

const char *qFlagLocation(const char *method);

#define QT_STRINGIFY2(x) #x
#define QT_STRINGIFY(x) QT_STRINGIFY2(x)
#define QLOCATION "\0" __FILE__ ":" QT_STRINGIFY(__LINE__)
#define METHOD(a)   qFlagLocation("0"#a QLOCATION)
#define SLOT(a)     qFlagLocation("1"#a QLOCATION)
#define SIGNAL(a)   qFlagLocation("2"#a QLOCATION)

struct QMetaObject {
    struct Connection {};
};

class QString;

struct QMetaMethod {};

namespace Qt {
    enum ConnectionType {
        AutoConnection,
        DirectConnection,
        QueuedConnection,
        BlockingQueuedConnection,
        UniqueConnection =  0x80
    };
}

namespace QtPrivate {
    template <bool B, typename T = void> struct QEnableIf;
    template <typename T> struct QEnableIf<true, T> { typedef T Type; };


    template <typename T> struct RemoveRef { typedef T Type; };
    template <typename T> struct RemoveRef<T&> { typedef T Type; };
    template <typename T> struct RemoveConstRef { typedef T Type; };
    template <typename T> struct RemoveConstRef<const T&> { typedef T Type; };
    // With variadic template, lists are represented using a variadic template argument instead of the lisp way
    template <typename...> struct List {};
    template <typename Head, typename... Tail> struct List<Head, Tail...> { typedef Head Car; typedef List<Tail...> Cdr; };
    template <typename, typename> struct List_Append;
    template <typename... L1, typename...L2> struct List_Append<List<L1...>, List<L2...>> { typedef List<L1..., L2...> Value; };
    template <typename L, int N> struct List_Left {
        typedef typename List_Append<List<typename L::Car>,typename List_Left<typename L::Cdr, N - 1>::Value>::Value Value;
    };
    template <typename L> struct List_Left<L, 0> { typedef List<> Value; };
    // List_Select<L,N> returns (via typedef Value) the Nth element of the list L
    template <typename L, int N> struct List_Select { typedef typename List_Select<typename L::Cdr, N - 1>::Value Value; };
    template <typename L> struct List_Select<L,0> { typedef typename L::Car Value; };
    template <typename T>
    struct ApplyReturnValue {
        void *data;
        explicit ApplyReturnValue(void *data_) : data(data_) {}
    };
    template<typename T, typename U>
    void operator,(const T &value, const ApplyReturnValue<U> &container) {
        if (container.data)
            *reinterpret_cast<U*>(container.data) = value;
    }
    template<typename T, typename U>
    void operator,(T &&value, const ApplyReturnValue<U> &container) {
        if (container.data)
            *reinterpret_cast<U*>(container.data) = value;
    }
    template<typename T>
    void operator,(T, const ApplyReturnValue<void> &) {}
    template <int...> struct IndexesList {};
    template <typename IndexList, int Right> struct IndexesAppend;
    template <int... Left, int Right> struct IndexesAppend<IndexesList<Left...>, Right>
    { typedef IndexesList<Left..., Right> Value; };
    template <int N> struct Indexes
    { typedef typename IndexesAppend<typename Indexes<N - 1>::Value, N - 1>::Value Value; };
    template <> struct Indexes<0> { typedef IndexesList<> Value; };
    template<typename Func> struct FunctionPointer { enum {ArgumentCount = -1, IsPointerToMemberFunction = false}; };

    template <typename, typename, typename, typename> struct FunctorCall;
    template <int... II, typename... SignalArgs, typename R, typename Function>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, Function> {
        static void call(Function f, void **arg) {
            f((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };
    template <int... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...)> {
        static void call(SlotRet (Obj::*f)(SlotArgs...), Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };
    template <int... II, typename... SignalArgs, typename R, typename... SlotArgs, typename SlotRet, class Obj>
    struct FunctorCall<IndexesList<II...>, List<SignalArgs...>, R, SlotRet (Obj::*)(SlotArgs...) const> {
        static void call(SlotRet (Obj::*f)(SlotArgs...) const, Obj *o, void **arg) {
            (o->*f)((*reinterpret_cast<typename RemoveRef<SignalArgs>::Type *>(arg[II+1]))...), ApplyReturnValue<R>(arg[0]);
        }
    };

    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...)>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, o, arg);
        }
    };
    template<class Obj, typename Ret, typename... Args> struct FunctionPointer<Ret (Obj::*) (Args...) const>
    {
        typedef Obj Object;
        typedef List<Args...>  Arguments;
        typedef Ret ReturnType;
        typedef Ret (Obj::*Function) (Args...) const;
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = true};
        template <typename SignalArgs, typename R>
        static void call(Function f, Obj *o, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, o, arg);
        }
    };

    template<typename Ret, typename... Args> struct FunctionPointer<Ret (*) (Args...)>
    {
        typedef List<Args...> Arguments;
        typedef Ret ReturnType;
        typedef Ret (*Function) (Args...);
        enum {ArgumentCount = sizeof...(Args), IsPointerToMemberFunction = false};
        template <typename SignalArgs, typename R>
        static void call(Function f, void *, void **arg) {
            FunctorCall<typename Indexes<ArgumentCount>::Value, SignalArgs, R, Function>::call(f, arg);
        }
    };

    template<typename Function, int N> struct Functor
    {
        template <typename SignalArgs, typename R>
        static void call(Function &f, void *, void **arg) {
            FunctorCall<typename Indexes<N>::Value, SignalArgs, R, Function>::call(f, arg);
        }
    };
    template<typename A1, typename A2> struct AreArgumentsCompatible {
        static int test(A2);
        static char test(...);
        static A1 dummy();
        enum { value = sizeof(test(dummy())) == sizeof(int) };
    };
    template<typename A1, typename A2> struct AreArgumentsCompatible<A1, A2&> { enum { value = false }; };
    template<typename A> struct AreArgumentsCompatible<A&, A&> { enum { value = true }; };
    // void as a return value
    template<typename A> struct AreArgumentsCompatible<void, A> { enum { value = true }; };
    template<typename A> struct AreArgumentsCompatible<A, void> { enum { value = true }; };
    template<> struct AreArgumentsCompatible<void, void> { enum { value = true }; };

    template <typename List1, typename List2> struct CheckCompatibleArguments { enum { value = false }; };
    template <> struct CheckCompatibleArguments<List<>, List<>> { enum { value = true }; };
    template <typename List1> struct CheckCompatibleArguments<List1, List<>> { enum { value = true }; };
    template <typename Arg1, typename Arg2, typename... Tail1, typename... Tail2>
    struct CheckCompatibleArguments<List<Arg1, Tail1...>, List<Arg2, Tail2...>>
    {
        enum { value = AreArgumentsCompatible<typename RemoveConstRef<Arg1>::Type, typename RemoveConstRef<Arg2>::Type>::value
                    && CheckCompatibleArguments<List<Tail1...>, List<Tail2...>>::value };
    };
    template <typename Functor, typename ArgList> struct ComputeFunctorArgumentCount;

    template <typename Functor, typename ArgList, bool Done> struct ComputeFunctorArgumentCountHelper
    { enum { Value = -1 }; };
    template <typename Functor, typename First, typename... ArgList>
    struct ComputeFunctorArgumentCountHelper<Functor, List<First, ArgList...>, false>
        : ComputeFunctorArgumentCount<Functor,
            typename List_Left<List<First, ArgList...>, sizeof...(ArgList)>::Value> {};

    template <typename Functor, typename... ArgList> struct ComputeFunctorArgumentCount<Functor, List<ArgList...>>
    {
        template <typename D> static D dummy();
        template <typename F> static auto test(F f) -> decltype(((f.operator()((dummy<ArgList>())...)), int()));
        static char test(...);
        enum {
            Ok = sizeof(test(dummy<Functor>())) == sizeof(int),
            Value = Ok ? sizeof...(ArgList) : int(ComputeFunctorArgumentCountHelper<Functor, List<ArgList...>, Ok>::Value)
        };
    };

    /* get the return type of a functor, given the signal argument list  */
    template <typename Functor, typename ArgList> struct FunctorReturnType;
    template <typename Functor, typename ... ArgList> struct FunctorReturnType<Functor, List<ArgList...>> {
        template <typename D> static D dummy();
        typedef decltype(dummy<Functor>().operator()((dummy<ArgList>())...)) Value;
    };
}

class QObject {
public:
    QObject(QObject* parent = nullptr) {}

    static QMetaObject::Connection connect(const QObject *sender, const char *signal,
                        const QObject *receiver, const char *member, Qt::ConnectionType = Qt::AutoConnection);

    static QMetaObject::Connection connect(const QObject *sender, const QMetaMethod &signal,
                        const QObject *receiver, const QMetaMethod &method,
                        Qt::ConnectionType type = Qt::AutoConnection);

    inline QMetaObject::Connection connect(const QObject *sender, const char *signal,
                        const char *member, Qt::ConnectionType type = Qt::AutoConnection) const {
        return connect(sender, signal, this, member, type);
    }

    //Connect a signal to a pointer to qobject member function
    template <typename Func1, typename Func2>
    static inline QMetaObject::Connection connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                     const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot,
                                     Qt::ConnectionType type = Qt::AutoConnection)
    {
        // keep the checks here
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;
        Q_STATIC_ASSERT_X(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                          "The slot requires more arguments than the signal provides.");
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");

        return QMetaObject::Connection();
    }

    //connect to a function pointer  (not a member)
    template <typename Func1, typename Func2>
    static inline typename QtPrivate::QEnableIf<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0, QMetaObject::Connection>::Type
            connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
    {
        return connect(sender, signal, sender, slot, Qt::DirectConnection);
    }

    //connect to a function pointer  (not a member)
    template <typename Func1, typename Func2>
    static inline typename QtPrivate::QEnableIf<int(QtPrivate::FunctionPointer<Func2>::ArgumentCount) >= 0 &&
                                                !QtPrivate::FunctionPointer<Func2>::IsPointerToMemberFunction, QMetaObject::Connection>::Type
            connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, const QObject *context, Func2 slot,
                    Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;
        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                          "The slot requires more arguments than the signal provides.");
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");
        return QMetaObject::Connection();
    }

    //connect to a functor
    template <typename Func1, typename Func2>
    static inline typename QtPrivate::QEnableIf<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::Type
            connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 slot)
    {
        return connect(sender, signal, sender, slot, Qt::DirectConnection);
    }

    //connect to a functor, with a "context" object defining in which event loop is going to be executed
    template <typename Func1, typename Func2>
    static inline typename QtPrivate::QEnableIf<QtPrivate::FunctionPointer<Func2>::ArgumentCount == -1, QMetaObject::Connection>::Type
            connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, const QObject *context, Func2 slot,
                    Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        const int FunctorArgumentCount = QtPrivate::ComputeFunctorArgumentCount<Func2 , typename SignalType::Arguments>::Value;

        Q_STATIC_ASSERT_X((FunctorArgumentCount >= 0),
                          "Signal and slot arguments are not compatible.");
        const int SlotArgumentCount = (FunctorArgumentCount >= 0) ? FunctorArgumentCount : 0;
        typedef typename QtPrivate::FunctorReturnType<Func2, typename QtPrivate::List_Left<typename SignalType::Arguments, SlotArgumentCount>::Value>::Value SlotReturnType;

        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<SlotReturnType, typename SignalType::ReturnType>::value),
                          "Return type of the slot is not compatible with the return type of the signal.");
        return QMetaObject::Connection();
    }

    static bool disconnect(const QObject *sender, const char *signal,
                           const QObject *receiver, const char *member);
    static bool disconnect(const QObject *sender, const QMetaMethod &signal,
                           const QObject *receiver, const QMetaMethod &member);
    inline bool disconnect(const char *signal = 0,
                           const QObject *receiver = 0, const char *member = 0) const
        { return disconnect(this, signal, receiver, member); }
    inline bool disconnect(const QObject *receiver, const char *member = 0) const
        { return disconnect(this, 0, receiver, member); }
    static bool disconnect(const QMetaObject::Connection &);

    template <typename Func1, typename Func2>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;
        //compilation error if the arguments does not match.
        Q_STATIC_ASSERT_X((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");
        return true;
    }
    template <typename Func1>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const QObject *receiver, void **zero)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        return true;
    }

//now the signals and slots for QObject
public Q_SLOTS:
    void deleteLater();
Q_SIGNALS:
    void destroyed(QObject* obj = 0);
    void objectNameChanged(const QString& objectName);
};

)delim";

typedef std::vector<std::string> StringList;
static const auto isNewline = [](char c) {return c == '\n';};

static bool runTool(clang::FrontendAction *toolAction, const std::string code) {
    std::vector<std::string> commands { "clang-tool", "-fsyntax-only", "-std=c++11", FILE_NAME };
    clang::FileSystemOptions opt;
    opt.WorkingDir = BASE_DIR;
    clang::FileManager* files = new clang::FileManager{ opt };
    ToolInvocation Invocation(commands, toolAction, files);
    //for some reason free() is called on these strings, although StringRef should be non-owning
    Invocation.mapVirtualFile(FILE_NAME, code);
    Invocation.mapVirtualFile(FAKE_QOBJECT_H_NAME, fakeQObjectCode);
    return Invocation.run();
}

int testMain(std::string input, std::string expected) {
    llvm::sys::PrintStackTraceOnErrorSignal();
    StringList refactoringFiles { FILE_NAME };

    MatchFinder matchFinder;
    Replacements replacements;
    ConnectConverter converter(&replacements, refactoringFiles);
    converter.setupMatchers(&matchFinder);
    auto factory = newFrontendActionFactory(&matchFinder, converter.sourceCallback());
    auto action = factory->create();

    bool success = runTool(action, input);
    if (!success) {
        outs() << "Failed to run tool!\n";
        return 1;
    }
    outs() << "\n";
    converter.printStats();
    outs().flush();
    ssize_t offsetAdjust = 0;
    for (const Replacement& rep : replacements) {
        if (rep.getFilePath() != FILE_NAME) {
            outs() << "Attempting to write to illegal file:" << rep.getFilePath() << "\n";
            success = false;
        }
        input.replace(size_t(offsetAdjust + ssize_t(rep.getOffset())), rep.getLength(), rep.getReplacementText().str());
        offsetAdjust += ssize_t(rep.getReplacementText().size()) - ssize_t(rep.getLength());
    }
    outs() << "Comparing result with expected result...\n";
    if (input == expected) {
        (outs().changeColor(llvm::raw_ostream::GREEN, true) << "Success!\n").resetColor();
        return 0;
    }
    outs().flush();
    // something is wrong
    StringList expectedLines;
    boost::split(expectedLines, expected, isNewline);
    StringList resultLines;
    boost::split(resultLines, input, isNewline);
    auto lineCount = std::min(resultLines.size(), expectedLines.size());
    for (size_t i = 0; i < lineCount; ++i) {
        if (expectedLines[i] != resultLines[i]) {
            ((((outs() << i << " expected:").changeColor(llvm::raw_ostream::GREEN, true) << expectedLines[i]).resetColor()
                    << "\n" << i << " result  :").changeColor(llvm::raw_ostream::RED, true) << resultLines[i]).resetColor()
                    << "\n";
        } else {
            //outs() << i << "  okay   :" << expectedLines[i] << "\n";
            // nothing
        }
    }
    if (resultLines.size() > lineCount) {
        outs() << "Additional lines in result:\n";
        for (size_t i = lineCount; i < resultLines.size(); ++i) {
            outs() << i << " = '" << resultLines[i] << "'\n";
        }
    }
    if (expectedLines.size() > lineCount) {
        outs() << "Additional lines in expected:\n";
        for (size_t i = lineCount; i < expectedLines.size(); ++i) {
            outs() << i << " = '" << expectedLines[i] << "'\n";
        }
    }
    (outs().changeColor(llvm::raw_ostream::RED, true) << "Failed!\n").resetColor();
    return 1;
}
