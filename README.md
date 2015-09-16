convert2qt5signalslot
=====================

Convert from Qt4 signal/slot syntax to the new Qt5 style


Installation
------------

Packages for openSUSE 13.2 and openSUSE Tumbleweed are available at http://download.opensuse.org/repositories/home:/a_richardson/, otherwise you will have to build from source.

Assuming you have the development files for clang and Qt5 installed the following commands should produce a valid installation:
```
mkdir build
cd build
cmake ..
make
sudo make install
```

Usage
-----


`convert2qt5signalslot -p <path-to-json-compile-db> <source-file>`

The JSON compilation Database can be created using e.g. `cmake -DCMAKE_EXPORT_COMPILE_COMMMANDS=1`.

It will overwrite your source files so make a backup or have them in a VCS!

NOTE: It should actually allow passing more than one source file, but then for some reason the replacements are not actually performed, only the matches are list.
I don't know whether this is an issue with my code, or whether it is a clang issue

To convert a whole source tree you can use the following command:
`find <dir> -iname '*.cpp' -print0 | xargs -0 -n1 -P4 convert2qt5signalslot -p <path-to-json-compile-db>`

The `-n1` makes sure that only one file is converted by each `convert2qt5signalslot` process, since one compiler error will make conversions in all other files fail.
The `-P4` options ensures that 4 processes are run in parallel. Adapt this to the number of cores you have.
Having the `-print0` option with `find` and the `-0` with `xargs` makes sure that paths with spaces and other special characters are handled properly.


Known problems
------------

###stddef.h not found

This should be fixed in the current version since `"<CLANG_LIBRARY_DIR>/clang/<CLANG_VERSION_STRING>/include"` gets added to the include paths automatically.

It will break if you compile convert2qt5signalslot on one system and then run it on another where CLANG_LIBRARY_DIR does not match.

In order to fix this you should make sure that the compiler builtin headers are found in `<path-to-tool-binary>/../lib/clang/<version>/include`.
If that is not the case you can do e.g. `ln -s /usr/lib64/clang $PREFIX/lib/clang`. Note that clang tools always search in `lib` even if the
distribution uses `lib64` (This has been fixed in SVN recently).

