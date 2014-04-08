#include "Qt5SignalSlotSyntaxConverter.h"
#include "PreProcessorCallback.h"
#include "llvmutils.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Analysis/CFG.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Sema/Sema.h>
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
using llvm::outs;
using llvm::errs;

static llvm::cl::OptionCategory clCategory("convert2qt5signalslot specific options");
static llvm::cl::opt<bool> verboseMode("verbose", llvm::cl::cat(clCategory), llvm::cl::desc("Enable verbose output"));
static llvm::cl::list<std::string> skipPrefixes("skip-prefix", llvm::cl::cat(clCategory), llvm::cl::ZeroOrMore,
        llvm::cl::desc("signals/slots with this prefix will be skipped (useful for Q_PRIVATE_SLOTS). May be passed multiple times.") );

//http://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
static std::string expr2str(const Expr *d, ASTContext* ctx) {
    clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, ctx->getSourceManager(), ctx->getLangOpts()));
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

/** expr->getLocStart() + expr->getLocEnd() don't return the proper location if the expression is a macro expansion
 * this function fixes this
 */
static SourceRange getMacroExpansionRange(const Expr* expr, ASTContext* context, bool* validMacroExpansion = nullptr) {
    SourceLocation beginLoc;
    const bool isMacroStart = Lexer::isAtStartOfMacroExpansion(expr->getLocStart(), context->getSourceManager(), context->getLangOpts(), &beginLoc);
    if (!isMacroStart)
        beginLoc = expr->getLocStart();
    SourceLocation endLoc;
    const bool isMacroEnd = Lexer::isAtEndOfMacroExpansion(expr->getLocEnd(), context->getSourceManager(), context->getLangOpts(), &endLoc);
    if (!isMacroEnd)
        endLoc = expr->getLocEnd();
    if (validMacroExpansion)
        *validMacroExpansion = isMacroStart && isMacroEnd;
    return SourceRange(beginLoc, endLoc);
}

static void printReplacementRange(SourceRange range, ASTContext* ctx, const std::string& replacement) {
    outs() << "Replacing '";
    colouredOut(llvm::raw_ostream::SAVEDCOLOR, true) << Lexer::getSourceText(CharSourceRange::getTokenRange(range), ctx->getSourceManager(), ctx->getLangOpts());
    outs() << "' with '";
    colouredOut(llvm::raw_ostream::SAVEDCOLOR, true) << replacement;

    outs() << "' (from ";
    range.getBegin().print(outs(), ctx->getSourceManager());
    outs() << " to ";
    range.getEnd().print(outs(), ctx->getSourceManager());
    outs() << ")\n";
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
        assert(isa<CXXMemberCallExpr>(p.call)); // instance method -> must be member call expression
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
    SourceRange signalRange = getMacroExpansionRange(p.signal, result.Context);
    SourceRange slotRange = getMacroExpansionRange(p.slot, result.Context);

    if (verboseMode) {
        (outs() << "signal literal: ").write_escaped(p.signalLiteral ? p.signalLiteral->getBytes() : "nullptr") << "\n";
        (outs() << "slot literal: ").write_escaped(p.slotLiteral ? p.slotLiteral->getBytes() : "nullptr") << "\n";
    }


    const CXXRecordDecl* senderTypeDecl = p.sender->getBestDynamicClassType();
    const CXXRecordDecl* receiverTypeDecl = p.receiver->getBestDynamicClassType();

    const std::string signalReplacement = calculateReplacementStr(senderTypeDecl, p.signalLiteral, p.senderString);
    const std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, p.slotLiteral, p.receiverString);
    printReplacementRange(signalRange, result.Context, signalReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(signalRange), signalReplacement));
    printReplacementRange(slotRange, result.Context, slotReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(slotRange), slotReplacement));
    convertedMatches++;
}

