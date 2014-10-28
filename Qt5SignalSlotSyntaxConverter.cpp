#include "Qt5SignalSlotSyntaxConverter.h"
#include "PreProcessorCallback.h"
#include "ClangUtils.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Analysis/CFG.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/Scope.h>
#include <clang/Sema/Lookup.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>

#include <atomic>
#include <stdexcept>

#include <QMetaObject> // QMetaObject::normalizedSignature
#include <QByteArray>


using namespace clang;
using namespace clang::tooling;
using namespace ClangUtils;
using llvm::outs;
using llvm::errs;

llvm::cl::OptionCategory clCategory("convert2qt5signalslot specific options");
static llvm::cl::opt<bool> verboseMode("verbose", llvm::cl::cat(clCategory),
        llvm::cl::desc("Enable verbose output"), llvm::cl::init(false));
static llvm::cl::opt<bool> convertNotFound("convert-not-found", llvm::cl::cat(clCategory),
        llvm::cl::desc("Whether to convert to new syntax if no method with that name was found"), llvm::cl::init(true));
static llvm::cl::opt<bool> convertQQuickItemNotFound("convert-qquickitem-not-found", llvm::cl::cat(clCategory),
        llvm::cl::desc("Whether to convert to new syntax if no method with that name was found and object is a QQuickItem subclass. These are often not public API, so no function pointers are available"), llvm::cl::init(false));
// not static so that it can be modified by the tests
llvm::cl::opt<std::string> nullPtrString("nullptr", llvm::cl::init("nullptr"), llvm::cl::cat(clCategory),
        llvm::cl::desc("the string that will be used for a null pointer constant (Default is 'nullptr')"));

//http://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
static std::string expr2str(const Expr *d, ASTContext* ctx) {
    clang::SourceLocation b(sourceLocationBeforeStmt(d, ctx));
    clang::SourceLocation e(sourceLocationAfterStmt(d, ctx));
    const char* start = ctx->getSourceManager().getCharacterData(b);
    const char* end = ctx->getSourceManager().getCharacterData(e);
    if (!start || !end || end < start) {
        errs() << "could not get string for ";
        d->dumpPretty(*ctx);
        errs() << " at " << b.printToString(ctx->getSourceManager()) << "\nAST:\n";
        d->dumpColor();
        throw std::runtime_error("Failed to convert expression to string");
    }
    return std::string(start, size_t(end - start));
}

/** Signal/Slot string literals are always
 * "0" or "1" or "2" followed by the method name (e.g. "valueChanged(int)" )
 * then a null character, and then filename + linenumber
 *  the interesting part is from the second char to the first '(' character
 */
static StringRef signalSlotName(const StringLiteral* literal, ConnectCallMatcher::ReplacementType* type) {
    StringRef bytes = literal->getString();
    size_t bracePos = bytes.find_first_of('(');
    if (bracePos == StringRef::npos) {
        throw std::runtime_error(("invalid format of signal slot string: " + bytes).str());
    }
    if (bytes[0] == '1') {
        *type = ConnectCallMatcher::ReplaceSlot;
    } else if (bytes[0] == '2') {
        *type = ConnectCallMatcher::ReplaceSignal;
    } else {
        *type = ConnectCallMatcher::ReplaceOther;
        (errs() << "Connection string is neither signal nor slot:").write_escaped(bytes) << "\n";
    }
    return bytes.substr(1, bracePos - 1);
}

static StringRef signalSlotParameters(StringRef bytes) {
    auto openingPos = bytes.find_first_of('(');
    if (openingPos == StringRef::npos) {
        throw std::runtime_error(("invalid format of signal slot parameters: " + bytes).str());
    }
    auto closingPos = bytes.find(')', openingPos);
    if (closingPos == StringRef::npos || closingPos <= openingPos) {
        throw std::runtime_error(("invalid format of signal slot parameters: " + bytes).str());
    }
    StringRef result = bytes.slice(openingPos + 1, closingPos);
    //((outs() << "Parameters from '").write_escaped(bytes) << "' are '").write_escaped(result) << "'\n";
    return result;
}

void ConnectCallMatcher::addReplacement(SourceRange range, const std::string& replacement, ASTContext* ctx) {
    CharSourceRange csr = CharSourceRange::getCharRange(range);
    outs() << "Replacing '";
    colouredOut(llvm::raw_ostream::SAVEDCOLOR, true) << Lexer::getSourceText(csr, ctx->getSourceManager(), ctx->getLangOpts());
    outs() << "' with '";
    colouredOut(llvm::raw_ostream::SAVEDCOLOR, true) << replacement;
    outs() << "' (from ";
    csr.getBegin().print(outs(), ctx->getSourceManager());
    outs() << " to ";
    csr.getEnd().print(outs(), ctx->getSourceManager());
    outs() << ")\n";

    replacements->insert(Replacement(ctx->getSourceManager(), csr, replacement));
}


