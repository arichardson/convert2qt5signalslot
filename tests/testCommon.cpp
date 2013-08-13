#include "testCommon.h"

#include "../Qt5SignalSlotSyntaxConverter.h"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Rewrite/Frontend/FixItRewriter.h>
#include <llvm/Support/Signals.h>

#include <boost/algorithm/string/split.hpp>

using namespace clang::tooling;
using llvm::errs;

#define FILE_NAME "/this/path/does/not/exist/test-input.cpp"

typedef std::vector<std::string> StringList;
static const auto isNewline = [](char c) { return c == '\n'; };


int testMain(std::string input, std::string expected) {
  llvm::sys::PrintStackTraceOnErrorSignal();
  StringList refactoringFiles{FILE_NAME};

  MatchFinder matchFinder;
  Replacements replacements;
  ConnectConverter converter(&replacements, refactoringFiles);
  converter.setupMatchers(&matchFinder);
  auto factory = newFrontendActionFactory(&matchFinder);
  auto action = factory->create();
  //FIXME these are system dependent, fix it
  StringList args{ "-I/usr/lib64/clang/3.3/include", "-I/usr/include/qt5/QtCore", "-I/usr/include/qt5/", "-fPIE" };
  bool success = clang::tooling::runToolOnCodeWithArgs(action, input, args, FILE_NAME);
  if (!success) {
      errs() << "Failed to run tool!\n";
      return 1;
  }
  llvm::outs() << "\n";
  converter.printStats();
  llvm::outs().flush();
  ssize_t offsetAdjust = 0;
  for (const Replacement& rep : replacements) {
      if (rep.getFilePath() != FILE_NAME) {
          llvm::errs() << "Attempting to write to illegal file:" << rep.getFilePath() << "\n";
          success = false;
      }
      input.replace(size_t(offsetAdjust + ssize_t(rep.getOffset())), rep.getLength(), rep.getReplacementText().str());
      offsetAdjust += ssize_t(rep.getReplacementText().size()) - ssize_t(rep.getLength());
  }
  StringList expectedLines;
  boost::split(expectedLines, expected, isNewline);
  StringList resultLines;
  boost::split(resultLines, input, isNewline);
  llvm::outs() << "Comparing result with expected result...\n";
  llvm::outs().flush();
  auto lineCount = std::min(resultLines.size(), expectedLines.size());
  for (size_t i = 0; i < lineCount; ++i) {
      if (expectedLines[i] != resultLines[i]) {
          ((((llvm::errs() << i << " expected:").changeColor(llvm::raw_ostream::GREEN, true) << expectedLines[i]).resetColor() << "\n"
                           << i << " result  :").changeColor(llvm::raw_ostream::RED, true) << resultLines[i]).resetColor() << "\n";
          success = false;
      }
      else {
          llvm::errs() << i << "  okay   :" << expectedLines[i] << "\n";
      }
  }
  if (resultLines.size() > lineCount) {
      success = false;
      llvm::errs() << "Additional lines in result:\n";
      for (size_t i = lineCount; i < resultLines.size(); ++i) {
          llvm::errs() << i << " = '" << resultLines[i] << "'\n";
      }
  }
  if (expectedLines.size() > lineCount) {
      success = false;
      llvm::errs() << "Additional lines in result:\n";
      for (size_t i = lineCount; i < expectedLines.size(); ++i) {
          llvm::errs() << i << " = '" << expectedLines[i] << "'\n";
      }
  }
  return success ? 0 : 1;
}
