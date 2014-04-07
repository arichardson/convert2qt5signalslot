#include "Qt5SignalSlotSyntaxConverter.h"

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

static llvm::cl::OptionCategory clCategory("convert2qt5signalslot specific options");
static llvm::cl::list<std::string> skipPrefixes("skip-prefix", llvm::cl::cat(clCategory), llvm::cl::ZeroOrMore,
        llvm::cl::desc("signals/slots with this prefix will be skipped (useful for Q_PRIVATE_SLOTS). May be passed multiple times.") );

//http://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
static std::string expr2str(const Expr *d, ASTContext* ctx) {
    clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, ctx->getSourceManager(), ctx->getLangOpts()));
    const char* start = ctx->getSourceManager().getCharacterData(b);
    const char* end = ctx->getSourceManager().getCharacterData(e);
    if (!start || !end || end < start) {
        llvm::errs() << "could not get string for ";
        d->dumpPretty(*ctx);
        llvm::errs() << " at " << b.printToString(ctx->getSourceManager()) << "\nAST:\n";
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
    //((llvm::outs() << "Parameters from '").write_escaped(bytes) << "' are '").write_escaped(result) << "'\n";
    return result;
}

/** expr->getLocStart() + expr->getLocEnd() don't return the proper location if the expression is a macro expansion
 * this function fixes this
 */
static SourceRange getMacroExpansionRange(const Expr* expr, ASTContext* context, bool* validMacroExpansion) {
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

static void printReplacementRange(SourceRange range, SourceManager* manager, const std::string& replacement) {
    llvm::outs() << "Replacing ";
    range.getBegin().print(llvm::outs(), *manager);
    llvm::outs() << " to ";
    range.getEnd().print(llvm::outs(), *manager);
    llvm::outs() << " with '" << replacement << "'\n";
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
        (llvm::errs().changeColor(llvm::raw_ostream::BLUE, true) << "Not converting current match. Reason: ")
                .write_escaped(e.what()) << "\n\n";
        llvm::errs().resetColor();
    }
    catch (const std::exception& e) {
        failedMatches++;
        (llvm::errs().changeColor(llvm::raw_ostream::RED, true) << "Failed to convert match: ")
                .write_escaped(e.what()) << "\n\n";
        llvm::errs().resetColor();
    }
    llvm::outs().flush();
    llvm::errs().flush();
}

/** find the first StringLiteral child of @p stmt or null if not found */
static const StringLiteral* findStringLiteralChild(const Stmt* stmt) {
    for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
        if (isa<StringLiteral>(*it)) {
            return cast<StringLiteral>(*it);
        }
        const StringLiteral* ret = findStringLiteralChild(*it);
        if (ret) {
            return ret;
        }
    }
    return nullptr;
}

void ConnectCallMatcher::matchFound(const ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    llvm::outs() << "\nMatch " << ++foundMatches << " found at ";
    p.call->getExprLoc().print(llvm::outs(), *result.SourceManager);

    llvm::outs() << " inside function " << p.containingFunctionName;
    //llvm::outs() << ", num args: " << numArgs << ": ";
    //call->dumpPretty(*result.Context); //show result after macro expansion

    const std::string oldCall = expr2str(p.call, result.Context);
    ((llvm::outs() << ": ").changeColor(llvm::raw_ostream::BLUE) << oldCall).resetColor() << "\n";

    //check if we should touch this file
    const std::string filename = getRealPath(result.SourceManager->getFilename(p.call->getExprLoc()));
    if (std::find(refactoringFiles.begin(), refactoringFiles.end(), filename) == refactoringFiles.end()) {
        throw std::runtime_error("Found match in file '" + filename + "' which is not one of the files to be refactored!");
    }
}

void ConnectCallMatcher::convert(const MatchFinder::MatchResult& result) {
    Parameters p;
    p.call = result.Nodes.getNodeAs<CallExpr>("callExpr");
    p.decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    p.containingFunction = result.Nodes.getNodeAs<FunctionDecl>("parent");
    p.containingFunctionName = p.containingFunction->getQualifiedNameAsString();
    if (p.decl->getName() == "connect") {
        // check whether this is the inline implementation of the QObject::connect member function
        if (p.containingFunctionName == "QObject::connect") {
            return; // this can't be converted
        }
        matchFound(p, result);
        return convertConnect(p, result);
    }
    else if (p.decl->getName() == "disconnect") {
        // check whether this is the inline implementation of the QObject::disconnect member function
        if (p.containingFunctionName == "QObject::disconnect") {
            return; // this can't be converted
        }
        matchFound(p, result);
        return convertDisconnect(p, result);
    }
    else {
        throw std::runtime_error("Bad method: " + p.decl->getNameAsString());
    }
}