class SkipMatchException : public std::runtime_error {
public:
    SkipMatchException(const std::string& msg) : std::runtime_error(msg) {}
    SkipMatchException(const SkipMatchException&) = default;
    virtual ~SkipMatchException() noexcept;
};

SkipMatchException::~SkipMatchException() noexcept {}

void ConnectCallMatcher::run(const MatchFinder::MatchResult& result) {
    try {
        assert(currentCompilerInstance);
        //sema and pp must be null or unchanged
        assert(!sema || sema == &currentCompilerInstance->getSema());
        assert(!pp || pp == &currentCompilerInstance->getPreprocessor());
        sema = &currentCompilerInstance->getSema();
        pp = &currentCompilerInstance->getPreprocessor();
        convert(result);
    }
    catch (const SkipMatchException& e) {
        skippedMatches++;
        outs().flush();
        (colouredOut(llvm::raw_ostream::MAGENTA) << "Not converting current match. Reason: ").writeEscaped(e.what());
        (outs() << "\n\n").flush();
    }
    catch (const std::exception& e) {
        failedMatches++;
        outs().flush();
        (colouredOut(llvm::raw_ostream::RED) << "Failed to convert match: ").writeEscaped(e.what());
        (outs() << "\n\n").flush();
    }
}

void ConnectCallMatcher::matchFound(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    outs() << "\nMatch " << ++foundMatches << " found inside function " << p.containingFunctionName << ": ";
    //outs() << ", num args: " << numArgs << ": ";
    //call->dumpPretty(*result.Context); //show result after macro expansion
    const std::string oldCall = expr2str(p.call, result.Context);
    colouredOut(llvm::raw_ostream::BLUE) << oldCall;
    outs() << " (at ";
    p.call->getExprLoc().print(outs(), *result.SourceManager);
    outs() << ")\n";

    //check if we should touch this file
    const std::string filename = getRealPath(result.SourceManager->getFilename(p.call->getExprLoc()));
    if (std::find(refactoringFiles.begin(), refactoringFiles.end(), filename) == refactoringFiles.end()) {
        throw std::runtime_error("Found match in file '" + filename + "' which is not one of the files to be refactored!");
    }
}

static void foundWrongCall(llvm::raw_string_ostream& out, const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    out.changeColor(llvm::raw_ostream::MAGENTA, true) << expr2str(p.call, result.Context);
    out.resetColor();
    out << " (at ";
    p.call->getExprLoc().print(out, *result.SourceManager);
    out << ")\n";
}

static void foundWithoutStringLiterals(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    std::string buf;
    llvm::raw_string_ostream out(buf);
    out << "Found QObject::" << p.decl->getName() << "() call that doesn't use SIGNAL()/SLOT() inside function "
            << p.containingFunctionName << ": ";
    foundWrongCall(out, p, result);
    throw SkipMatchException(out.str());
}

