convert2qt5signalslot
=====================

Convert from Qt4 signal/slot syntax to the new Qt5 style

Usage
-----


`convert2qt5signalslot -p <path-to-json-compile-db> <source-file>`

The JSON compilation Database can be created using e.g. `cmake -DCMAKE_EXPORT_COMPILE_COMMMANDS=1`.

It will overwrite your source files so make a backup or have them in a VCS!

NOTE: It should actually allow passing more than one source file, but then for some reason the replacements are not actually performed, only the matches are list.
I don't know whether this is an issue with my code, or whether it is a clang issue

To convert a whole source tree you can use the following command:
`find <dir> -iname '*.cpp' -print0 | xargs -0 -n 1 convert2qt5signalslot -p <path-to-json-compile-db>`
Once the issue with more than one source file is fixed you can drop the `-n 1` part.

Known issues
------------

###stddef.h not found

If it complains that stddef.h is not found you should make sure that the compiler builtin headers are found in `$PREFIX/lib/clang/<version>/include`.
If that is not the case you can do e.g. `ln -s /usr/lib64/clang /usr/local/lib64/clang`

###Namespaces not supported yet
If you had a call like `connect(obj, SIGNAL(foo()), SLOT(onFoo())` and the type of `obj` is in a namespace (i.e. `MyNS::MyClass`) then the generated code will be `connect(obj, &MyClass::foo, this, &MyClass::onFoo)`. This may fail to compile unless you have a using directive for that class or namespace.

This issue will be resolved once I find out which namespaces/classes have a using directive, so that it won't add the namespace if not necessary.

###Overloaded signals/slots not supported yet
You will have to resolve thos cases manually until I add that feature

See the note at http://qt-project.org/doc/qt-5.0/qtcore/signalsandslots.html for how to resolve that
