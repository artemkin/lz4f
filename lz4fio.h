/*
	LZ4FIO.H
    Copyright (c) 2015 Ray Edward Bornert II

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the 
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
    OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
    Change History:
===============================================================================

2015-MAY-28: Ray Bornert
    Created files lz4lib.h lz4lib.cpp in the zlib gzxxx API style

===============================================================================
*/

/*
	This file is best viewed at tab width = 4
*/

#ifndef _lz4fio_h_
#define _lz4fio_h_

#include <stdio.h>
#include <stddef.h>

typedef enum {
	 lz4f_ok				= +0
	,lz4f_bad_arg			= -1
	,lz4f_bad_header		= -2
	,lz4f_bad_frame			= -3
	//...
	,lz4f_fail_heap			= -10
	,lz4f_fail_open			= -11
	,lz4f_fail_close		= -12
	,lz4f_fail_read			= -13
	,lz4f_fail_write		= -14
	,lz4f_fail_compress		= -15
	,lz4f_fail_decompress	= -16
} lz4f_error_t;

#if _WIN32
extern __declspec( thread ) lz4f_error_t lz4ferr;
#else
extern __thread				lz4f_error_t lz4ferr;
#endif

extern const char* lz4f_version_string;

/*=========================================================
struct lz4c_header_s

History:
    2015-MAY-29 Ray Bornert
        Original coding and comments

=========================================================*/
struct lz4c_header_s
{
	// http://github.com/Cyan4973/lz4/blob/master/lz4_Frame_format.md

    //////////////////////////////////////////////////////
    // LONG LONG 0
    //////////////////////////////////////////////////////
    //----------------------------------------------------
    // LONG 0  
    //----------------------------------------------------
    // CANONICAL SIGNATURE BYTES (CS)
    unsigned char    hex04;    // signature 0x04
    unsigned char    hex22;    // signature 0x22
    unsigned char    hex4D;    // signature 0x4D
    unsigned char    hex18;    // signature 0x18
    //----------------------------------------------------

    //----------------------------------------------------
    // LONG 1
    //----------------------------------------------------
    // FRAME FLAGS BYTE (FLG)
    unsigned char    rffu0            :2; // bits 1-0
    unsigned char    c_checksum       :1; // bit    2
    unsigned char    c_size           :1; // bit    3
    unsigned char    b_checksum       :1; // bit    4
    unsigned char    b_independent    :1; // bit    5
    unsigned char    version          :2; // bits 7-6

    // BLOCK DESCRIPTOR BYTE (BD)
    unsigned char    rffu1            :4; // bits 3-0
    unsigned char    b_maxsize        :3; // bits 6-4
    unsigned char    rffu2            :1; // bit    7

    // HEADER CHECKSUM BYTE (HC)
    unsigned char    hc;

    // RESERVED FOR FUTURE USE BYTE (RFFU)
    unsigned char    rffu;
    //----------------------------------------------------
    //////////////////////////////////////////////////////

    //////////////////////////////////////////////////////
    // LONG LONG 1
    //////////////////////////////////////////////////////
    // CONTENT SIZE LONG LONG (CZ)
    unsigned long long    content_size;
    //////////////////////////////////////////////////////

};  // end lz4c_header_s

#pragma pack(push,4)
struct lz4f_header_s
{
	// lz4f canonical signature
	char	chL;	// signature 'L' hex 0x4C
	char	chZ;	// signature 'Z' hex 0x5A
	char	ch4;	// signature '4' hex 0x34
	char	chf;	// signature 'f' hex 0x66

	lz4c_header_s lz4c;

	bool is_valid_header_signature() const
	{
		return(true
			&& 'L' == chL
			&& 'Z' == chZ
			&& '4' == ch4
			&& 'f' == chf
			&& 0x04 == lz4c.hex04
			&& 0x22 == lz4c.hex22
			&& 0x4D == lz4c.hex4D
			&& 0x18 == lz4c.hex18
		);
	}
};
#pragma pack(pop)


struct lz4f_buffers_s;
struct lz4File_s
{
	FILE* fp; // standard C-RTL FILE
	lz4f_buffers_s* pb; // lz4f buffers
	lz4f_header_s h; // lz4f header 
};
typedef lz4File_s* lz4File;

///////////////////////////////////////////////////////////////////////////////
/*
	lz4File lz4open ( const char * fname, const char * fmode );

	fname: standard path to target file
	fmode: standard gzip style file mode with restrictions
			valid fmode strings are:
			"r" "rb"
			"w" "wb"
			"w0" "w1" "w2" "w3" "w4" 
			"w5" "w6" "w7" "w8" "w9" 
			For the "wN" modes the value of N is passed directly
			to the lz4 compression function and the interpretation
			is entirely subject to the lz4 source code.
			The default value of N is 9 (high compression) for
			modes "w" and "wb"

	Return value:
		On error, NULL is returned and lz4ferr contains details.
		On success, a heap allocated lz4File struct is returned
*/
lz4File lz4open ( const char * fname, const char * fmode );
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/*
	int lz4close	( lz4File f );

	f	: a valid lz4File structure returned by lz4open

	Return value:
		On error, return value is negative and lz4ferr contains details.
		On success, the return value is 0
*/
int lz4close	( lz4File f );
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/*
	size_t lz4write	( lz4File f, const void* pbytes, const size_t nbytes );

	f		: a valid lz4File structure returned by lz4open
	pbytes	: pointer to readable destination buffer
	nbytes	: the number of bytes to write to file f

	Return value:
		On error, return value is 0 and lz4ferr contains details.
		On success, return value is nbytes and lz4ferr = lz4f_ok
*/
size_t lz4write	( lz4File f, const void* pbytes, const size_t nbytes );
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/*
	size_t lz4read	( lz4File f, void* pbytes, const size_t nbytes );

	f		: a valid lz4File structure returned by lz4open
	pbytes	: pointer to writable destination buffer
	nbytes	: the number of bytes to read from file f

	Return value:
		On error, return value is 0 and lz4ferr contains details.
		On success, return value is nbytes and lz4ferr = lz4f_ok

*/
size_t lz4read	( lz4File f, void* pbytes, const size_t nbytes );
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/*
	char * lz4gets	( lz4File f, char *pbytes, const size_t nbytes );

	f		: a valid lz4File structure returned by lz4open
	pbytes	: pointer to writable destination buffer
	nbytes	: the byte length of the pbytes buffer

	Return value:
		On error, return value is NULL and lz4ferr contains details.
		On success, return value is pbytes and lz4ferr = lz4f_ok

	lz4gets reads a maximum of nbytes-1 from file f but will stop
	reading if the line-feed LF character decimal 10 (hex 0xA) is
	encountered.  In either case the buffer is guaranteed to be 
	NULL terminated with a single 0 byte.

*/
char * lz4gets	( lz4File f, char *pbytes, const size_t nbytes );
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/*
	int lz4eof		( lz4File f );

	f		: a valid lz4File structure returned by lz4open

	Return value:
		On error, return value is negative and lz4ferr contains details.
		On success, return value is zero when end-of-file is false and 
			non-zero when end-of-file is true.

*/
int lz4eof		( lz4File f );
///////////////////////////////////////////////////////////////////////////////

#endif