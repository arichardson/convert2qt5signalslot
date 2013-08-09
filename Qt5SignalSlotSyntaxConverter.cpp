
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Refactoring.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/Casting.h>


using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

namespace {


class ConnectCallMatcher : public MatchFinder::MatchCallback {
public:
    ConnectCallMatcher(Replacements* reps) : replacements(reps) {}
    virtual void run(const MatchFinder::MatchResult& result) override;
private:
    int count = 0;
    Replacements* replacements;
};


class ConnectConverter {
public:
    ConnectConverter(Replacements* reps) : matcher(reps) {}
    void setupMatchers(MatchFinder* matchFinder);
private:
    ConnectCallMatcher matcher;
};


LangOptions options = []() { LangOptions l; l.CPlusPlus11 = true; return l; }();

//http://stackoverflow.com/questions/11083066/getting-the-source-behind-clangs-ast
std::string expr2str(const Expr *d, SourceManager* manager) {
    clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
    clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, *manager, options));
    return std::string(manager->getCharacterData(b), manager->getCharacterData(e)-manager->getCharacterData(b));
}

/** Signal/Slot string literals are always
 * "0" or "1" or "2" followed by the method name (e.g. "valueChanged(int)" )
 * then a null character, and then filename + linenumber
 *  the interesting part is from the second char to the first '(' character
 */
std::string signalSlotName(const StringLiteral* literal) {
    StringRef bytes = literal->getBytes();
    return bytes.substr(1, bytes.find_first_of('(') - 1);
}

/** expr->getLocStart() + expr->getLocEnd() don't return the proper location if the expression is a macro expansion
 * this function fixes this
 */
static SourceRange getMacroExpansionRange(const Expr* expr,SourceManager* manager, bool* validMacroExpansion) {
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
    llvm::outs() << " with " << replacement << "\n";
}

void ConnectCallMatcher::run(const MatchFinder::MatchResult& result) {
    const CallExpr* call = result.Nodes.getNodeAs<CallExpr>("callExpr");
    const CXXMethodDecl* decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    llvm::outs() << "Match " << ++count << " found at ";
    call->getExprLoc().print(llvm::outs(), *result.SourceManager);
    unsigned numArgs = call->getNumArgs();
    llvm::outs() << ", num args: " << numArgs << ": ";

    //call->dumpPretty(*result.Context); //show result after macro expansion

    const std::string oldCall = expr2str(call, result.SourceManager);
    llvm::outs() << oldCall << "\n";
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
        receiverString = expr2str(receiver, result.SourceManager);
        slot = call->getArg(3);
        connectionTypeExpr = call->getArg(4);

    }
    else if (numArgs == 4) {
        signal = call->getArg(1);
        slot = call->getArg(2);
        assert(isa<CXXMemberCallExpr>(call));
        const CXXMemberCallExpr* cxxCall = cast<CXXMemberCallExpr>(call);
        receiver = cxxCall->getImplicitObjectArgument(); //get this pointer
        connectionTypeExpr =  call->getArg(3);
        receiverString = "this";
    }
    else {
        llvm::errs() << "Bad number of args: " << numArgs << "\n";
        return;
    }

    //get the start/end location for the SIGNAL/SLOT macros, since getLocStart() and getLocEnd() don't return the right result for expanded macros
    bool signalRangeOk;
    SourceRange signalRange = getMacroExpansionRange(signal, result.SourceManager, &signalRangeOk);
    bool slotRangeOk;
    SourceRange slotRange = getMacroExpansionRange(slot, result.SourceManager, &slotRangeOk);
    if (!signalRangeOk || !slotRangeOk) {
        llvm::errs() << "connect() call must use SIGNAL/SLOT macro so that conversion can work!\n";
        return;
    }

    const std::string signalName = signalSlotName(result.Nodes.getNodeAs<clang::StringLiteral>("signalStr"));
    const std::string slotName = signalSlotName(result.Nodes.getNodeAs<clang::StringLiteral>("slotStr"));

    const CXXRecordDecl* senderTypeDecl = sender->getBestDynamicClassType();
    const std::string senderType = senderTypeDecl->getDeclName().getAsString();
    const CXXRecordDecl* receiverTypeDecl = receiver->getBestDynamicClassType();
    const std::string receiverType = receiverTypeDecl->getDeclName().getAsString();


    llvm::outs() << "\n";

    const std::string senderString = expr2str(sender, result.SourceManager);
    llvm::outs() << "sender: " <<  senderString << " , type: " << senderType;
    //sender->dumpPretty(*result.Context);

    llvm::outs() << ", signal: " << signalName;
    llvm::outs() << ", receiver: " << receiverString  << " , type: " << receiverType;
    llvm::outs() << ", signal: " << slotName;
    llvm::outs() << "type: ";
    std::string connectionType;
    if (connectionTypeExpr->isDefaultArgument()) {
        llvm::outs() << "default param";
        connectionType = "";
    }
    else {
        llvm::outs() << expr2str(connectionTypeExpr, result.SourceManager);
        connectionType = ", " + expr2str(connectionTypeExpr, result.SourceManager);
    }
    llvm::outs() << "\n";

    const std::string signalReplacement = "&" + senderType + "::" + signalName;
    //if converting member version we have to add the previously implicit this argument
    const std::string slotReplacement = (numArgs == 4 ? "this, &" : "&") + receiverType + "::" + slotName;
    printReplacementRange(signalRange, result.SourceManager, signalReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(signalRange), signalReplacement));
    printReplacementRange(slotRange, result.SourceManager, slotReplacement);
    replacements->insert(Replacement(*result.SourceManager, CharSourceRange::getTokenRange(slotRange), slotReplacement));
}

void ConnectConverter::setupMatchers(MatchFinder* match_finder) {
    //both connect overloads
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

}  // namespace

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  CommonOptionsParser options(argc, argv);
  tooling::RefactoringTool tool(options.getCompilations(), options.getSourcePathList());

  MatchFinder matchFinder;
  ConnectConverter converter(&tool.getReplacements());
  converter.setupMatchers(&matchFinder);

  return tool.runAndSave(newFrontendActionFactory(&matchFinder));
}
