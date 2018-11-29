#include "Qt5SignalSlotSyntaxConverter.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/Signals.h>

using namespace clang::tooling;

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  CommonOptionsParser options(argc, argv, clCategory);
#if CLANG_VERSION_MAJOR >= 7
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
#else
  llvm::sys::PrintStackTraceOnErrorSignal();
#endif
  RefactoringTool tool(options.getCompilations(), options.getSourcePathList());
  std::vector<std::string> refactoringFiles;
  for (const std::string& s : options.getSourcePathList()) {
      refactoringFiles.push_back(getRealPath(s));
  }

  MatchFinder matchFinder;
#if CLANG_VERSION_MAJOR >= 7
  ConnectConverter converter(tool.getReplacements(), refactoringFiles);
#else
  ConnectConverter converter(&tool.getReplacements(), refactoringFiles);
#endif
  converter.setupMatchers(&matchFinder);
  auto frontend = newFrontendActionFactory(&matchFinder, converter.sourceCallback());
  int retVal = tool.runAndSave(frontend.get()); // TODO: does this take ownership?
  llvm::outs() << "\n";
  converter.printStats();
  return retVal;
}
