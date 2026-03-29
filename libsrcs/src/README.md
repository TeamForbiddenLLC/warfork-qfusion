# qfusion-libsrcs

This is the source for the statically linked libraries used in Qfusion, with the settings we use for building them.

## Compiling libcurl
1. open visual studio,  go to the tools menu -> visual studio command prompt
2. in command prompt change current directory to libcurl/lib
3. type:
`nmake /f Makefile.vc10 clean`
4. to build debug configuration, type:
`nmake CFG=debug-zlib /f Makefile.vc10`
alternatively, to build release configuration, type:
`nmake CFG=release-zlib /f Makefile.vc10`
5. locate and copy the libcurl.lib file in either 'debug-zlib' or 'release-zlib' directory

## Compiling libogg
libogg/win32/VS2008/libogg_static.vcproj

## Compiling libvorbis
libvorbis/win32/VS2008/libvorbis/libvorbis_static.vcproj

libvorbis/win32/VS2008/libvorbisfile/libvorbisfile_static.vcproj

## Notes on OpenSSL
Some libcrypto assembly files were pre-converted to make OpenSSL buildable without Perl.

Also the symlink paths in `include` were changed to real `#include` directives.

The build scripts for Android depend on these changes, make sure the above is applied when updating OpenSSL.
