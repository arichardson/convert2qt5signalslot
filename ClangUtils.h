#ifndef CLANGUTILS_H_
#define CLANGUTILS_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/ASTContext.h>
#include <clang/Lex/Lexer.h>

namespace ClangUtils {

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
static inline const T* findFirstChildWithTypeHelper(const clang::Stmt* stmt) {
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

    auto ret = clang::Lexer::getLocForEndOfToken(stmt->getLocEnd(), 0, ctx->getSourceManager(), ctx->getLangOpts());
    return ret.getLocWithOffset(offset);
}

static inline clang::SourceLocation sourceLocationBeforeStmt(const clang::Stmt* stmt, clang::ASTContext* ctx, int offset = 0) {
    clang::SourceLocation beginLoc = stmt->getLocStart();
    // if it is a macro expansion sets beginLoc to the location where the macro is being expanded, not where it is defined
    // i.e. in this case from qobjectdefs.h -> file that is refactored
    clang::Lexer::isAtStartOfMacroExpansion(beginLoc, ctx->getSourceManager(), ctx->getLangOpts(), &beginLoc);
    return beginLoc.getLocWithOffset(offset);
}

/** @return a SourceRange that contains the whole @p stmt, optionally adding/removing @p offset characters */
static inline clang::SourceRange sourceRangeForStmt(const clang::Stmt* stmt, clang::ASTContext* ctx, int offset = 0) {
    return clang::SourceRange(sourceLocationBeforeStmt(stmt, ctx), sourceLocationAfterStmt(stmt, ctx, offset));
}

/** Checks whether @p cls inherits from a class named @p name with the specified access (public, private, protected, none)
 *
 * @param name can be something like "class QObject" (must match the way a QualType is converted to string) */
static inline bool inheritsFrom(const clang::CXXRecordDecl* cls, const char* name,
        clang::AccessSpecifier access = clang::AS_public) {
    for (auto it = cls->bases_begin(); it != cls->bases_end(); ++it) {
        clang::QualType type = it->getType();
        const std::string typeName = type.getAsString();
        if (it->getAccessSpecifier() == access && typeName == name) {
            return true;
        }
        auto baseDecl = type->getAsCXXRecordDecl(); // will always succeed
        assert(baseDecl);
        // now check the base classes for the base class
        bool found = inheritsFrom(baseDecl, name, access);
        if (found) {
            return true;
        }
    }
    return false;
}

/** returns the parent contexts of @p ctx (including @p ctx). First entry will be ctx and the last will be a TranslationUnitDecl* */
static inline std::vector<const clang::DeclContext*> getParentContexts(const clang::DeclContext* ctx) {
    std::vector<const clang::DeclContext*> ret;
    while (ctx) {
        ret.push_back(ctx);
        ctx = ctx->getLookupParent();
    }
    return ret;
}

static inline  void printParentContexts(const clang::DeclContext* base) {
    for (auto ctx : getParentContexts(base)) {
        llvm::outs() << "::";
        if (auto record = llvm::dyn_cast<clang::CXXRecordDecl>(ctx)) {
            llvm::outs() << record->getName() << "(record)";
        }
        else if (auto ns = llvm::dyn_cast<clang::NamespaceDecl>(ctx)) {
            llvm::outs() << ns->getName() << "(namespace)";
        }
        else if (auto func = llvm::dyn_cast<clang::FunctionDecl>(ctx)) {
            if (llvm::isa<clang::CXXConstructorDecl>(ctx)) {
                llvm::outs() << "(ctor)";
            }
            else if (llvm::isa<clang::CXXDestructorDecl>(ctx)) {
                llvm::outs() << "(dtor)";
            }
            else if (llvm::isa<clang::CXXConversionDecl>(ctx)) {
                llvm::outs() << "(conversion)";
            }
            else {
                llvm::outs() << func->getName() << "(func)";
            }
        }
        else if (llvm::dyn_cast<clang::TranslationUnitDecl>(ctx)) {
            llvm::outs() << "(translation unit)";
        }
        else {
            llvm::outs() << "unknown(" << ctx->getDeclKindName() << ")";
        }
    }
}

static inline std::vector<const clang::DeclContext*> getNameQualifiers(const clang::DeclContext* ctx) {
    std::vector<const clang::DeclContext*> ret;
    while (ctx) {
        // only namespaces and classes/structs/unions add additional qualifiers to the name lookup
        if (auto ns = llvm::dyn_cast<clang::NamespaceDecl>(ctx)) {
            if (!ns->isAnonymousNamespace()) {
                ret.push_back(ns);
            }
        }
        else if (llvm::isa<clang::CXXRecordDecl>(ctx)) {
            ret.push_back(ctx);
        }
        ctx = ctx->getLookupParent();
    }
    return ret;
}

} // namespace ClangUtils

#endif /* CLANGUTILS_H_ */
