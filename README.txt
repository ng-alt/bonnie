To compile for OS/2 type "nmake -f Makefile.os2".

To compile for NT type "nmake -f Makefile.nt".

To build a Solaris package type "make -C sun".

To build a Debian package type "dpkg-buildpackage".

To build a RPM type "".

To compile for any other Unix type "./configure" and then type "make" after
the configure script successfully completes.  If ./configure produces errors
then send me your config.log file (don't bother trying to compile it until
you have ./configure run correctly).

Unix compilation currently requires GCC and GNU Make.  You should be able to
compile with a non-GCC compiler by running the following command:
CFLAGS="..." make
Where ... represents the compilation flags for your C++ compiler.
