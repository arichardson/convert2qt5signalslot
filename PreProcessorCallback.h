#pragma once

#include "llvmutils.h"
#include <clang/Lex/Preprocessor.h>

static const char* Q_PRIVATE_SLOT_definition =
        "#undef Q_PRIVATE_SLOT\n"
        "#define Q_PRIVATE_SLOT(d, signature) static_assert(#d && #signature, \"q_private_slot\");\n";


class ConverterPPCallbacks  : public clang::PPCallbacks {
    clang::Preprocessor& pp;
    uint undefWarningDiagId;
    uint redefWarningDiagId;
    bool addingQ_PRIVATE_SLOT_defintion = false;
    bool initialized = false;
public:
    ConverterPPCallbacks(clang::Preprocessor& pp) : pp(pp) {
        undefWarningDiagId = pp.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Warning, "undefining '%0' macro");
        redefWarningDiagId = pp.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Warning, "redefining '%0' macro");
    }
    void addQ_PRIVATE_SLOT_definition(clang::SourceLocation origLoc) {
        llvm::MemoryBuffer* file = llvm::MemoryBuffer::getMemBuffer(Q_PRIVATE_SLOT_definition, "q_private_slot_defintion.moc");
        clang::SourceLocation location = pp.getSourceManager().getFileLoc(origLoc);
        pp.EnterSourceFile(pp.getSourceManager().createFileIDForMemBuffer(file, clang::SrcMgr::C_System), nullptr, location);
    }
protected:
    void MacroUndefined(const clang::Token& token, const clang::MacroDirective*) override {
        if (token.getIdentifierInfo()->getName() == "Q_PRIVATE_SLOT") {
            if (!addingQ_PRIVATE_SLOT_defintion) {
                pp.getDiagnostics().Report(token.getLocation(), undefWarningDiagId) << "Q_PRIVATE_SLOT";
                // we have to redefine it to what it should be
                addQ_PRIVATE_SLOT_definition(token.getLocation());
            }
        }
    }

    void MacroDefined(const clang::Token& token, const clang::MacroDirective* md) override {
        //llvm::outs() << "macro defined: " << token.getIdentifierInfo()->getName() << "\n";
        if (token.getIdentifierInfo()->getName() == "Q_PRIVATE_SLOT") {
            if (!addingQ_PRIVATE_SLOT_defintion) {
                pp.getDiagnostics().Report(token.getLocation(), redefWarningDiagId) << "Q_PRIVATE_SLOT";
                // we have to redefine it to what it should be
                addQ_PRIVATE_SLOT_definition(token.getLocation());
            }
        }
    }
    virtual void FileChanged(clang::SourceLocation loc, FileChangeReason reason, clang::SrcMgr::CharacteristicKind, clang::FileID prevFID) override {
        clang::PresumedLoc presumedLoc = pp.getSourceManager().getPresumedLoc(loc, false);
        //llvm::outs() << "File changed: name=" << presumedLoc.getFilename() << ", reason=" << reason << "\n";
        if (!initialized) {
            initialized = true;
            addQ_PRIVATE_SLOT_definition({});
        }
        if (strcmp(presumedLoc.getFilename(), "q_private_slot_defintion.moc") == 0) {
            if (reason == EnterFile) {
                addingQ_PRIVATE_SLOT_defintion = true;
            }
            else if (reason == ExitFile) {
                addingQ_PRIVATE_SLOT_defintion = true;
            }
        }
    }
};
