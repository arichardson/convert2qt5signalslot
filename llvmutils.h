#ifndef LLVMUTILS_H_
#define LLVMUTILS_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>
#include <clang/AST/Stmt.h>

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

/** find the first child of @p stmt or with type @p T or null if none found */
template<class T>
static const T* findFirstChildWithType(const clang::Stmt* stmt) {
    if (!stmt) {
        return nullptr;
    }
    using namespace llvm;
    for (auto it = stmt->child_begin(); it != stmt->child_end(); ++it) {
        const T* ret = dyn_cast<T>(*it);
        if (ret) {
            return ret;
        }
        // try children now
        ret = findFirstChildWithType<T>(*it);
        if (ret) {
            return ret;
        }
    }
    return nullptr;
}

#endif /* LLVMUTILS_H_ */
