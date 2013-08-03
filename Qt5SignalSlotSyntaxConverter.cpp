
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


using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

namespace {


class ConnectAsMemberCallMatcher : public MatchFinder::MatchCallback {
public:
    ConnectAsMemberCallMatcher(Replacements* reps) : replacements(reps) {}
    virtual void run(const MatchFinder::MatchResult& result) override;
private:
    int count = 0;
    Replacements* const replacements;
};


class ConnectAsStaticCallMatcher : public MatchFinder::MatchCallback {
public:
    ConnectAsStaticCallMatcher(Replacements* reps) : replacements(reps) {}
    virtual void run(const MatchFinder::MatchResult& result) override;
private:
    int count = 0;
    Replacements* replacements;
};


class ConnectConverter {
public:
    ConnectConverter(Replacements* reps) : staticMatcher(reps), memberMatcher(reps) {}
    void setupMatchers(MatchFinder* matchFinder);
private:
    ConnectAsStaticCallMatcher staticMatcher;
    ConnectAsMemberCallMatcher memberMatcher;
};


void ConnectAsMemberCallMatcher::run(const MatchFinder::MatchResult& result) {
    const CXXMemberCallExpr* call = result.Nodes.getNodeAs<CXXMemberCallExpr>("callExpr");
    const CXXMethodDecl* decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    llvm::errs() << "Member Match " << ++count << " found at";
    call->getExprLoc().print(llvm::errs(), *result.SourceManager);
    llvm::errs() << ":\n";
    call->dumpPretty(*result.Context);
    //decl->printName(llvm::errs());
    assert(decl->getNameAsString() == "connect");
    assert(decl->getDeclKind() == Decl::CXXMethod);

    const Expr* sender = call->getArg(0);
    const Expr* signal = call->getArg(1);
    const Expr* slot = call->getArg(2);
    const Expr* type = call->getArg(3);

    llvm::errs() << "\nsender: ";
    sender->dumpPretty(*result.Context);
    llvm::errs() << "\nsignal: ";
    signal->dumpPretty(*result.Context);
    llvm::errs() << "\nslot: ";
    slot->dumpPretty(*result.Context);
    llvm::errs() << "\ntype: ";
    if (type->isDefaultArgument())
        llvm::errs() << "default param";
    else
        type->dumpPretty(*result.Context);

    llvm::errs() << "\n\n\n\n";
}

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

void ConnectAsStaticCallMatcher::run(const MatchFinder::MatchResult& result) {
    const CallExpr* call = result.Nodes.getNodeAs<CallExpr>("callExpr");
    const CXXMethodDecl* decl = result.Nodes.getNodeAs<CXXMethodDecl>("decl");
    llvm::errs() << "Static Match " << ++count << " found at ";
    call->getExprLoc().print(llvm::errs(), *result.SourceManager);
    llvm::errs() << ":\n";
    //call->dumpPretty(*result.Context); //show result after macro expansion

    const std::string oldCall = expr2str(call, result.SourceManager);
    llvm::errs() << "\n";
    llvm::errs() << oldCall << "\n";
    //call->dumpColor(); //show AST


    const std::string signalName = signalSlotName(result.Nodes.getNodeAs<clang::StringLiteral>("signalStr"));
    const std::string slotName = signalSlotName(result.Nodes.getNodeAs<clang::StringLiteral>("slotStr"));

    const Expr* sender = call->getArg(0);
    const CXXRecordDecl* senderTypeDecl = sender->getBestDynamicClassType();
    const std::string senderType = senderTypeDecl->getDeclName().getAsString();
    //const Expr* signal = call->getArg(1);
    const Expr* receiver = call->getArg(2);
    const CXXRecordDecl* receiverTypeDecl = receiver->getBestDynamicClassType();
    const std::string receiverType = receiverTypeDecl->getDeclName().getAsString();
    //const Expr* slot = call->getArg(3);
    const Expr* type = call->getArg(4);


    llvm::errs() << "\n";

    const std::string senderString = expr2str(sender, result.SourceManager);
    llvm::errs() << "sender: " <<  senderString << " , type: " << senderType << "\n";
    //sender->dumpPretty(*result.Context);

    llvm::errs() << "signal: " << signalName << "\n";

    const std::string receiverString = expr2str(receiver, result.SourceManager);
    llvm::errs() << "receiver: " << expr2str(receiver, result.SourceManager)  << " , type: " << receiverType << "\n";

    llvm::errs() << "signal: " << slotName << "\n";


    llvm::errs() << "type: ";
    std::string connectionType;
    if (type->isDefaultArgument()) {
        llvm::errs() << "default param";
        connectionType = "";
    }
    else {
        llvm::errs() << expr2str(type, result.SourceManager);
        connectionType = ", " + expr2str(type, result.SourceManager);
    }
    llvm::errs() << "\n";

    const std::string newCall = Twine("connect(" + senderString + ", &" + senderType + "::" + signalName + ", " + receiverString + ", &" + receiverType + "::" + slotName + connectionType + ")").str();

    llvm::errs() << "\n\n" << "old: " << oldCall << "\n" << "new: " << newCall << "\n\n";
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
             ))
    )), &memberMatcher);

    match_finder->addMatcher(id("callExpr", callExpr(

            hasDeclaration(id("decl",
                    methodDecl(
                            hasName("::QObject::connect"),
                            parameterCountIs(5)
                    )
             )),

             hasArgument(1, hasDescendant(id("signalStr", stringLiteral()))),

             hasArgument(3, hasDescendant(id("slotStr", stringLiteral())))

    )), &staticMatcher);
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
