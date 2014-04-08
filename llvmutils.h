#ifndef LLVMUTILS_H_
#define LLVMUTILS_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

struct ColouredOStream {
    ColouredOStream(llvm::raw_ostream& stream, llvm::raw_ostream::Colors colour, bool bold)
            : stream(stream) {
        stream.changeColor(colour, bold);
    };
    ~ColouredOStream() { stream.resetColor(); }
    template<typename T>
    ColouredOStream& operator<<(const T& value) {
        stream << value;
        return *this;
    }
    ColouredOStream& writeEscaped(llvm::StringRef str, bool useHexEscapes = false) {
        stream.write_escaped(str, useHexEscapes);
        return *this;
    }

private:
    llvm::raw_ostream& stream;
};

static inline ColouredOStream colouredOut(llvm::raw_ostream::Colors colour, bool bold = true) {
    return ColouredOStream(llvm::outs(), colour, bold);
}

static inline ColouredOStream colouredErr(llvm::raw_ostream::Colors colour, bool bold = true) {
    return ColouredOStream(llvm::errs(), colour, bold);
}

static inline bool isNullPointerConstant(const clang::Expr* expr, clang::ASTContext* ctx) {
    return expr->isNullPointerConstant(*ctx, clang::Expr::NPC_NeverValueDependent) == clang::Expr::NPCK_NotNull;
}

namespace {
template<class T>
static const T* findFirstChildWithTypeHelper(const clang::Stmt* stmt) {
    const T* casted = llvm::dyn_cast<T>(stmt);
    if (casted) {
        return casted;
    }
    for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
        // will return *it if it is the correct type, otherwise it checks the children
        const T* child = findFirstChildWithTypeHelper<T>(*it);
        if (child) {
            return child;
        }
    }
    return nullptr;
}
}

/** find the first child of @p stmt or with type @p T or null if none found */
template<class T>
static inline const T* findFirstChildWithType(const clang::Stmt* stmt) {
    if (!stmt) {
        return nullptr;
    }
    return findFirstChildWithTypeHelper<T>(stmt);
}

/** @return the source location just after the end of  @p stmt. Useful for creating a clang::SourceRange for Replacements
 * Adds @p offset characters to the end if needed */
static inline clang::SourceLocation sourceLocationAfterStmt(const clang::Stmt* stmt, clang::ASTContext* ctx, int offset = 0) {
    clang::SourceLocation endLoc = stmt->getLocEnd();
    // if it is a macro expansion sets endLoc to the location where the macro is being expanded, not where it is defined
    // i.e. in this case from qobjectdefs.h -> file that is refactored
    clang::Lexer::isAtEndOfMacroExpansion(endLoc, ctx->getSourceManager(), ctx->getLangOpts(), &endLoc);

    // TODO: do we want to be just after the token, or is advancing by one character enough? Probably 1 char is fine
    //return clang::Lexer::getLocForEndOfToken(stmt->getLocEnd(), offset, ctx->getSourceManager(), ctx->getLangOpts());
    return endLoc.getLocWithOffset(1 + offset); // add one to the end location to point just past it
}

/** @return a SourceRange that contains the whole @p stmt, optionally adding/removing @p offset characters */
static inline clang::SourceRange sourceRangeForStmt(const clang::Stmt* stmt, clang::ASTContext* ctx, int offset = 0) {
    clang::SourceLocation beginLoc = stmt->getLocStart();
    // if it is a macro expansion sets beginLoc to the location where the macro is being expanded, not where it is defined
    // i.e. in this case from qobjectdefs.h -> file that is refactored
    clang::Lexer::isAtStartOfMacroExpansion(beginLoc, ctx->getSourceManager(), ctx->getLangOpts(), &beginLoc);
    return clang::SourceRange(beginLoc, sourceLocationAfterStmt(stmt, ctx, offset));
}

#endif /* LLVMUTILS_H_ */
