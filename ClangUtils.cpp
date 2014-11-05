#include "ClangUtils.h"

#include <clang/AST/DeclCXX.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/Lookup.h>
#include <clang/Lex/Preprocessor.h>

using namespace clang;
using llvm::outs;
using llvm::errs;

static void collectAllUsingNamespaceDeclsHelper(const clang::DeclContext* ctx, std::vector<UsingDirectiveDecl*>& buf,
        SourceLocation callLocation, const SourceManager& sm) {
    if (!ctx) {
        return;
    }
    for (auto&& udir : ctx->using_directives()) {
        // udir->dump(outs());
        if (!sm.isBeforeInTranslationUnit(udir->getLocStart(), callLocation)) {
            // outs() << "Using directive is after call location, not adding!\n";
            continue;
        }
        buf.push_back(udir);
    }
    collectAllUsingNamespaceDeclsHelper(ctx->getLookupParent(), buf, callLocation, sm);
}

static std::vector<UsingDirectiveDecl*> collectAllUsingNamespaceDecls(const clang::DeclContext* ctx,
        SourceLocation callLocation, const SourceManager& sm) {
    std::vector<UsingDirectiveDecl*> ret;
    collectAllUsingNamespaceDeclsHelper(ctx, ret, callLocation, sm);
    return ret;
}

static void collectAllUsingDeclsHelper(const clang::DeclContext* ctx, std::vector<UsingShadowDecl*>& buf,
        SourceLocation callLocation, const SourceManager& sm) {
    if (!ctx) {
        return;
    }
    for (auto it = ctx->decls_begin(); it != ctx->decls_end(); ++it) {
        auto usingDecl = dyn_cast<UsingDecl>(*it);
        if (!usingDecl) {
            continue;
        }
        if (!sm.isBeforeInTranslationUnit(usingDecl->getLocStart(), callLocation)) {
            //outs() << "Using decl is after call location, not adding!\n";
            continue;
        }
        //usingDecl->dump(outs());
        for (auto shadowIt = usingDecl->shadow_begin(); shadowIt != usingDecl->shadow_end(); shadowIt++) {
            //(*shadowIt)->dump(outs());
            buf.push_back(*shadowIt);
        }
    }
    collectAllUsingDeclsHelper(ctx->getLookupParent(), buf, callLocation, sm);
}

static std::vector<UsingShadowDecl*> collectAllUsingDecls(const clang::DeclContext* ctx, SourceLocation callLocation,
        const SourceManager& sm) {
    std::vector<UsingShadowDecl*> ret;
    collectAllUsingDeclsHelper(ctx, ret, callLocation, sm);
    return ret;
}