void ConnectCallMatcher::convertDisconnect(ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    // check whether this is the inline implementation of the QObject::disconnect member function
    if (p.containingFunctionName == "QObject::disconnect") {
        return; // this can't be converted
    }
    std::string nullArgsAfterCall; // in case there were default arguments, we have to add these to the end of the call
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
        assert(isa<CXXMemberCallExpr>(p.call)); // instance method -> must be a member call expression
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
        // TODO: allow nullptr/0/NULL/Q_NULLPTR
        // there are no default arguments for the pointer-to-memberfunction syntax -> add null if it is a default argument
        if (p.receiver->isDefaultArgument()) {
            nullArgsAfterCall += ", 0";
        }
        if (p.slot->isDefaultArgument()) {
            nullArgsAfterCall += ", 0";
        }
    }
    else {
        foundOtherOverload(p, result);
        return;
    }
    p.signalLiteral = findFirstChildWithType<StringLiteral>(p.signal);
    p.slotLiteral = findFirstChildWithType<StringLiteral>(p.slot);
    if (!p.signalLiteral && !p.slotLiteral) {
        // at least one of the parameters must be a string literal (SIGNAL()/SLOT() expansion)

        if ((p.signal && isNullPointerConstant(p.signal, result.Context)) || isNullPointerConstant(p.slot, result.Context)) {
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
        SourceRange signalRange = getMacroExpansionRange(p.signal, result.Context);
        const CXXRecordDecl* senderTypeDecl = p.sender->getBestDynamicClassType();
        std::string signalReplacement = calculateReplacementStr(senderTypeDecl, p.signalLiteral, p.senderString) + nullArgsAfterCall;
        if (!nullArgsAfterCall.empty()) {
            // something is seriously wrong if there is a slot literal, but we have default parameters to append!
            // this must never happen
            assert(!p.slotLiteral);
        }
        printReplacementRange(signalRange, result.Context, signalReplacement);
        replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(signalRange), signalReplacement));
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
            // disconnect(QObject* receiver, const char* method) ->  replace "receiver" with "sender, 0, receiver"
            replacementRange = getMacroExpansionRange(p.receiver, result.Context);
            assert(!p.signal);
            assert(!p.receiver->isDefaultArgument()); // must be explicitly passed
            // TODO: customize nullptr
            replacement = p.senderString + ", 0, " + expr2str(p.receiver, result.Context);

        }
        else if (numArgs == 3) {
            // disconnect(const char* signal, QObject* receiver, const char* method) -> replace "signal" with "sender, signal"
            assert(p.signal);
            replacementRange = getMacroExpansionRange(p.signal, result.Context);
            // TODO: customize nullptr
            const std::string signalString = p.signal->isDefaultArgument() ? "0" : expr2str(p.signal, result.Context);
            replacement = p.senderString + ", " + signalString;
        }
        else {
            throw std::logic_error("QObject::disconnect member function with neither 2 nor 3 args found!");
        }
        printReplacementRange(replacementRange, result.Context, replacement);
        replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(replacementRange), replacement));
    }
    if (p.slotLiteral) {
        SourceRange slotRange = getMacroExpansionRange(p.slot, result.Context);
        const CXXRecordDecl* receiverTypeDecl = p.receiver->getBestDynamicClassType();
        const std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, p.slotLiteral, p.receiverString);
        printReplacementRange(slotRange, result.Context, slotReplacement);
        replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(slotRange), slotReplacement));
    }
    convertedMatches++;
}


std::string ConnectCallMatcher::calculateReplacementStr(const CXXRecordDecl* type,
        const StringLiteral* connectStr, const std::string& prepend) {
    struct SearchInfo {
        std::vector<CXXMethodDecl*> results;
        StringRef methodName;
        StringRef parameters;
        clang::Sema* sema;
    } searchInfo;
    searchInfo.methodName = signalSlotName(connectStr);
    searchInfo.sema = sema;
    for (const auto& prefix : skipPrefixes) {
        if (searchInfo.methodName.startswith(prefix)) {
            //e.g. in KDE all private slots start with _k_
            //I don't know how to convert Q_PRIVATE_SLOTS, so allow an easy way of skipping them
            throw SkipMatchException(("'" + searchInfo.methodName + "' starts with '" + prefix + "'").str());
        }
    }
    auto searchLambda = [](const CXXRecordDecl* cls, void *userData) {
        auto info = static_cast<SearchInfo*>(userData);
        for (auto it = cls->method_begin(); it != cls->method_end(); ++it) {
            if (!it->getIdentifier()) {
                //is not a normal C++ method, maybe constructor or destructor
                continue;
            }
            if (it->getName() == info->methodName) {
                if (!info->results.empty()) {
                    //make sure we don't add overrides
                    FunctionDecl* d1 = info->results[0];
                    const bool overload = info->sema->IsOverload(*it, d1, false);
                    if (verboseMode) {
                        outs() << (*it)->getQualifiedNameAsString() << " is an overload of " << d1->getQualifiedNameAsString() << " = " << overload << "\n";
                    }
                    if (overload) {
                        if (verboseMode) {
                            outs() << "Found match: " << (*it)->getQualifiedNameAsString() << "\n";
                        }
                        info->results.push_back(*it);
                    }
                }
                else {
                    if (verboseMode) {
                        outs() << "Found match: " << (*it)->getQualifiedNameAsString() << "\n";
                    }
                    info->results.push_back(*it);
                }
//                outs() << "Found " << searchInfo->results.size() << ". defintion: "
//                        << cls->getName() << "::" << it->getName() << "\n";
            }
        }
        return true;
    };
    searchLambda(type, &searchInfo); //search baseClass
    type->forallBases(searchLambda, &searchInfo, false); //now find in base classes
    if (verboseMode) {
        outs() << "scanned " << type->getQualifiedNameAsString() << " for overloads of " << searchInfo.methodName << ": " << searchInfo.results.size() << " results\n";
    }
    SmallString<128> result;
    if (!prepend.empty()) {
        result = prepend + ", ";
    }
    //TODO abort if none found
    const bool resolveOverloads = searchInfo.results.size() > 1; //TODO skip overriden methods!
    if (resolveOverloads) {
        if (verboseMode) {
            outs() << type->getName() << "::" << searchInfo.methodName << " is a overloaded signal/slot. Found "
                    << searchInfo.results.size() << " overloads.\n";
        }
        //overloaded signal/slot found -> we have to disambiguate
        searchInfo.parameters = signalSlotParameters(connectStr);
        result += "static_cast<void("; //TODO return type
        result += type->getName();
        result += "::*)(";
        result += searchInfo.parameters;
        result += ")>(";
    }
    result += '&';
    result += type->getQualifiedNameAsString();
    result += "::";
    result += searchInfo.methodName;
    if (resolveOverloads) {
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