static void foundOtherOverload(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    if (!verboseMode) {
        return;
    }
    std::string buf;
    llvm::raw_string_ostream out(buf);
    out << "Found QObject::" << p.decl->getName() << "() overload without const char* parameters inside function "
            << p.containingFunctionName << ": ";
    foundWrongCall(out, p, result);
    out << "Called method was: " << p.decl->getQualifiedNameAsString() << "(";
    for (uint i = 0; i < p.decl->getNumParams(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << p.decl->getParamDecl(i)->getType().getAsString();
    }
    out << ")\n";
    throw SkipMatchException(out.str());
}


void ConnectCallMatcher::convert(const MatchFinder::MatchResult& result) {
    if (result.Context != lastAstContext) {
        //outs() << "\n\n\nAST context changed!\n\n\n";
        lastAstContext = result.Context;
        constCharPtrType = result.Context->getPointerType(result.Context->getConstType(result.Context->CharTy));
    }
    Parameters p;
    p.call = result.Nodes.getNodeAs<CallExpr>("callExpr");
    p.decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    p.containingFunction = result.Nodes.getNodeAs<FunctionDecl>("parent");
    p.containingFunctionName = p.containingFunction->getQualifiedNameAsString();
    if (p.decl->getName() == "connect") {
        return convertConnect(p, result);
    }
    else if (p.decl->getName() == "disconnect") {
        return convertDisconnect(p, result);
    }
    else {
        throw std::runtime_error("Bad method: " + p.decl->getNameAsString());
    }
}


static bool isSmartPointerOperatorArrow(const Expr* expr, std::initializer_list<const char*> classes, ASTContext* ctx) {
    auto opCall = dyn_cast<CXXOperatorCallExpr>(expr->IgnoreImplicit());
    if (!opCall) {
        return false;
    }
    //outs() << "operator=" << getOperatorSpelling(opCall->getOperator()) << ": " << opCall->getOperator() << "\n";
    // must be operator->()
    if (opCall->getOperator() != OO_Arrow) {
        return false;
    }
    if (auto recordDecl = dyn_cast_or_null<CXXRecordDecl>(opCall->getCalleeDecl()->getDeclContext())) {
        for (const char* className : classes) {
            if (isOrInheritsFrom(recordDecl, className)) {
                return true;
            }
        }
    }
    return false;
}

static inline bool isQtSmartPointerOperatorArrow(const Expr* expr, ASTContext* ctx) {
    return isSmartPointerOperatorArrow(expr, { "QPointer", "QScopedPointer", "QSharedPointer" }, ctx);
}

static inline bool isStlSmartPointerOperatorArrow(const Expr* expr, ASTContext* ctx) {
    // at least with libstdc++ shared_ptr operator-> is inside class __shared_ptr
    return isSmartPointerOperatorArrow(expr, { "unique_ptr", "shared_ptr", "__shared_ptr" }, ctx);
}

static inline bool isQPointerImplicitConversion(const Expr* expr, ASTContext* ctx) {
    if (const CXXMemberCallExpr* memberCall = dyn_cast<CXXMemberCallExpr>(expr->IgnoreImplicit())) {
        if (!isa<CXXConversionDecl>(memberCall->getCalleeDecl())) {
            return false;
        }
        //outs() << "conversion\n";
        if (auto recordDecl = dyn_cast_or_null<CXXRecordDecl>(memberCall->getCalleeDecl()->getDeclContext())) {
            if (isOrInheritsFrom(recordDecl, "QPointer")) {
                return true;
            }
        }
    }
    return false;
}

/** @return the string representation for the implicit receiver argument in connect/ sender argument in disconnect*/
static std::string membercallImplicitParameter(const Expr* thisArg, ASTContext* ctx) {
    const std::string stringRepresentation = thisArg->isImplicitCXXThis() ? "this" : expr2str(thisArg, ctx);
    // outs() << "\n\nType of " << stringRepresentation << " is " << thisArg->getType().getAsString() << "\n";
    // thisArg->dump(outs(), ctx->getSourceManager());
    // outs() << "\nlvalue: " << thisArg->isLValue() << " rvalue: " << thisArg->isRValue() << " xvalue: " << thisArg->isXValue()  << " glvalue: " << thisArg->isGLValue() << "\n";

    // we have to add an address-of operator if it is not a pointer, but instead a value or reference
    if (!thisArg->getType()->isPointerType()) {
        if (verboseMode) {
            outs() << stringRepresentation << " is not a pointer -> taking address.\n";
        }
        return "&" + stringRepresentation;
    }
    // we have to append .data() for QPointer arguments since the implicit conversion operator is somehow ignore with the new syntax
    if (isQtSmartPointerOperatorArrow(thisArg, ctx)) {
        return stringRepresentation + ".data()";
    }
    else if (isStlSmartPointerOperatorArrow(thisArg, ctx)) {
        //STL uses .get() instead of .data()
        return stringRepresentation + ".get()";
    }
    return stringRepresentation;

}

void ConnectCallMatcher::convertConnect(ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    // check whether this is the inline implementation of the QObject::connect member function
    // this would be caught as foundWithoutStringLiterals, but since this happens in every file skip it
    if (p.containingFunctionName == "QObject::connect") {
        return; // this can't be converted
    }
    // check that it is the correct overload with const char*
    const unsigned numArgs = p.decl->getNumParams();
    if (numArgs == 5 && p.decl->isStatic()) {
        // static method connect(QObject*, const char*, QObject*, const char*, Qt::ConnectionType)
        if (p.decl->getParamDecl(1)->getType() != constCharPtrType
                || p.decl->getParamDecl(3)->getType() != constCharPtrType) {
            foundOtherOverload(p, result);
            return;
        }
        p.sender = p.call->getArg(0);
        p.signal = p.call->getArg(1);
        p.receiver = p.call->getArg(2);
        p.slot = p.call->getArg(3);
        //connectionTypeExpr = call->getArg(4);

    }
    else if (numArgs == 4 && p.decl->isInstance()) {
        // instance method connect(QObject*, const char*, const char*, Qt::ConnectionType)
        if (p.decl->getParamDecl(1)->getType() != constCharPtrType
                || p.decl->getParamDecl(2)->getType() != constCharPtrType) {
            foundOtherOverload(p, result);
            return;
        }
        const CXXMemberCallExpr* cxxCall = cast<CXXMemberCallExpr>(p.call);
        p.sender = p.call->getArg(0);
        p.signal = p.call->getArg(1);
        p.slot = p.call->getArg(2);
        p.receiver = cxxCall->getImplicitObjectArgument(); //get this pointer
        //connectionTypeExpr = call->getArg(3);
        p.receiverString = membercallImplicitParameter(p.receiver, result.Context);
    }
    else {
        foundOtherOverload(p, result);
        return;
    }
    p.signalLiteral = findFirstChildWithType<StringLiteral>(p.signal);
    p.slotLiteral = findFirstChildWithType<StringLiteral>(p.slot);
    if (!p.signalLiteral || !p.slotLiteral) {
        // both signal and slot must be string literals (SIGNAL()/SLOT() expansion)
        foundWithoutStringLiterals(p, result);
        return;
    }

    matchFound(p, result);
    //get the start/end location for the SIGNAL/SLOT macros, since getLocStart() and getLocEnd() don't return the right result for expanded macros
    SourceRange signalRange = sourceRangeForStmt(p.signal, result.Context);
    SourceRange slotRange = sourceRangeForStmt(p.slot, result.Context);

    if (verboseMode) {
        (outs() << "signal literal: ").write_escaped(p.signalLiteral ? p.signalLiteral->getBytes() : "nullptr") << "\n";
        (outs() << "slot literal: ").write_escaped(p.slotLiteral ? p.slotLiteral->getBytes() : "nullptr") << "\n";
    }


    const CXXRecordDecl* senderTypeDecl = p.sender->getBestDynamicClassType();
    const CXXRecordDecl* receiverTypeDecl = p.receiver->getBestDynamicClassType();

    const std::string signalReplacement = calculateReplacementStr(senderTypeDecl, p.signalLiteral,
            p.senderString, p);
    const std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, p.slotLiteral,
            p.receiverString, p);
    addReplacement(signalRange, signalReplacement, result.Context);
    addReplacement(slotRange, slotReplacement, result.Context);
    tryRemovingMembercallArg(p, result);
    if (isQPointerImplicitConversion(p.sender, result.Context)) {
        outs() << "sender is qpointer\n";
        addReplacement(sourceRangeForStmt(p.sender, result.Context),
                expr2str(p.sender, result.Context) + ".data()", result.Context);
    }
    // instance method qpointer.data() is handled in membercallImplicitParameter()
    if (p.decl->isStatic() && isQPointerImplicitConversion(p.receiver, result.Context)) {
        outs() << "receiver is qpointer\n";
        addReplacement(sourceRangeForStmt(p.receiver, result.Context),
                expr2str(p.receiver, result.Context) + ".data()", result.Context);
    }
    convertedMatches++;
}

