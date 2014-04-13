#include "Qt5SignalSlotSyntaxConverter.h"
#include "PreProcessorCallback.h"
#include "ClangUtils.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Analysis/CFG.h>
#include <clang/AST/ASTContext.h>
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


using namespace clang;
using namespace clang::tooling;
using namespace ClangUtils;
using llvm::outs;
using llvm::errs;

static llvm::cl::OptionCategory clCategory("convert2qt5signalslot specific options");
static llvm::cl::opt<bool> verboseMode("verbose", llvm::cl::cat(clCategory),
        llvm::cl::desc("Enable verbose output"), llvm::cl::init(false));
// not static so that it can be modified by the tests
llvm::cl::opt<std::string> nullPtrString("nullptr", llvm::cl::init("nullptr"), llvm::cl::cat(clCategory),
        llvm::cl::desc("the string that will be used for a null pointer constant (Default is 'nullptr')"));
static llvm::cl::list<std::string> skipPrefixes("skip-prefix", llvm::cl::cat(clCategory), llvm::cl::ZeroOrMore,
        llvm::cl::desc("signals/slots with this prefix will be skipped (useful for Q_PRIVATE_SLOTS). May be passed multiple times.") );

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
static StringRef signalSlotName(const StringLiteral* literal) {
    StringRef bytes = literal->getString();
    size_t bracePos = bytes.find_first_of('(');
    if (!(bytes[0] == '0' || bytes[0] == '1' || bytes[0] == '2') || bracePos == StringRef::npos) {
        throw std::runtime_error(("invalid format of signal slot string: " + bytes).str());
    }
    return bytes.substr(1, bracePos - 1);
}

static StringRef signalSlotParameters(const StringLiteral* literal) {
    StringRef bytes = literal->getString();
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

static void foundWrongCall(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    colouredOut(llvm::raw_ostream::MAGENTA) << expr2str(p.call, result.Context);
    outs() << " (at ";
    p.call->getExprLoc().print(outs(), *result.SourceManager);
    outs() << ")\n";
}

static void foundWithoutStringLiterals(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    outs() << "Found QObject::" << p.decl->getName() << "() call that doesn't use SIGNAL()/SLOT() inside function "
            << p.containingFunctionName << ": ";
    foundWrongCall(p, result);
}

static void foundOtherOverload(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    if (!verboseMode) {
        return;
    }
    outs() << "Found QObject::" << p.decl->getName() << "() overload without const char* parameters inside function "
            << p.containingFunctionName << ": ";
    foundWrongCall(p, result);
    outs() << "Called method was: " << p.decl->getQualifiedNameAsString() << "(";
    for (uint i = 0; i < p.decl->getNumParams(); ++i) {
        if (i != 0) {
            outs() << ", ";
        }
        outs() << p.decl->getParamDecl(i)->getType().getAsString();
    }
    outs() << ")\n";
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
    convertedMatches++;
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
    else if (p.decl->isInstance()) {
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
        if (numArgs == 2) {
            // add "sender, 0, " before receiver
            SourceLocation receiverStart = sourceLocationBeforeStmt(p.receiver, result.Context);
            replacementRange = SourceRange(receiverStart, receiverStart);
            assert(!p.signal);
            assert(!p.receiver->isDefaultArgument()); // must be explicitly passed
            replacement = p.senderString + ", " + nullPtrString + ", ";

        }
        else if (numArgs == 3) {
            // add "sender, " before signal
            SourceLocation signalStart = sourceLocationBeforeStmt(p.signal, result.Context);
            replacementRange = SourceRange(signalStart, signalStart);
            assert(p.signal);
            replacement = p.senderString + ", ";
            assert(!p.signal->isDefaultArgument()); // we don't convert calls such as foo->disconnect()
        }
        else {
            throw std::logic_error("QObject::disconnect member function with neither 2 nor 3 args found!");
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
        if (!inheritsFrom(containingMethod->getParent(), "class QObject")) {
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

std::string ConnectCallMatcher::calculateReplacementStr(const CXXRecordDecl* type,
        const StringLiteral* connectStr, const std::string& prepend, const ConnectCallMatcher::Parameters& p) {

    StringRef methodName = signalSlotName(connectStr);
    // TODO: handle Q_PRIVATE_SLOTS
    for (const auto& prefix : skipPrefixes) {
        if (methodName.startswith(prefix)) {
            //e.g. in KDE all private slots start with _k_
            //I don't know how to convert Q_PRIVATE_SLOTS, so allow an easy way of skipping them
            throw SkipMatchException(("'" + methodName + "' starts with '" + prefix + "'").str());
        }
    }
    DeclarationName lookupName(pp->getIdentifierInfo(methodName));


    // this doesn't work
    // TODO: how to get a scope for lookup
    //Scope* scope = sema->getScopeForContext();


    //outs() << "Looking up " << methodName << " in " << type->getQualifiedNameAsString() << "\n";
    LookupResult lookup(*sema, lookupName, sourceLocationBeforeStmt(p.call, &currentCompilerInstance->getASTContext()), Sema::LookupMemberName);
    // setting inUnqualifiedLookup to true makes sure that base classes are searched too
    sema->LookupQualifiedName(lookup, const_cast<DeclContext*>(type->getPrimaryContext()), true);
    //outs() << "lr after lookup: ";
    //lookup.print(outs());
    const uint numFound = [&]() -> uint {
        if (lookup.isSingleResult()) {
            //NamedDecl* result = lookup.getFoundDecl();
            //outs() << "SINGLE RESULT!\n";
            return 1;
        }
        else if (lookup.isOverloadedResult()) {
            //outs() << "OVERLOADED!\n";
            uint i = 0;
            for (NamedDecl* decl : lookup) {
                //outs() << decl->getQualifiedNameAsString() << ": " << decl->getDeclKindName() << "\n";
                (void)decl;
                i++;
            }
            return i;
        }
        else if (lookup.isAmbiguous()) {
            //outs() << "AMBIGUOUS!\n";
            uint i = 0;
            for (NamedDecl* decl : lookup) {
                //outs() << decl->getQualifiedNameAsString() << ": " << decl->getDeclKindName() << "\n";
                (void)decl;
                i++;
            }
            return i;
        }
        else if (lookup.empty()) {
            //TODO warning
            //outs() << "NOT FOUND!\n";
            return 0;
        }
        else {
            //TODO warning
            //outs() << "SOME OTHER CASE!\n";
            return 0;
        }
    }();

    std::string qualifiedName = getLeastQualifiedName(type, p.containingFunction, p.call, verboseMode);


    if (verboseMode) {
        outs() << "scanned " << type->getQualifiedNameAsString() << " for overloads of " << methodName << ": " << numFound << " results\n";
    }
    SmallString<128> result;
    if (!prepend.empty()) {
        result = prepend + ", ";
    }
    if (numFound > 1) {
        if (verboseMode) {
            outs() << type->getName() << "::" << methodName << " is a overloaded signal/slot. Found "
                    << numFound << " overloads.\n";
        }
        //overloaded signal/slot found -> we have to disambiguate
        StringRef parameters = signalSlotParameters(connectStr); // TODO suggest correct parameters
        result += "static_cast<void("; //TODO return type
        result += qualifiedName;
        result += "::*)(";
        result += parameters;
        result += ")>(";
    }
    result += '&';
    result += qualifiedName;
    result += "::";
    result += methodName;
    if (numFound > 1) {
        result += ')';
    }
    return result.str();

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
    pp.addPPCallbacks(new ConverterPPCallbacks(pp));
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