static std::string getLeastQualifiedNameInternal(const clang::TagType* tt, const clang::DeclContext* containingFunction,
        const clang::CallExpr* callExpression, bool verbose, clang::ASTContext* ast) {
    static StringRef colonColon = "::";
    auto td = tt->getDecl();
    auto targetTypeQualifiers = ClangUtils::getNameQualifiers(td);
    //printParentContexts(type);
    //outs() << ", name qualifiers: " << targetTypeQualifiers.size() << "\n";
    assert(td->Equals(targetTypeQualifiers[0]));
    // TODO template arguments


    if (targetTypeQualifiers.size() < 2) {
        // no need to qualify the name if there is no surrounding context
        return ClangUtils::getNameWithTemplateParams(td, ast->getLangOpts());
    }
    auto usingDecls = collectAllUsingDecls(containingFunction,
            ClangUtils::sourceLocationBeforeStmt(callExpression, ast), ast->getSourceManager());

    auto isUsingType = [td](UsingShadowDecl* usd) {
        auto cxxRecord = dyn_cast<CXXRecordDecl>(usd->getTargetDecl());
        return cxxRecord && td->Equals(cxxRecord);
    };
    if (ClangUtils::contains(usingDecls, isUsingType)) {
        if (verbose) {
            outs() << "Using decl for " << td->getQualifiedNameAsString() << " exists, not qualifying\n";
        }
        // no need to qualify the name if there is a using decl for the current type
        return ClangUtils::getNameWithTemplateParams(td, ast->getLangOpts());
    }

    auto usingNamespaceDecls = collectAllUsingNamespaceDecls(containingFunction,
            ClangUtils::sourceLocationBeforeStmt(callExpression, ast), ast->getSourceManager());

    // have to qualify, but check the current scope first
    auto containingScopeQualifiers = ClangUtils::getNameQualifiers(containingFunction->getLookupParent());
    // type must always be included, now check whether the other scopes have to be explicitly named
    // it's not neccessary if the current function scope is also inside that namespace/class
    // for some reason twine crashes here, use std::string
    llvm::SmallString<64> buffer;
    buffer = ClangUtils::getNameWithTemplateParams(td, ast->getLangOpts());
    for (uint i = 1; i < targetTypeQualifiers.size(); ++i) {
        const DeclContext* ctx = targetTypeQualifiers[i];
        assert(ctx->isNamespace() || ctx->isRecord());
        auto declContextEquals = [ctx](const DeclContext* dc) {
            return ctx->Equals(dc);
        };
        auto isUsingNamespaceForContext = [ctx](UsingDirectiveDecl* ud) {
            return ud->getNominatedNamespace()->Equals(ctx);
        };
        auto isUsingDeclForContext = [ctx](UsingShadowDecl* usd) {
            auto usedCtx = dyn_cast<DeclContext>(usd->getTargetDecl());
            return usedCtx && usedCtx->Equals(ctx);
        };
        if (ClangUtils::contains(containingScopeQualifiers, declContextEquals)) {
            auto named = cast<NamedDecl>(ctx);
            if (verbose) {
                outs() << "Don't need to add " << named->getQualifiedNameAsString() << " to lookup\n";
            }
        } else if (ClangUtils::contains(usingNamespaceDecls, isUsingNamespaceForContext)) {
            auto ns = cast<NamespaceDecl>(ctx);
            if (verbose) {
                outs() << "Ending qualifier search: using namespace for '" << ns->getQualifiedNameAsString() << "' exists\n";
            }
            break;
        } else if (ClangUtils::contains(usingDecls, isUsingDeclForContext)) {
            auto named = cast<NamedDecl>(ctx);
            // this is the last one we have to add since a using directive exists
            auto name = ClangUtils::getNameWithTemplateParams(named, ast->getLangOpts());
            buffer.insert(buffer.begin(), colonColon.begin(), colonColon.end());
            buffer.insert(buffer.begin(), name.begin(), name.end());
            if (verbose) {
                outs() << "Ending qualifier search: using for '" << named->getQualifiedNameAsString() << "' exists\n";
            }
            break;
        } else {
            if (auto record = dyn_cast<CXXRecordDecl>(ctx)) {
                auto name = ClangUtils::getNameWithTemplateParams(record, ast->getLangOpts());;
                buffer.insert(buffer.begin(), colonColon.begin(), colonColon.end());
                buffer.insert(buffer.begin(), name.begin(), name.end());
            } else if (auto ns = dyn_cast<NamespaceDecl>(ctx)) {
                auto name = ClangUtils::getNameWithTemplateParams(ns, ast->getLangOpts());;
                buffer.insert(buffer.begin(), colonColon.begin(), colonColon.end());
                buffer.insert(buffer.begin(), name.begin(), name.end());
            } else {
                // this should never happen
                outs() << "Weird type:" << ctx->getDeclKindName() << ":" << (void*) ctx << "\n";
                ClangUtils::printParentContexts(td);
            }
        }
    }
    return buffer.str().str();
}

std::string ClangUtils::getLeastQualifiedName(clang::QualType type, const clang::DeclContext* containingFunction,
        const clang::CallExpr* callExpression, bool verbose, clang::ASTContext* ast) {
    // outs() << "About to print " << type.getAsString() << "\n";
    std::string append;
    // determine the references and pointers to be appended to the type
    if (type->isReferenceType()) {
        append = "&";
        type = type->getPointeeType();
    }
    while (type->isPointerType()) {
        Qualifiers qual = type.getQualifiers();
        if (qual.hasConst()) {
            append.insert(0, " const");
        }
        if (qual.hasVolatile()) {
            append.insert(0, " volatile");
        }
        append.insert(0, "*");
        type = type->getPointeeType();
    }

    // now that we have the underlying type add the qualifiers
    Qualifiers qual = type.getQualifiers();
    const char* prepend = "";
    if (qual.hasConst()) {
        if (qual.hasVolatile()) {
            prepend = "volatile const ";
        } else {
            prepend = "const ";
        }
    } else if (qual.hasVolatile()) {
        prepend = "volatile ";
    }

    std::string result;
    if (const TagType* tt = type->getAs<TagType>()) {
        // outs() << type.getAsString() << " is a tag type\n";
        result = getLeastQualifiedNameInternal(tt, containingFunction, callExpression, verbose, ast);
    } else {
        // outs() << type.getAsString() << " is not a tag type\n";
        PrintingPolicy printPol(ast->getLangOpts());
        printPol.SuppressTagKeyword = true;
        printPol.LangOpts.CPlusPlus = true;
        printPol.LangOpts.CPlusPlus11 = true;
        result = type.getAsString(printPol); // could be builtin type e.g. int
    }
    // outs() << "least qualified name for " << QualType::getAsString(type.getTypePtr(), Qualifiers()) << " is " << (prepend + result + append) << "\n";
    // outs() << "pre='" << prepend << "', result='" << result << "', append='" << append << "'\n";
    return prepend + result + append;
}

