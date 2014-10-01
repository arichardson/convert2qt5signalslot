#include "Qt5SignalSlotSyntaxConverter.h"

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm/Support/Signals.h>

using namespace clang::tooling;

static llvm::cl::extrahelp common_help(CommonOptionsParser::HelpMessage);
extern llvm::cl::OptionCategory clCategory;



int main(int argc, const char* argv[]) {
  CommonOptionsParser options(argc, argv, clCategory);
  llvm::sys::PrintStackTraceOnErrorSignal();
  RefactoringTool tool(options.getCompilations(), options.getSourcePathList());
  std::vector<std::string> refactoringFiles;
  for (const std::string& s : options.getSourcePathList()) {
      refactoringFiles.push_back(getRealPath(s));
  }

  MatchFinder matchFinder;
  ConnectConverter converter(&tool.getReplacements(), refactoringFiles);
  converter.setupMatchers(&matchFinder);
  auto frontend = newFrontendActionFactory(&matchFinder, converter.sourceCallback());
  int retVal = tool.runAndSave(frontend.get()); // TODO: does this take ownership?
  llvm::outs() << "\n";
  converter.printStats();
  return retVal;
}