static std::string castSignalToMemberFunctionIfNullPtr(ConnectCallMatcher::Parameters& p, clang::ASTContext* ctx) {
    // QObject::disconnect(myObject, 0, receiver, &QObject::deleteLater); cannot be called (compiler error)
    // have to change it to QObject::disconnect(myObject, static_cast<void(QObject::*)()>(0), receiver, &QObject::deleteLater);
    // p.signal can be null in the 2 arg version of disconnect
    std::string signalString = p.signal ? expr2str(p.signal, ctx) : nullPtrString;
    if (!p.signal || isNullPointerConstant(p.signal, ctx)) {
        // need to explicitly cast null pointer since there is no qt overload for signal == null
        assert(p.slotLiteral);
        StringRef parameters = signalSlotParameters(p.slotLiteral->getString()); // TODO suggest correct parameters
        return "static_cast<void(" + getLeastQualifiedName(p.sender->getBestDynamicClassType(),
                p.containingFunction, p.call, verboseMode, ctx) + "::*)(" + parameters.str() + ")>(" + signalString + ")";
    }
    else {
        return signalString;
    }
}

void ConnectCallMatcher::convertDisconnect(ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    // check whether this is the inline implementation of the QObject::disconnect member function
    if (p.containingFunctionName == "QObject::disconnect") {
        return; // this can't be converted
    }
    const unsigned numArgs = p.decl->getNumParams();
    if (numArgs == 4 && p.decl->isStatic()) {
        // static method disconnect(QObject*, const char*, QObject*, const char*)
        if (p.decl->getParamDecl(1)->getType() != constCharPtrType
                || p.decl->getParamDecl(3)->getType() != constCharPtrType) {
            foundOtherOverload(p, result);
            return;
        }
        p.sender = p.call->getArg(0);
        p.signal = p.call->getArg(1);
        p.receiver = p.call->getArg(2);
        p.slot = p.call->getArg(3);
    }
    else if (p.decl->isInstance()) {
        const CXXMemberCallExpr* cxxCall = cast<CXXMemberCallExpr>(p.call);
        if (numArgs == 2) {
            if (p.decl->getParamDecl(1)->getType() != constCharPtrType) {
                foundOtherOverload(p, result);
                return;
            }
            // instance method disconnect(QObject* receiver, const char* method)
            p.receiver = p.call->getArg(0);
            p.slot = p.call->getArg(1);
        }
        else if (numArgs == 3) {
            // instance method disconnect(const char* signal, QObject* receiver, const char* method)
            if (p.decl->getParamDecl(0)->getType() != constCharPtrType
                    || p.decl->getParamDecl(2)->getType() != constCharPtrType) {
                foundOtherOverload(p, result);
                return;
            }
            p.signal = p.call->getArg(0);
            p.receiver = p.call->getArg(1);
            p.slot = p.call->getArg(2);
        }
        // sender is always the this argument
        p.sender = cxxCall->getImplicitObjectArgument();
        // expand to "this" if it is a call such as "disconnect(0, 0, 0)" or to "foo()" with "foo()->disconnect(0, 0, 0)"
        p.senderString = membercallImplicitParameter(p.sender, result.Context);
    }
    else {
        foundOtherOverload(p, result);
        return;
    }
    p.signalLiteral = findFirstChildWithType<StringLiteral>(p.signal);
    p.slotLiteral = findFirstChildWithType<StringLiteral>(p.slot);
    if (!p.signalLiteral && !p.slotLiteral) {
        // at least one of the parameters must be a string literal (SIGNAL()/SLOT() expansion)

        if ((!p.signal || !isNullPointerConstant(p.signal, result.Context)) && !isNullPointerConstant(p.slot, result.Context)) {
            // signal or slot is not a string literal and not a nullPointer -> strange call -> print message
            foundWithoutStringLiterals(p, result);
        }
        // otherwise both are null pointer constants -> no message since this is the normal case
        return;
    }
    matchFound(p, result);
    if (verboseMode) {
        (outs() << "signal literal: ").write_escaped(p.signalLiteral ? p.signalLiteral->getBytes() : "nullptr") << "\n";
        (outs() << "slot literal: ").write_escaped(p.slotLiteral ? p.slotLiteral->getBytes() : "nullptr") << "\n";
    }

    //get the start/end location for the SIGNAL/SLOT macros, since getLocStart() and getLocEnd() don't return the right result for expanded macros
    if (p.signalLiteral) {
        SourceRange signalRange = sourceRangeForStmt(p.signal, result.Context);
        const CXXRecordDecl* senderTypeDecl = p.sender->getBestDynamicClassType();
        std::string signalReplacement = calculateReplacementStr(senderTypeDecl, p.signalLiteral,
                p.senderString, p);
        addReplacement(signalRange, signalReplacement, result.Context);
    }
    else {
        //no signal literal argument -> 3 possibilities:
        // foo->disconnect(receiver, SLOT(func()))
        // foo->disconnect(0, receiver, SLOT(func()))
        // QObject::disconnect(foo, 0, receiver, SLOT(func()))
        //
        // they must all expand to QObject::disconnect(foo, 0, receiver, &Receiver::func)
        // doesn't compile yet with latest Qt, needs a patch

        // only the member calls need the sender parameter added, since it is already present with the static call
        SourceRange replacementRange;
        std::string replacement;
        if (p.decl->isInstance() && numArgs == 2) {
            // add "sender, 0, " before receiver
            SourceLocation receiverStart = sourceLocationBeforeStmt(p.receiver, result.Context);
            replacementRange = SourceRange(receiverStart, receiverStart);
            assert(!p.signal);
            assert(!p.receiver->isDefaultArgument()); // must be explicitly passed

            // we need to cast the null pointer to a pointer to member function because Qt has no overload for this case
            replacement = p.senderString + ", " + castSignalToMemberFunctionIfNullPtr(p, result.Context) + ", ";
        }
        else if (p.decl->isInstance() && numArgs == 3) {
            // add "sender, " before signal
            replacementRange = sourceRangeForStmt(p.signal, result.Context);

            assert(p.signal);
            replacement = p.senderString + ", ";
            assert(!p.signal->isDefaultArgument()); // we don't convert calls such as foo->disconnect()
            replacement += castSignalToMemberFunctionIfNullPtr(p, result.Context);
        }
        else if (p.decl->isStatic() && numArgs == 4) {
            // replace signal with a static_cast<void(QObject::*)()>(0) since the Qt API requires it
            replacementRange = sourceRangeForStmt(p.signal, result.Context);
            assert(p.signal);
            assert(!p.signal->isDefaultArgument()); // we don't convert calls such as foo->disconnect()
            replacement = castSignalToMemberFunctionIfNullPtr(p, result.Context);

        }
        else {
            throw std::logic_error("QObject::disconnect member function with wrong arguments!");

        }

        addReplacement(replacementRange, replacement, result.Context);
    }
    if (p.slotLiteral) {
        SourceRange slotRange = sourceRangeForStmt(p.slot, result.Context);
        const CXXRecordDecl* receiverTypeDecl = p.receiver->getBestDynamicClassType();
        const std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, p.slotLiteral,
                p.receiverString, p);
        addReplacement(slotRange, slotReplacement, result.Context);
    }
    // add the default arguments
    if (p.receiver->isDefaultArgument()) {
        // this means that we have to add two null arguments to after the signal since there are no
        // default arguments for the function pointer diconnect() overload
        assert(p.slot->isDefaultArgument());
        SourceLocation afterSignal = sourceLocationAfterStmt(p.signal, result.Context);
        addReplacement(SourceRange(afterSignal, afterSignal), ", " + nullPtrString + ", " + nullPtrString, result.Context);
    }
    else if (p.slot->isDefaultArgument()) {
        assert(!p.receiver->isDefaultArgument());
        // similar but this time only one null arg and is inserted after receiver instead of after signal
        SourceLocation afterReceiver = sourceLocationAfterStmt(p.receiver, result.Context);
        addReplacement(SourceRange(afterReceiver, afterReceiver), ", " + nullPtrString, result.Context);
    }

    if (isQPointerImplicitConversion(p.receiver, result.Context)) {
        //outs() << "receiver is qpointer\n";
        addReplacement(sourceRangeForStmt(p.receiver, result.Context),
                expr2str(p.receiver, result.Context) + ".data()", result.Context);
    }
    // instance method qpointer.data() is handled in membercallImplicitParameter()
    if (p.decl->isStatic() && isQPointerImplicitConversion(p.sender, result.Context)) {
        //outs() << "sender is qpointer\n";
        addReplacement(sourceRangeForStmt(p.sender, result.Context),
                expr2str(p.sender, result.Context) + ".data()", result.Context);
    }
    tryRemovingMembercallArg(p, result);
    convertedMatches++;
}