std::vector<const clang::DeclContext*> ClangUtils::getNameQualifiers(const clang::DeclContext* ctx) {
    std::vector<const clang::DeclContext*> ret;
    while (ctx) {
        // only namespaces and classes/structs/unions add additional qualifiers to the name lookup
        if (auto ns = dyn_cast<clang::NamespaceDecl>(ctx)) {
            if (!ns->isAnonymousNamespace()) {
                ret.push_back(ns);
            }
        } else if (isa<clang::CXXRecordDecl>(ctx) || isa<clang::EnumDecl>(ctx)) {
            ret.push_back(ctx);
        } else if (isa<TranslationUnitDecl>(ctx)) {
            break;
        } else {
            errs() << "unknown context type: " << ctx->getDeclKindName() << "\n";
        }
        ctx = ctx->getLookupParent();
    }
    return ret;
}

void ClangUtils::printParentContexts(const clang::DeclContext* base) {
    for (auto ctx : getParentContexts(base)) {
        outs() << "::";
        if (auto record = dyn_cast<clang::CXXRecordDecl>(ctx)) {
            outs() << ClangUtils::getNameWithTemplateParams(record) << "(record)";
        } else if (auto ns = dyn_cast<clang::NamespaceDecl>(ctx)) {
            outs() << ClangUtils::getNameWithTemplateParams(ns) << "(namespace)";
        } else if (auto func = dyn_cast<clang::FunctionDecl>(ctx)) {
            if (isa<clang::CXXConstructorDecl>(ctx)) {
                outs() << "(ctor)";
            } else if (isa<clang::CXXDestructorDecl>(ctx)) {
                outs() << "(dtor)";
            } else if (isa<clang::CXXConversionDecl>(ctx)) {
                outs() << "(conversion)";
            } else {
                outs() << ClangUtils::getNameWithTemplateParams(func) << "(func)";
            }
        } else if (dyn_cast<clang::TranslationUnitDecl>(ctx)) {
            outs() << "(translation unit)";
        } else {
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

std::vector<clang::FunctionDecl*> ClangUtils::lookupFunctionsInClass(StringRef methodName, const CXXRecordDecl* type, SourceLocation loc, const CompilerInstance& ci) {
    DeclarationName lookupName(ci.getPreprocessor().getIdentifierInfo(methodName));
    // TODO: how to get a scope for lookup
    // looks like we have to fill the Scope* manually

    //outs() << "Looking up " << methodName << " in " << type->getQualifiedNameAsString() << "\n";
    LookupResult lookup(ci.getSema(), lookupName, loc, Sema::LookupMemberName);
    lookup.suppressDiagnostics(); // don't output errors if the method could not be found!
    // setting inUnqualifiedLookup to true makes sure that base classes are searched too
    ci.getSema().LookupQualifiedName(lookup, const_cast<DeclContext*>(type->getPrimaryContext()), true);
    //outs() << "lr after lookup: ";
    //lookup.print(outs());
    std::vector<FunctionDecl*> results;
    auto addDeclToResultsIfFunction = [&](NamedDecl* found) {
        // if it has been brought to the local scope using a using declaration get the actual decl instead
        if (UsingShadowDecl* usingShadow = dyn_cast<UsingShadowDecl>(found)) {
            found = usingShadow->getTargetDecl();
        }
        if (auto decl = dyn_cast<FunctionDecl>(found)) {
            results.push_back(decl);
        } else {
            errs() << found->getQualifiedNameAsString() << " is not a function, but a " << found->getDeclKindName() << "\n";
        }
    };
    if (lookup.isSingleResult()) {
        addDeclToResultsIfFunction(lookup.getFoundDecl());
    } else if (lookup.isOverloadedResult() || lookup.isAmbiguous()) {
        // if (lookup.isAmbiguous()) {
        //     errs() << "AMBIGUOUS LOOKUP!\"n;
        // }
        for (NamedDecl* found : lookup) {
            addDeclToResultsIfFunction(found);
        }
    }
    return results;
}

