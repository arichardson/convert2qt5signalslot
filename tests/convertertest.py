#!/usr/bin/env python

from __future__ import print_function, with_statement

import os, sys, atexit, shutil, tempfile
import subprocess
import plumbum
from plumbum import cmd, FG, local;

print(sys.argv)

program = sys.argv[1]
testfile = sys.argv[2]

print("Testfile is", testfile)
not os.path.isabs(testfile) or sys.exit(testfile + " must not be absolute")

originalFile = os.path.abspath(testfile + ".original")
refactoredFile = os.path.abspath(testfile + ".refactored")

os.path.exists(originalFile) or sys.exit(originalFile + " does not exist")
os.path.exists(refactoredFile) or sys.exit(refactoredFile + " does not exist")

#create a temporary dir that is deleted at exit
testDir = tempfile.mkdtemp(prefix = "convertqt5_")
sourceDir = os.path.abspath(os.getcwd())
atexit.register(shutil.rmtree, testDir)

#copy to test dir
convertFile = os.path.join(testDir, testfile);

shutil.copyfile(originalFile, convertFile)
with open(os.path.join(testDir, "CMakeLists.txt"), "a") as cmakelist:
    cmakelist.write("cmake_minimum_required(VERSION 2.8.11)\n");
    cmakelist.write("find_package(Qt5Widgets)\n");
    #adapt the next line to your compiler as needed
    cmakelist.write("include_directories(/usr/lib64/gcc/x86_64-suse-linux/4.7/include/ /usr/lib64/gcc/x86_64-suse-linux/4.7/include-fixed/)\n"); #hack for the builtin stddef.h
    cmakelist.write("add_executable(test " + testfile + ")\n");
    cmakelist.write("target_link_libraries(test Qt5::Widgets)\n");



cmd.cmake.run(["-DCMAKE_EXPORT_COMPILE_COMMANDS=1", "."], cwd=testDir)
signalSlotConverter = plumbum.local[program];
signalSlotConverter["-p", testDir, convertFile] & FG

print("Comparing to expected result...")
#diff will fail with exit code 1 if files differ, so just running this is enough
diff = cmd.diff("-u", refactoredFile, convertFile)



print("Test passed!")



