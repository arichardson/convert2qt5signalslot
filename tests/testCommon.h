#pragma once

#include <string>
#include <map>
#include <vector>

#include "config-tests.h"

#define BASE_DIR "/the/path/to/be/refactored/"
#define FILE_NAME BASE_DIR "input.cpp"
#define INCLUDE_DIR "/fake/system/include/path/"


std::string readFile(const std::string& name);

typedef std::map<std::string, std::string> AdditionalFiles;

bool codeCompiles(const std::string& code, const AdditionalFiles& additionalFiles = {}, const std::vector<std::string>& extraOptions = {});

int testMain(std::string input, std::string expected, int found, int converted, const std::vector<const char*>& converterOptions = {});

int testMainWithoutCompileCheck(std::string input, std::string expected, int found, int converted, const std::vector<const char*>& converterOptions = {});

