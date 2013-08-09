convert2qt5signalslot
=====================

Convert from Qt4 signal/slot syntax to the new Qt5 style

Usage
-----


`convert2qt5signalslot -p <path-to-json-compile-db> <source1> <source2> ...`

The JSON compilation Database can be created using e.g. `cmake -DCMAKE_EXPORT_COMPILE_COMMMANDS=1`.

It will overwrite your source files so make a backup or have them in a VCS!

stddef.h not found
------------------
If it complains that stddef.h is not found you should make sure that the compiler builtin headers are found in `$PREFIX/lib/clang/<version>/include`.
If that is not the case you can do e.g. `ln -s /usr/lib64/clang /usr/local/lib64/clang`