void ConnectCallMatcher::convertConnect(ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    const unsigned numArgs = p.decl->getNumParams();
    p.sender = p.call->getArg(0);
    if (numArgs == 5) {
        //static QObject::connect
        p.signal = p.call->getArg(1);
        p.receiver = p.call->getArg(2);
        //receiverString = expr2str(receiver, result.SourceManager, result.Context);
        p.slot = p.call->getArg(3);
        //connectionTypeExpr = call->getArg(4);

    }
    else if (numArgs == 4) {
        p.signal = p.call->getArg(1);
        p.slot = p.call->getArg(2);
        assert(isa<CXXMemberCallExpr>(p.call));
        const CXXMemberCallExpr* cxxCall = cast<CXXMemberCallExpr>(p.call);
        p.receiver = cxxCall->getImplicitObjectArgument(); //get this pointer
        //connectionTypeExpr = call->getArg(3);
        if (p.receiver->isImplicitCXXThis()) {
            p.receiverString = "this";
        }
        else {
            // this expands to foo() in foo()->connect(...)
            p.receiverString = expr2str(p.receiver, result.Context);
        }
    }
    else {
        throw std::runtime_error("Bad number of args: " + std::to_string(numArgs));
    }

    //get the start/end location for the SIGNAL/SLOT macros, since getLocStart() and getLocEnd() don't return the right result for expanded macros
    bool signalRangeOk;
    SourceRange signalRange = getMacroExpansionRange(p.signal, result.Context, &signalRangeOk);
    bool slotRangeOk;
    SourceRange slotRange = getMacroExpansionRange(p.slot, result.Context, &slotRangeOk);
    // doesn't have to be a macro expansion
    //if (!signalRangeOk || !slotRangeOk) {
    //    throw std::runtime_error("connect() call must use SIGNAL/SLOT macro so that conversion can work!\n");
    //}

    const StringLiteral* signalLiteral = findStringLiteralChild(p.signal);
    const StringLiteral* slotLiteral = findStringLiteralChild(p.slot);
    llvm::outs() << "signal literal: " << (signalLiteral ? signalLiteral->getBytes() : "nullptr") << "\n";
    llvm::outs() << "slot literal: " << (slotLiteral ? slotLiteral->getBytes() : "nullptr") << "\n";


    const CXXRecordDecl* senderTypeDecl = p.sender->getBestDynamicClassType();
    const CXXRecordDecl* receiverTypeDecl = p.receiver->getBestDynamicClassType();

    const std::string signalReplacement = calculateReplacementStr(senderTypeDecl, signalLiteral, p.senderString);
    const std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, slotLiteral, p.receiverString);
    printReplacementRange(signalRange, result.SourceManager, signalReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(signalRange), signalReplacement));
    printReplacementRange(slotRange, result.SourceManager, slotReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(slotRange), slotReplacement));
    convertedMatches++;
}

void ConnectCallMatcher::convertDisconnect(ConnectCallMatcher::Parameters& p, const MatchFinder::MatchResult& result) {
    //TODO: implement
    llvm::errs() << "converting disconnect not implemented!\n";
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
                    llvm::outs() << (*it)->getQualifiedNameAsString() << " is an overload of " << d1->getQualifiedNameAsString() << " = " << overload << "\n";
                    if (overload) {
                        llvm::outs() << "Found match: " << (*it)->getQualifiedNameAsString() << "\n";
                        info->results.push_back(*it);
                    }
                }
                else {
                    llvm::outs() << "Found match: " << (*it)->getQualifiedNameAsString() << "\n";
                    info->results.push_back(*it);
                }
//                llvm::outs() << "Found " << searchInfo->results.size() << ". defintion: "
//                        << cls->getName() << "::" << it->getName() << "\n";
            }
        }
        return true;
    };
    searchLambda(type, &searchInfo); //search baseClass
    type->forallBases(searchLambda, &searchInfo, false); //now find in base classes
    llvm::outs() << "scanned " << type->getQualifiedNameAsString() << " for overloads of " << searchInfo.methodName << ": " << searchInfo.results.size() << " results\n";
    SmallString<128> result;
    if (!prepend.empty()) {
        result = prepend + ", ";
    }
    //TODO abort if none found
    const bool resolveOverloads = searchInfo.results.size() > 1; //TODO skip overriden methods!
    if (resolveOverloads) {
        llvm::outs() << type->getName() << "::" << searchInfo.methodName << " is a overloaded signal/slot. Found "
                << searchInfo.results.size() << " overloads.\n";
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
    matchFinder->addMatcher(
        id("callExpr", callExpr(
            hasDeclaration(id("decl", methodDecl(
                hasName("::QObject::connect"),
                    anyOf(
                        // QMetaObject::Connection connect(const QObject* sender, const char* signal, const char* method, Qt::ConnectionType type = Qt::AutoConnection) const
                        allOf(parameterCountIs(4),
                                hasParameter(1, hasType(asString("const char *"))),
                                hasParameter(2, hasType(asString("const char *")))
                            ),
                        // static QMetaObject::Connection connect(const QObject* sender, const char* signal, const QObject* receiver, const char* method, Qt::ConnectionType type = Qt::AutoConnection)
                        allOf(parameterCountIs(5),
                                hasParameter(1, hasType(asString("const char *"))),
                                hasParameter(3, hasType(asString("const char *")))
                            )
                        )
            ))),
            hasAncestor(id("parent", methodDecl())) // we need this to determine whether we are inside QObject::connect
        )), &matcher);
}

bool ConnectCallMatcher::handleBeginSource(clang::CompilerInstance& CI, llvm::StringRef Filename) {
    llvm::outs() << "Handling file: " << Filename << "\n";
    currentCompilerInstance = &CI;
    return true;
}

void ConnectCallMatcher::handleEndSource() {
    currentCompilerInstance = nullptr;
    sema = nullptr;
    pp = nullptr;
}

void ConnectCallMatcher::printStats() const {
    (llvm::outs() << "Found " << foundMatches << " matches: ").changeColor(llvm::raw_ostream::GREEN) << convertedMatches << " converted sucessfully";
    if (failedMatches > 0) {
        (llvm::outs().resetColor() << ", ").changeColor(llvm::raw_ostream::RED) << failedMatches << " conversions failed";
    }
    if (skippedMatches > 0) {
        (llvm::outs().resetColor() << ", ").changeColor(llvm::raw_ostream::YELLOW) << skippedMatches << " conversions skipped";
    }
    llvm::outs().resetColor() << ".\n";
    llvm::outs().flush();
}