void ConnectCallMatcher::tryRemovingMembercallArg(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    if (!p.decl->isInstance()) {
        return; // only makes sense with member calls
    }
    const CXXMemberCallExpr* membercall = cast<CXXMemberCallExpr>(p.call);
    Expr* objArg = membercall->getImplicitObjectArgument();
    if (objArg->isImplicitCXXThis()) {
        return; // nothing to remove and we don't need QObject:: since this is a QObject subclass
    }
    std::string replacement;
    // check whether we need to prefix with QObject::
    if (const CXXMethodDecl* containingMethod = dyn_cast<CXXMethodDecl>(p.containingFunction)) {
        // no need for prefix if we are inside a QObject subclass method since it is automatically in scope
        if (!isOrInheritsFrom(containingMethod->getParent(), "QObject")) {
            replacement = "QObject::";
        }
    }
    else {
        // non-class method (e.g. main()) -> have to add QObject::
        replacement = "QObject::";
    }
    // remove 2 more chars if pointer ("->"), or only one otherwise (".")
    const int additionalCharsToRemove = objArg->getType()->isPointerType() ? 2 : 1;
    SourceRange range = sourceRangeForStmt(objArg, result.Context, additionalCharsToRemove);
    addReplacement(range, replacement, result.Context);
}

template<typename Output, typename Range, typename Callback, typename Separator>
static void join(Output* out, Range range, Separator separator, Callback callback) {
    bool first = true;
    for (auto it : range) {
        if (!first) {
            *out += separator;
        } else {
            first = false;
        }
        *out += callback(it);
    }
}

