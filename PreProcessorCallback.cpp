#include "PreProcessorCallback.h"

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


ConverterPPCallbacks::~ConverterPPCallbacks()
{
}


void ConverterPPCallbacks::addQ_PRIVATE_SLOT_definition(clang::SourceLocation origLoc)
{
    clang::SourceLocation location = pp.getSourceManager().getFileLoc(origLoc);
    pp.EnterSourceFile(pp.getSourceManager().createFileID(llvm::MemoryBuffer::getMemBuffer(Q_PRIVATE_SLOT_definition, "q_private_slot_defintion.moc"),
            clang::SrcMgr::C_System, 0, 0, location), nullptr, location);
}

void ConverterPPCallbacks::MacroDefined(const clang::Token& token, const clang::MacroDirective*) {
    llvm::StringRef macroName = token.getIdentifierInfo()->getName();

#if 0 // enable to debug macro definitions
    if (!macroName.startswith("__")) { // hide the builtin macros
        std::string location = token.getLocation().printToString(pp.getSourceManager());
        llvm::errs() << location << ": " << macroName << " = ";
        pp.DumpMacro(*md->getMacroInfo());
    }
#endif
    if (macroName == "Q_PRIVATE_SLOT") {
        // llvm::errs() << "Q_PRIVATE_SLOT defined at " << md->getLocation().printToString(pp.getSourceManager()) << "\n";
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

void ConverterPPCallbacks::handleMacroUndefined(const clang::Token& token) {
    if (token.getIdentifierInfo()->getName() == "Q_PRIVATE_SLOT") {
        // llvm::errs() << "Q_PRIVATE_SLOT undefined at " << token.getLocation().printToString(pp.getSourceManager()) << "\n";
        if (!addingQ_PRIVATE_SLOT_defintion) {
            pp.getDiagnostics().Report(token.getLocation(), undefWarningDiagId) << "Q_PRIVATE_SLOT";
            // we have to redefine it to what it should be
            addQ_PRIVATE_SLOT_definition(token.getLocation());
        }
    }
}

void ConverterPPCallbacks::FileChanged(clang::SourceLocation loc, clang::PPCallbacks::FileChangeReason reason, clang::SrcMgr::CharacteristicKind, clang::FileID prevFID) {
    Q_UNUSED(prevFID)
    clang::PresumedLoc presumedLoc = pp.getSourceManager().getPresumedLoc(loc, false);
    // llvm::errs() << "File changed: name=" << presumedLoc.getFilename() << ", reason=" << reason << "\n";
    if (strcmp(presumedLoc.getFilename(), "q_private_slot_defintion.moc") == 0) {
        if (reason == EnterFile) {
            addingQ_PRIVATE_SLOT_defintion = true;
        }
        else if (reason == ExitFile) {
            addingQ_PRIVATE_SLOT_defintion = true;
        }
    }
}

bool ConverterPPCallbacks::FileNotFound(StringRef FileName, llvm::SmallVectorImpl<char>& RecoveryPath) {
    Q_UNUSED(RecoveryPath)
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

