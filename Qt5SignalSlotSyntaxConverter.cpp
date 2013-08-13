#include "Qt5SignalSlotSyntaxConverter.h"

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Analysis/CFG.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ADT/SmallString.h>

#include <atomic>
#include <stdexcept>


using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory clCategory("convert2qt5signalslot specific options");
static llvm::cl::list<std::string> skipPrefixes("skip-prefix", llvm::cl::cat(clCategory), llvm::cl::ZeroOrMore,
        llvm::cl::desc("signals/slots with this prefix will be skipped (useful for Q_PRIVATE_SLOTS). May be passed multiple times.") );

//http://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
static std::string expr2str(const Expr *d, SourceManager* manager, ASTContext* ctx) {
    clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, *manager, ctx->getLangOpts()));
    const char* start = manager->getCharacterData(b);
    const char* end = manager->getCharacterData(e);
    if (!start || !end || end < start) {
        llvm::errs() << "could not get string for ";
        d->dumpPretty(*ctx);
        llvm::errs() << " at " << b.printToString(*manager) << "\nAST:\n";
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
static SourceRange getMacroExpansionRange(const Expr* expr,SourceManager* manager, const LangOptions& options, bool* validMacroExpansion) {
    SourceLocation beginLoc;
    const bool isMacroStart = Lexer::isAtStartOfMacroExpansion(expr->getLocStart(), *manager, options, &beginLoc);
    if (!isMacroStart)
        beginLoc = expr->getLocStart();
    SourceLocation endLoc;
    const bool isMacroEnd = Lexer::isAtEndOfMacroExpansion(expr->getLocEnd(), *manager, options, &endLoc);
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
    virtual ~SkipMatchException();
};

SkipMatchException::~SkipMatchException() {}


void ConnectCallMatcher::run(const MatchFinder::MatchResult& result) {
    try {
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


void ConnectCallMatcher::convert(const MatchFinder::MatchResult& result) {
    const CallExpr* call = result.Nodes.getNodeAs<CallExpr>("callExpr");
    const CXXMethodDecl* decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    llvm::outs() << "\nMatch " << ++foundMatches << " found at ";
    call->getExprLoc().print(llvm::outs(), *result.SourceManager);
    unsigned numArgs = decl->getNumParams();
    llvm::outs() << ", num args: " << numArgs << ": ";

    //call->dumpPretty(*result.Context); //show result after macro expansion

    const std::string oldCall = expr2str(call, result.SourceManager, result.Context);
    llvm::outs() << oldCall << "\n";

    //check if we should touch this file
    const std::string filename = getRealPath(result.SourceManager->getFilename(call->getExprLoc()));
    if (std::find(refactoringFiles.begin(), refactoringFiles.end(), filename) == refactoringFiles.end()) {
        throw std::runtime_error("Found match in file '" + filename + "' which is not one of the files to be refactored!");
    }

    const Expr* sender = call->getArg(0);
    const Expr* signal;
    const Expr* slot;
    const Expr* receiver;
    const Expr* connectionTypeExpr;
    std::string receiverString;

    if (numArgs == 5) {
        //static QObject::connect
        signal = call->getArg(1);
        receiver = call->getArg(2);
        //receiverString = expr2str(receiver, result.SourceManager, result.Context);
        slot = call->getArg(3);
        connectionTypeExpr = call->getArg(4);

    }
    else if (numArgs == 4) {
        signal = call->getArg(1);
        slot = call->getArg(2);
        assert(isa<CXXMemberCallExpr>(call));
        const CXXMemberCallExpr* cxxCall = cast<CXXMemberCallExpr>(call);
        receiver = cxxCall->getImplicitObjectArgument(); //get this pointer
        connectionTypeExpr = call->getArg(3);
        if (receiver->isImplicitCXXThis()) {
            receiverString = "this";
        }
        else {
            receiverString = expr2str(receiver, result.SourceManager, result.Context);
        }
    }
    else {
        throw std::runtime_error("Bad number of args: " + std::to_string(numArgs));
    }

    //get the start/end location for the SIGNAL/SLOT macros, since getLocStart() and getLocEnd() don't return the right result for expanded macros
    bool signalRangeOk;
    SourceRange signalRange = getMacroExpansionRange(signal, result.SourceManager, result.Context->getLangOpts(), &signalRangeOk);
    bool slotRangeOk;
    SourceRange slotRange = getMacroExpansionRange(slot, result.SourceManager, result.Context->getLangOpts(), &slotRangeOk);
    if (!signalRangeOk || !slotRangeOk) {
        throw std::runtime_error("connect() call must use SIGNAL/SLOT macro so that conversion can work!\n");
    }

    const StringLiteral* signalLiteral = result.Nodes.getNodeAs<clang::StringLiteral>("signalStr");
    const StringLiteral* slotLiteral = result.Nodes.getNodeAs<clang::StringLiteral>("slotStr");

    const CXXRecordDecl* senderTypeDecl = sender->getBestDynamicClassType();
    const CXXRecordDecl* receiverTypeDecl = receiver->getBestDynamicClassType();


    const std::string signalReplacement = calculateReplacementStr(senderTypeDecl, signalLiteral, std::string());
    std::string slotReplacement = calculateReplacementStr(receiverTypeDecl, slotLiteral, receiverString);
    printReplacementRange(signalRange, result.SourceManager, signalReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(signalRange), signalReplacement));
    printReplacementRange(slotRange, result.SourceManager, slotReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(slotRange), slotReplacement));
    convertedMatches++;
}

std::string ConnectCallMatcher::calculateReplacementStr(const CXXRecordDecl* type,
        const StringLiteral* connectStr, const std::string& prepend) {
    struct SearchInfo {
        std::vector<const CXXMethodDecl*> results;
        StringRef methodName;
        StringRef parameters;
    } searchInfo;
    searchInfo.methodName = signalSlotName(connectStr);
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
                info->results.push_back(*it);
//                llvm::outs() << "Found " << searchInfo->results.size() << ". defintion: "
//                        << cls->getName() << "::" << it->getName() << "\n";
            }
        }
        return true;
    };
    searchLambda(type, &searchInfo); //search baseClass
    type->forallBases(searchLambda, &searchInfo, false); //now find in base classes
    SmallString<128> result;
    if (!prepend.empty()) {
        result = prepend + ", ";
    }
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

void ConnectConverter::setupMatchers(MatchFinder* match_finder) {
    //both connect overloads
    using namespace clang::ast_matchers;
    match_finder->addMatcher(id("callExpr", memberCallExpr(
            hasDeclaration(id("decl",
                    methodDecl(
                            hasName("::QObject::connect"),
                            parameterCountIs(4)
                    )
             )),
             hasArgument(1, anyOf(hasDescendant(stringLiteral().bind("signalStr")), stringLiteral().bind("signalStr"))),
             hasArgument(2, anyOf(hasDescendant(stringLiteral().bind("slotStr")), stringLiteral().bind("slotStr")))

    )), &matcher);

    match_finder->addMatcher(id("callExpr", callExpr(

            hasDeclaration(id("decl",
                    methodDecl(
                            hasName("::QObject::connect"),
                            parameterCountIs(5)
                    )
             )),

             hasArgument(1, anyOf(hasDescendant(stringLiteral().bind("signalStr")), stringLiteral().bind("signalStr"))),
             hasArgument(3, anyOf(hasDescendant(stringLiteral().bind("slotStr")), stringLiteral().bind("slotStr")))

    )), &matcher);
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
