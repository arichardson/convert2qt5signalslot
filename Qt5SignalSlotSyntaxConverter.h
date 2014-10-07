#pragma once
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Tooling/Refactoring.h>
#include <atomic>

namespace clang {
    class Sema;
}

extern llvm::cl::opt<std::string> nullPtrString;


using clang::ast_matchers::MatchFinder;

class ConnectCallMatcher : public MatchFinder::MatchCallback, public clang::tooling::SourceFileCallbacks {
public:
    enum ReplacementType {
        ReplaceSignal,
        ReplaceSlot,
        ReplaceOther
    };

    ConnectCallMatcher(clang::tooling::Replacements* reps, const std::vector<std::string>& files)
        : foundMatches(0), convertedMatches(0), skippedMatches(0), failedMatches(0), replacements(reps),
          refactoringFiles(files) {}
    virtual void run(const MatchFinder::MatchResult& result) override;
    void convert(const MatchFinder::MatchResult& result);
    bool handleBeginSource(clang::CompilerInstance &CI, llvm::StringRef Filename) override;
    void handleEndSource() override;

    void printStats() const;
    struct Parameters {
        const clang::CallExpr* call = nullptr;
        const clang::CXXMethodDecl* decl = nullptr;
        const clang::FunctionDecl* containingFunction = nullptr;
        const clang::Expr* sender = nullptr;
        const clang::Expr* signal = nullptr;
        const clang::StringLiteral* signalLiteral = nullptr;
        const clang::Expr* slot = nullptr;
        const clang::StringLiteral* slotLiteral = nullptr;
        const clang::Expr* receiver = nullptr;
        std::string containingFunctionName;
        std::string senderString;
        std::string receiverString;
    };
private:
    void convertConnect(Parameters& p, const MatchFinder::MatchResult& result);
    void convertDisconnect(Parameters& p, const MatchFinder::MatchResult& result);
    void matchFound(const Parameters& p, const MatchFinder::MatchResult& result);
    void addReplacement(clang::SourceRange range, const std::string& replacement, clang::ASTContext* ctx);
    /** remove foo-> from foo->connect() after the conversion */
    void tryRemovingMembercallArg(const Parameters& p, const MatchFinder::MatchResult& result);
    /**
     * @return The new signal/slot expression for the connect call
     */
    std::string calculateReplacementStr(const clang::CXXRecordDecl* type,
            const clang::StringLiteral* connectStr, const std::string& prepend, const Parameters& p);
private:
    friend class ConnectConverter;
    std::atomic_int foundMatches;
    std::atomic_int convertedMatches;
    std::atomic_int skippedMatches;
    std::atomic_int failedMatches;
    clang::CompilerInstance* currentCompilerInstance = nullptr;
    clang::Sema* sema = nullptr;
    clang::Preprocessor* pp = nullptr;
    clang::ASTContext* lastAstContext = nullptr;
    clang::QualType constCharPtrType;
    clang::tooling::Replacements* replacements = nullptr;
    const std::vector<std::string>& refactoringFiles;
};


class ConnectConverter {
public:
    ConnectConverter(clang::tooling::Replacements* reps, const std::vector<std::string>& files) : matcher(reps, files) {}
    void setupMatchers(MatchFinder* matchFinder);
    void printStats() const {
        matcher.printStats();
    }
    inline int matchesFound() const { return matcher.foundMatches.load(); }
    inline int matchesConverted() const { return matcher.convertedMatches.load(); }
    inline int matchesSkipped() const { return matcher.skippedMatches.load(); }
    inline int matchesFailed() const { return matcher.failedMatches.load(); }
    clang::tooling::SourceFileCallbacks* sourceCallback() { return &matcher; }
private:
    ConnectCallMatcher matcher;
};

static inline std::string getRealPath(const std::string& path) {
    char buf[PATH_MAX];
    const char* ret = realpath(path.c_str(), buf);
    if (!ret) {
        //resolving failed, could be a virtual path
        return path;
    }
    return std::string(ret);
}
