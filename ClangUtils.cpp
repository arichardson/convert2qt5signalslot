#include "ClangUtils.h"

#include <clang/AST/DeclCXX.h>

using namespace clang;
using namespace llvm;

std::string ClangUtils::getLeastQualifiedName(const clang::CXXRecordDecl* type, const clang::DeclContext* containingFunction,
        const clang::CallExpr* callExpression, bool verbose) {
    auto targetTypeQualifiers = getNameQualifiers(type);
    assert(type->Equals(targetTypeQualifiers[0]));
    std::string qualifiedName;

    // TODO template arguments

    if (targetTypeQualifiers.size() < 2) {
        // no need to qualify the name if there is no surrounding context
        return type->getName();
    }

    // have to qualify, but check the current scope first
    auto containingScopeQualifiers = getNameQualifiers(containingFunction->getLookupParent());
    // type must always be included, now check whether the other scopes have to be explicitly named
    // it's not neccessary if the current function scope is also inside that namespace/class
    Twine buffer = type->getName();
    for (uint i = 1; i < targetTypeQualifiers.size(); ++i) {
        const DeclContext* ctx = targetTypeQualifiers[i];
        assert(ctx->isNamespace() || ctx->isRecord());
        if (!contains(containingScopeQualifiers, [ctx](const DeclContext* dc) { return ctx->Equals(dc); })) {
            if (auto record = dyn_cast<CXXRecordDecl>(ctx)) {
                buffer = record->getName() + "::" + buffer;
            }
            else if (auto ns = dyn_cast<NamespaceDecl>(ctx)) {
                buffer = ns->getName() + "::" + buffer;
            }
            else {
                // this should never happen
                outs() << "Weird type:" << ctx->getDeclKindName() << ":" << (void*)ctx << "\n";
                printParentContexts(type);
            }
        }
        else {
            auto named = dyn_cast<NamedDecl>(ctx);
            if (verbose) {
                outs() << "Don't need to add " << (named ? named->getName() : "nullptr") << " to lookup\n";
            }
        }
    }
    return buffer.str();
}

std::vector<const clang::DeclContext*> ClangUtils::getNameQualifiers(const clang::DeclContext* ctx) {
    std::vector<const clang::DeclContext*> ret;
    while (ctx) {
        // only namespaces and classes/structs/unions add additional qualifiers to the name lookup
        if (auto ns = dyn_cast<clang::NamespaceDecl>(ctx)) {
            if (!ns->isAnonymousNamespace()) {
                ret.push_back(ns);
            }
        }
        else if (isa<clang::CXXRecordDecl>(ctx)) {
            ret.push_back(ctx);
        }
        ctx = ctx->getLookupParent();
    }
    return ret;
}

void ClangUtils::printParentContexts(const clang::DeclContext* base) {
    for (auto ctx : getParentContexts(base)) {
        outs() << "::";
        if (auto record = dyn_cast<clang::CXXRecordDecl>(ctx)) {
            outs() << record->getName() << "(record)";
        }
        else if (auto ns = dyn_cast<clang::NamespaceDecl>(ctx)) {
            outs() << ns->getName() << "(namespace)";
        }
        else if (auto func = dyn_cast<clang::FunctionDecl>(ctx)) {
            if (isa<clang::CXXConstructorDecl>(ctx)) {
                outs() << "(ctor)";
            }
            else if (isa<clang::CXXDestructorDecl>(ctx)) {
                outs() << "(dtor)";
            }
            else if (isa<clang::CXXConversionDecl>(ctx)) {
                outs() << "(conversion)";
            }
            else {
                outs() << func->getName() << "(func)";
            }
        }
        else if (dyn_cast<clang::TranslationUnitDecl>(ctx)) {
            outs() << "(translation unit)";
        }
        else {
            outs() << "unknown(" << ctx->getDeclKindName() << ")";
        }
    }
}


bool ClangUtils::inheritsFrom(const clang::CXXRecordDecl* cls, const char* name, clang::AccessSpecifier access) {
    for (auto it = cls->bases_begin(); it != cls->bases_end(); ++it) {
        clang::QualType type = it->getType();
        auto baseDecl = type->getAsCXXRecordDecl(); // will always succeed
        assert(baseDecl);
        // llvm::outs() << "Checking " << cls->getName() << " for " << name << "\n";
        if (it->getAccessSpecifier() == access && baseDecl->getName() == name) {
            return true;
        }
        // now check the base classes for the base class
        if (inheritsFrom(baseDecl, name, access)) {
            return true;
        }
    }
    return false;
}

bool ClangUtils::isOrInheritsFrom(const clang::CXXRecordDecl* cls, const char* name, clang::AccessSpecifier access) {
    // llvm::outs() << "Checking " << cls->getName() << " for " << name << "\n";
    if (cls->getName() == name) {
        return true;
    }
    return inheritsFrom(cls, name, access);
}
