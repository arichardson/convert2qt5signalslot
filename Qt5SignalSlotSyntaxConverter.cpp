
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Casting.h"


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
        connectionTypeExpr =  call->getArg(4);

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

    const std::string newCall = Twine("connect(" + senderString + ", &" + senderType + "::" + signalName + ", " + receiverString + ", &" + receiverType + "::" + slotName + connectionType + ")").str();

    llvm::outs() << "\n\n" << "old: " << oldCall << "\n" << "new: " << newCall << "\n\n";
    clang::CharSourceRange range = clang::CharSourceRange::getTokenRange(call->getSourceRange());
    replacements->insert(Replacement(*result.SourceManager, range, newCall));
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
             hasArgument(1, hasDescendant(id("signalStr", stringLiteral()))),
             hasArgument(2, hasDescendant(id("slotStr", stringLiteral())))

    )), &matcher);

    match_finder->addMatcher(id("callExpr", callExpr(

            hasDeclaration(id("decl",
                    methodDecl(
                            hasName("::QObject::connect"),
                            parameterCountIs(5)
                    )
             )),

             hasArgument(1, hasDescendant(id("signalStr", stringLiteral()))),
             hasArgument(3, hasDescendant(id("slotStr", stringLiteral())))

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
