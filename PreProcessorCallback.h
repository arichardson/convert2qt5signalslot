#pragma once

#include "ClangUtils.h"
#include <clang/Lex/Preprocessor.h>

static const char* Q_PRIVATE_SLOT_definition =
        "#undef Q_PRIVATE_SLOT\n"
        "#ifndef CONVERT_SIGNALS_TOKENPASTE\n"
        "#define CONVERT_SIGNALS_TOKENPASTE2(x, y) x ## y\n"
        "#define CONVERT_SIGNALS_TOKENPASTE(x, y) CONVERT_SIGNALS_TOKENPASTE2(x, y)\n"
        "#endif\n"
        "#define Q_PRIVATE_SLOT(d, signature) "
        "void CONVERT_SIGNALS_TOKENPASTE(__qt_private_slot_, __LINE__)() {"
        "    d;" // expression so we can get the type
        "    #d;" // make it easier to get the string representation of that expression
        "    #signature;" // we will have to parse this
        "}\n";


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
    void addQ_PRIVATE_SLOT_definition(clang::SourceLocation origLoc) {
        clang::SourceLocation location = pp.getSourceManager().getFileLoc(origLoc);
        pp.EnterSourceFile(pp.getSourceManager().createFileID(llvm::MemoryBuffer::getMemBuffer(Q_PRIVATE_SLOT_definition, "q_private_slot_defintion.moc"),
                clang::SrcMgr::C_System), nullptr, location);
    }
protected:
    void MacroUndefined(const clang::Token& token, const clang::MacroDirective*) override {
        if (token.getIdentifierInfo()->getName() == "Q_PRIVATE_SLOT") {
            //llvm::outs() << "Q_PRIVATE_SLOT undefined\n";
            if (!addingQ_PRIVATE_SLOT_defintion) {
                pp.getDiagnostics().Report(token.getLocation(), undefWarningDiagId) << "Q_PRIVATE_SLOT";
                // we have to redefine it to what it should be
                addQ_PRIVATE_SLOT_definition(token.getLocation());
            }
        }
    }

    void MacroDefined(const clang::Token& token, const clang::MacroDirective* md) override {
        llvm::StringRef macroName = token.getIdentifierInfo()->getName();

#if 0 // enable to debug macro definitions
        if (!macroName.startswith("__")) { // hide the builtin macros
            std::string location = token.getLocation().printToString(pp.getSourceManager());
            llvm::errs() << location << ": " << macroName << " = ";
            pp.DumpMacro(*md->getMacroInfo());
        }
#endif
        if (macroName == "Q_PRIVATE_SLOT") {
            //llvm::outs() << "Q_PRIVATE_SLOT defined\n" << "at" << location << mdLocation;
            if (!addingQ_PRIVATE_SLOT_defintion) {
                llvm::StringRef fileName = pp.getSourceManager().getPresumedLoc(token.getLocation()).getFilename();
                //llvm::errs() << "redefinition location: " << fileName << "\n";
                // the definition in qobjectdefs.h is expected -> don't warn
                if (!fileName.endswith("qobjectdefs.h")) {
                    pp.getDiagnostics().Report(token.getLocation(), redefWarningDiagId) << "Q_PRIVATE_SLOT";
                }
                // we have to redefine it to what it should be
                addQ_PRIVATE_SLOT_definition(token.getLocation());
            }
        }
    }

    void FileChanged(clang::SourceLocation loc, FileChangeReason reason, clang::SrcMgr::CharacteristicKind, clang::FileID prevFID) override {
        clang::PresumedLoc presumedLoc = pp.getSourceManager().getPresumedLoc(loc, false);
        //llvm::outs() << "File changed: name=" << presumedLoc.getFilename() << ", reason=" << reason << "\n";
        if (strcmp(presumedLoc.getFilename(), "q_private_slot_defintion.moc") == 0) {
            if (reason == EnterFile) {
                addingQ_PRIVATE_SLOT_defintion = true;
            }
            else if (reason == ExitFile) {
                addingQ_PRIVATE_SLOT_defintion = true;
            }
        }
    }

    bool FileNotFound(llvm::StringRef FileName, llvm::SmallVectorImpl< char >& RecoveryPath) override {
        // llvm::errs() << "File not found: " << FileName << "\n";
        if (FileName.endswith(".moc") || FileName.startswith("moc_") || FileName.endswith("_moc.cpp")) {
            if (!pp.GetSuppressIncludeNotFoundError()) {
                pp.SetSuppressIncludeNotFoundError(true);
            }
        } else {
            if (pp.GetSuppressIncludeNotFoundError()) {
                pp.SetSuppressIncludeNotFoundError(false);
            }
        }
        return false;
    }

//    void InclusionDirective(clang::SourceLocation, const clang::Token&, llvm::StringRef FileName, bool, clang::CharSourceRange, const clang::FileEntry*, llvm::StringRef, llvm::StringRef, const clang::Module*) override {
//        llvm::errs() << "InclusionDirective: " << FileName << "\n";
//    }
};