// from http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
static unsigned int levenshtein_distance(StringRef s1, StringRef s2) {
        const size_t len1 = s1.size(), len2 = s2.size();
        std::vector<unsigned int> col(len2+1), prevCol(len2+1);

        for (unsigned int i = 0; i < prevCol.size(); i++)
                prevCol[i] = i;
        for (unsigned int i = 0; i < len1; i++) {
                col[0] = i+1;
                for (unsigned int j = 0; j < len2; j++)
                        col[j+1] = std::min( std::min(prevCol[1 + j] + 1, col[j] + 1),
                                                                prevCol[j] + (s1[i]==s2[j] ? 0 : 1) );
                col.swap(prevCol);
        }
        return prevCol[len2];
}

/** Normalizes the signature (QMetaObject::normalizedSignature) from @p connectStr and
 * then finds the best matching method from @p methods based on the function signature.
 * @return an index into @p methods
 */
template<typename T, typename Getter>
static int findBestMatch(const StringLiteral* connectStr, const T& methods, Getter getter) {
    QByteArray normalizedSig = QMetaObject::normalizedSignature(connectStr->getString().data());
    StringRef expectedParams = signalSlotParameters(StringRef(normalizedSig.constData()));
    int bestMatch = 0;
    uint minDistance = std::numeric_limits<uint>::max();
    for (int i = 0; i < methods.size(); ++i) {
        FunctionDecl* decl = getter(methods, i);
        llvm::SmallString<64> paramsStr;
        bool first = true;
        for (ParmVarDecl* param : decl->params()) {
            if (first) {
                first = false;
            } else {
                paramsStr += ',';
            }
            paramsStr += QMetaObject::normalizedType(param->getType().getAsString().c_str()).constData();
        };
        uint distance = levenshtein_distance(expectedParams, paramsStr.str());
        // outs() << "Distance between '" << expectedParams << "' and '" << paramsStr.str() << "' is " << distance << "\n";
        if (distance <= minDistance) {
            bestMatch = i;
            minDistance = distance;
            // outs() << "Current best match is '" << paramsStr.str() << "', index = " << i << "\n";
        }
    }
    // outs() << "Final match is index = " << bestMatch << "\n";
    return bestMatch;
}


