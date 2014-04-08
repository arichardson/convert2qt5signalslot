// This file contains all sorts of clang utility functions that I wrote, but are not needed anymore

/** expr->getLocStart() + expr->getLocEnd() don't return the proper location if the expression is inside macro expansion
 * this function fixes this
 */
static inline clang::SourceRange getMacroExpansionRange(const clang::Stmt* expr, clang::ASTContext* context, bool* validMacroExpansion = nullptr) {
    using namespace clang;
    SourceLocation beginLoc;
    const bool isMacroStart = Lexer::isAtStartOfMacroExpansion(expr->getLocStart(), context->getSourceManager(), context->getLangOpts(), &beginLoc);
    if (!isMacroStart)
        beginLoc = expr->getLocStart();
    SourceLocation endLoc;
    const bool isMacroEnd = Lexer::isAtEndOfMacroExpansion(expr->getLocEnd(), context->getSourceManager(), context->getLangOpts(), &endLoc);
    if (!isMacroEnd)
        endLoc = sourceLocationAfterStmt(expr, context);
    if (validMacroExpansion)
        *validMacroExpansion = isMacroStart && isMacroEnd;
    return SourceRange(beginLoc, endLoc);
}
