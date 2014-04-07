#ifndef LLVMUTILS_H_
#define LLVMUTILS_H_

#include <llvm/Support/raw_ostream.h>

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
private:
    llvm::raw_ostream& stream;
};

static inline ColouredOStream colouredOut(llvm::raw_ostream::Colors colour, bool bold = true) {
    return ColouredOStream(llvm::outs(), colour, bold);
}

static inline ColouredOStream colouredErr(llvm::raw_ostream::Colors colour, bool bold = true) {
    return ColouredOStream(llvm::errs(), colour, bold);
}


#endif /* LLVMUTILS_H_ */
