#pragma once

#include <llvm/ADT/StringRef.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Basic/Version.h>

#include <QtGlobal>

class ConverterPPCallbacks  : public clang::PPCallbacks {
    clang::Preprocessor& pp;
    uint undefWarningDiagId;
    uint redefWarningDiagId;
    bool addingQ_PRIVATE_SLOT_defintion = false;
public:
    ConverterPPCallbacks(clang::Preprocessor& pp) : pp(pp) {
        undefWarningDiagId = pp.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Warning, "undefining '%0' macro");
        redefWarningDiagId = pp.getDiagnostics().getCustomDiagID(clang::DiagnosticsEngine::Warning, "redefining '%0' macro");
    }
    virtual ~ConverterPPCallbacks();

    void addQ_PRIVATE_SLOT_definition(clang::SourceLocation origLoc);
    void handleMacroUndefined(const clang::Token& token);
protected:

#if CLANG_VERSION_MAJOR >= 3 && CLANG_VERSION_MINOR >= 7
    void MacroUndefined(const clang::Token& token, const clang::MacroDefinition&) override {
#else
    void MacroUndefined(const clang::Token& token, const clang::MacroDirective*) override {
#endif
        handleMacroUndefined(token);
    }

    void MacroDefined(const clang::Token& token, const clang::MacroDirective* md) override;
    void FileChanged(clang::SourceLocation loc, FileChangeReason reason, clang::SrcMgr::CharacteristicKind, clang::FileID prevFID) override;
    bool FileNotFound(llvm::StringRef FileName, llvm::SmallVectorImpl<char>& RecoveryPath) override;

//    void InclusionDirective(clang::SourceLocation, const clang::Token&, llvm::StringRef FileName, bool, clang::CharSourceRange, const clang::FileEntry*, llvm::StringRef, llvm::StringRef, const clang::Module*) override {
//        llvm::errs() << "InclusionDirective: " << FileName << "\n";
//    }
};