std::string ConnectCallMatcher::handleQ_PRIVATE_SLOT(const CXXRecordDecl* type, const StringLiteral* connectStr, const std::string& prepend, const ConnectCallMatcher::Parameters& p) {
    ReplacementType methodType;
    StringRef methodName = signalSlotName(connectStr, &methodType);
    struct ParamInfo {
        CXXMethodDecl* method;
        Stmt* body;
    };
    std::vector<ParamInfo> privateSlotInfo;
    for (auto method : type->methods()) {
        // getName() asserts with operators or constructors
        if (StringRef(method->getNameAsString()).startswith("__qt_private_slot_")) {
            // outs() << "Found private slot: " << method->getName() << " in " << type->getNameAsString() << "\n";
            assert(std::distance(method->decls_begin(), method->decls_end()) == 1);
            CXXRecordDecl* localClass = dyn_cast<CXXRecordDecl>(*method->decls_begin());
            assert(localClass);
            assert(std::distance(localClass->method_begin(), localClass->method_end()) == 1);
            CXXMethodDecl* privateSlotMethod = dyn_cast<CXXMethodDecl>(*localClass->method_begin());
            assert(privateSlotMethod && privateSlotMethod->isStatic());
            if (privateSlotMethod->getName() == methodName) {
                assert(method->hasBody());
                privateSlotInfo.push_back(ParamInfo { privateSlotMethod, method->getBody() });
            }

        }
    }
    if (privateSlotInfo.empty()) {
        return {};
    }
    uint chosenInfoIndex = 0;
    if (privateSlotInfo.size() > 1) {
        // find the parameters that differ the least from the SLOT() argument
        chosenInfoIndex = findBestMatch(connectStr, privateSlotInfo, [](const std::vector<ParamInfo>& v, int index) { return v[index].method; });
    }
    auto info = privateSlotInfo[chosenInfoIndex];
    // outs() << "Found private slot\n";
    SmallString<128> result;
    if (!prepend.empty()) {
        result = prepend + ", ";
    }
    result += "[&]("; // just capture everything
    join(&result, info.method->params(), ", ", [&](ParmVarDecl* param) {
        return (getLeastQualifiedName(param->getType(), p.containingFunction, p.call, verboseMode,
                &currentCompilerInstance->getASTContext()) + " " + param->getName()).str();
    });
    result += ") { ";
    // outs() << "stmt type: " << info.body->getStmtClassName() << " ";
    assert(std::distance(info.body->child_begin(), info.body->child_end()) == 2); // 2 statements
    // info.body->dump(outs(), currentCompilerInstance->getSourceManager());
    StringLiteral* literal = dyn_cast<StringLiteral>(*info.body->child_begin());
    assert(literal);
    if (!p.receiver->isImplicitCXXThis()) {
        result += expr2str(p.receiver, lastAstContext);
        result += "->";
    }
    result += literal->getString();
    result += "->";
    result += info.method->getName();
    result += "(";
    join(&result, info.method->params(), ", ", [](ParmVarDecl* param) {
        return param->getName();
    });
    result += "); }";
    // outs() << "PRIVATE RESULT: " << result << "\n";
    return result.str().str();
    return {};
}


