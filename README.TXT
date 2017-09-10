/*

===============================================================================
README.TXT for the open source project LZ4FIO
===============================================================================

===============================================================================
This README document was created on: 2015-JUN-01 Monday
===============================================================================

The LZ4FIO files
===============================================================================
 README.TXT
 lz4fio.cpp
 lz4fio.h
 test.cpp
 liblz4f.vcproj
 makefile
===============================================================================

The LZ4FIO license
===============================================================================
The LZ4FIO open source project is the creation of Ray Edward Bornert II.
The relevant source code license can be found inside LICENSE.TXT and in
source code files lz4fio.cpp and lz4fio.h.
===============================================================================

The LZ4FIO purpose
===============================================================================
LZ4FIO is developer friendly C/C++ source code wrapper for the LZ4 codec.
The LZ4 codec files are contained in the folder lz4 as provided by the 
open source work of Yann Collet.  LZ4FIO provides an API similar to
STDIO and ZLIB which are quite familiar to C/C++ developers everywhere.
===============================================================================

The LZ4 codec source code
===============================================================================
The official LZ4 codec source code can be found here:
https://github.com/Cyan4973/lz4
===============================================================================

Building LZ4FIO
===============================================================================
The LZ4IO source code files require source code access to the following 
lz4 codec source code files:

lz4/lz4.h
lz4/lz4.c
lz4/lz4hc.h
lz4/lz4hc.c
lz4/xxhash.h
lz4/xxhash.c

Both the makefile and the Visual Studio project expect the lz4 folder to exist 
and to contain these files.  All build attempts will fail otherwise.

The LZ4FIO project includes the lz4 codec source code files by default as a 
simple convenience.  If you have only lz4fio.cpp and lz4fio.h then you must
go download a working copy of the LZ4 codec source code files listed above.  
Note that the LZ4 codec normally contains more files than are listed above and
it is ok to download the entire set of project files if that makes it easier.
Just know that you must acquire those listed above at a minimum.  You can and
should update your LZ4 codec source code files periodically according to the 
suggestions of the LZ4 codec developer(s).
===============================================================================

Building LZ4FIO for windows
===============================================================================
The file liblz4f.vcproj is a Visual Studio v9 standard project file with 
no pathing dependencies other then the lz4 source code folder already present
inside the proejct.  Simply open this project directly or add it to an existing
solution and build it and use it as you would any other windows static library.
===============================================================================

Building LZ4FIO for gnu compilers
===============================================================================
At any standard gnu g++/c++ compiler environment do:

> make clean
> make

and you will create the target binary liblz4f.a

To build the simple test program provided do:

> make test

and then run test with

> ./test
===============================================================================

Pure binaries
===============================================================================
If you already have a pre-built LIBLZ4F static library binary

Then you need only include the lz4fio.h header in your source code

#include "lz4fio.h"

And you need only link to static libary file:

liblz4f.a		(gnu/linux/osx)
liblz4f.lib		(windows)
===============================================================================

Porting to other operating systems
===============================================================================
Porting LZ4FIO to another operating system is easy. You need only supply the 
appropriate OS specific routines:

size_t get_page_size();
bool set_page_lock( unsigned char* a, const bool v );

You can find the source code for these functions at the bottom of lz4fio.cpp.
If your operating system and/or compiler environ does not offer page locking
then your only choice is to simply provide empty stub functions.

You also must satisfy your compiler syntax for thread safe allocation for the
per thread error code:

lz4f_error_t lz4ferr;

If your compiler is C++11 compliant then you can use the keyword thread_local.

extern thread_local lz4f_error_t lz4ferr;
thread_local lz4f_error_t lz4ferr;
===============================================================================

Bugs and Fixes and Wishes
===============================================================================
Please direct all bug reports and source code fixes and wishes to:
support@hixoxih.com
===============================================================================

Financial Support
===============================================================================
If you appreciate what I am doing and you would like to express yourself with
actual real world money, then I will be happy to accept your paypal donation
to the paypal account: support@hixoxih.com
===============================================================================

*/
