#include "Qt5SignalSlotSyntaxConverter.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/Signals.h>

using namespace clang::tooling;

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);

int main(int argc, const char* argv[]) {
  CommonOptionsParser options(argc, argv);
  llvm::sys::PrintStackTraceOnErrorSignal();
  RefactoringTool tool(options.getCompilations(), options.getSourcePathList());
  std::vector<std::string> refactoringFiles;
  for (const std::string& s : options.getSourcePathList()) {
      refactoringFiles.push_back(getRealPath(s));
  }

  MatchFinder matchFinder;
  ConnectConverter converter(&tool.getReplacements(), refactoringFiles);
  converter.setupMatchers(&matchFinder);
  int retVal = tool.runAndSave(newFrontendActionFactory(&matchFinder, converter.sourceCallback()));
  llvm::outs() << "\n";
  converter.printStats();
  return retVal;
}