std::string ConnectCallMatcher::calculateReplacementStr(const CXXRecordDecl* type,
        const StringLiteral* connectStr, const std::string& prepend, const ConnectCallMatcher::Parameters& p) {
    ReplacementType methodType;
    StringRef methodName = signalSlotName(connectStr, &methodType);
    DeclarationName lookupName(pp->getIdentifierInfo(methodName));
    // TODO: how to get a scope for lookup
    // looks like we have to fill the Scope* manually

    //outs() << "Looking up " << methodName << " in " << type->getQualifiedNameAsString() << "\n";
    LookupResult lookup(*sema, lookupName, sourceLocationBeforeStmt(p.call, &currentCompilerInstance->getASTContext()), Sema::LookupMemberName);
    lookup.suppressDiagnostics(); // don't output errors if the method could not be found!
    // setting inUnqualifiedLookup to true makes sure that base classes are searched too
    sema->LookupQualifiedName(lookup, const_cast<DeclContext*>(type->getPrimaryContext()), true);
    //outs() << "lr after lookup: ";
    //lookup.print(outs());
    std::vector<FunctionDecl*> results;
    auto addDeclToResultsIfFunction = [&](NamedDecl* found) {
        // if it has been brought to the local scope using a using declaration get the actual decl instead
        if (UsingShadowDecl* usingShadow = dyn_cast<UsingShadowDecl>(found)) {
            found = usingShadow->getTargetDecl();
        }
        if (auto decl = dyn_cast<FunctionDecl>(found)) {
            results.push_back(decl);
        } else {
            errs() << found->getQualifiedNameAsString() << " is not a function, but a " << found->getDeclKindName() << "\n";
        }
    };
    if (lookup.isSingleResult()) {
        addDeclToResultsIfFunction(lookup.getFoundDecl());
    } else if (lookup.isOverloadedResult() || lookup.isAmbiguous()) {
        // if (lookup.isAmbiguous()) {
        //     errs() << "AMBIGUOUS LOOKUP!\"n;
        // }
        for (NamedDecl* found : lookup) {
            addDeclToResultsIfFunction(found);
        }
    }
    if (results.empty()) {
        // check for Q_PRIVATE_SLOTS
        if (methodType == ReplaceSlot) {
            auto privateSlotResult = handleQ_PRIVATE_SLOT(type, connectStr, prepend, p);
            if (!privateSlotResult.empty()) {
                return privateSlotResult;
            }
            // not a private slot -> continue (skip or just convert)
        }

        if (!convertNotFound) {
            throw SkipMatchException(Twine("No method with name " + methodName + " found in " + type->getName()).str());
        }
        if (isOrInheritsFrom(type, "QQuickItem") && !convertQQuickItemNotFound) {
            throw SkipMatchException(Twine("No method with name " + methodName + " found in " + type->getName() + " (skipping because it is a QQuickItem)").str());
        }
    }
    //


    if (verboseMode) {
        outs() << "scanned " << type->getQualifiedNameAsString() << " for overloads of " << methodName << ": " << results.size() << " results\n";
    }
    SmallString<128> replacement;
    if (!prepend.empty()) {
        replacement = prepend + ", ";
    }
    std::string qualifiedName = getLeastQualifiedName(type, p.containingFunction, p.call, verboseMode, &currentCompilerInstance->getASTContext());

    if (results.size() > 1) {
        if (verboseMode) {
            outs() << type->getName() << "::" << methodName << " is a overloaded signal/slot. Found "
                    << results.size() << " overloads.\n";
        }
        int methodIndex = findBestMatch(connectStr, results, [](const std::vector<FunctionDecl*>& v, int index) { return v[index]; });
        FunctionDecl* chosenMethod = results[methodIndex];
        replacement += "static_cast<";
        replacement += getLeastQualifiedName(chosenMethod->getReturnType(), p.containingFunction, p.call, verboseMode, &currentCompilerInstance->getASTContext());
        replacement += "(";
        replacement += qualifiedName;
        replacement += "::*)(";
        if (chosenMethod->getNumParams() > 0) {
            join(&replacement, chosenMethod->params(), ',', [&](ParmVarDecl* param) {
                return getLeastQualifiedName(param->getType(), p.containingFunction, p.call, verboseMode, &currentCompilerInstance->getASTContext());
            });
        }
        replacement += ")>(";
    }
    replacement += '&';
    replacement += qualifiedName;
    replacement += "::";
    replacement += methodName;
    if (results.size() > 1) {
        replacement += ')';
    }
    return replacement.str().str();

}

void ConnectConverter::setupMatchers(MatchFinder* matchFinder) {
    //both connect overloads
    using namespace clang::ast_matchers;

    // !!!! when using asString it is very important to write "const char *" and not "const char*", since that doesn't work
    // handle both connect overloads with SIGNAL() and SLOT()
    matchFinder->addMatcher(id("callExpr", callExpr(
            hasDeclaration(id("decl", methodDecl(
                    anyOf(
                        hasName("::QObject::connect"),
                        hasName("::QObject::disconnect")
                    )
            ))),
            hasAncestor(id("parent", functionDecl()))
        )), &matcher);
}

bool ConnectCallMatcher::handleBeginSource(clang::CompilerInstance& CI, llvm::StringRef Filename) {
    outs() << "Handling file: " << Filename << "\n";
    currentCompilerInstance = &CI;
    Preprocessor& pp = currentCompilerInstance->getPreprocessor();
    pp.addPPCallbacks(llvm::make_unique<ConverterPPCallbacks>(pp));
    return true;
}

void ConnectCallMatcher::handleEndSource() {
    currentCompilerInstance = nullptr;
    sema = nullptr;
    pp = nullptr;
}

void ConnectCallMatcher::printStats() const {
    outs() << "Found " << foundMatches << " matches: ";
    colouredOut(llvm::raw_ostream::GREEN) << convertedMatches << " converted sucessfully";
    if (failedMatches > 0) {
        outs() << ", ";
        colouredOut(llvm::raw_ostream::RED) << failedMatches << " conversions failed";
    }
    if (skippedMatches > 0) {
        outs() << ", ";
        colouredOut(llvm::raw_ostream::YELLOW) << skippedMatches << " conversions skipped";
    }
    outs() << ".\n";
    outs().flush();
}
