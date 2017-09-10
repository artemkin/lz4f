/*
	LZ4FIO.CPP
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

////////////////////////
// LZ4 base headers
#include "lz4/lz4.h"
#include "lz4/lz4hc.h"
#include "lz4/xxhash.h"
////////////////////////

////////////////////
// LZ4FIO header
#include "lz4fio.h"
////////////////////

////////////////////
// C-RTL headers
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
////////////////////

//////////////////////////////////////////////////////
// os specific functions
bool set_page_lock( unsigned char* a, const bool v );
size_t get_page_size();
//////////////////////////////////////////////////////

///////////////////////////////////////////
// the lz4f error code
#if _WIN32
__declspec( thread ) lz4f_error_t lz4ferr;
#else
__thread             lz4f_error_t lz4ferr;
#endif
///////////////////////////////////////////

///////////////////////////////////////////
// the lz4f version string
const char* lz4f_version_string = "v0.0a";
///////////////////////////////////////////

/*=========================================================
lz4c_header_s lz4c_init_header()

History:
    2015-MAY-29 Ray Bornert
        Original coding and comments
=========================================================*/
lz4c_header_s lz4c_init_header()
{
    lz4c_header_s h;

    // CANONICAL SIGNATURE (CS)
    h.hex04            = 0x04;    // canonical hex signature
    h.hex22            = 0x22;    // canonical hex signature
    h.hex4D            = 0x4D;    // canonical hex signature
    h.hex18            = 0x18;    // canonical hex signature

    // FRAME FLAGS BYTE (FLG)
    h.rffu0            = 0;    // 2 bits   - default is 0
    h.c_checksum       = 0;    // 1 bit    - default is no content checksum suffix
    h.c_size           = 0;    // 1 bit    - default is no content size in frame
    h.b_checksum       = 0;    // 1 bit    - default is no block checksum
    h.b_independent    = 1;    // 1 bit    - default is block independent
    h.version          = 1;    // 2 bits   - default is version 1

    // BLOCK DESCRIPTOR BYTE (BD)
    h.rffu1            = 0;    // 4 bits    - default is 0
    h.b_maxsize        = 4;    // 3 bits    - default is 64kb
    h.rffu2            = 0;    // 1 bit     - default is 0

    // HEADER CHECKSUM BYTE (HC)
    h.hc               = 0;    // 1 byte    - default is no header checksum

    // RESERVED FOR FUTURE USE BYTE (RFFU)
    h.rffu             = 0;    // 1 byte    - default is 0

    // CONTENT SIZE LONG LONG (CZ)
    h.content_size     = 0;    // 8 bytes   - default is 0 

    return h;

}   // end lz4c_init_mandatory

/*=========================================================
int lz4c_read_header( FILE* fp, lz4c_header_s* ph )

History:
    2015-MAY-29 Ray Bornert
        Original coding and comments
=========================================================*/
int lz4c_read_header( FILE* fp, lz4c_header_s* ph )
{
    size_t zresult;
    lz4c_header_s H;
    lz4c_header_s h;
    assert( NULL!=fp );
    assert( NULL!=ph );
    H = lz4c_init_header();
    h = lz4c_init_header();
    zresult = fread( &h, 6, 1, fp );
    if(false
        || 1!=zresult
        || 0!=memcmp(&H,&h,4)
    )
    {
        // on fail file read canonical signature 4 bytes
        return -1;
    }

    if(1==h.c_size)
    {
        zresult = fread( &h.content_size, 8, 1, fp );
        if(1!=zresult)
        {
            // on fail file read content size 8 bytes
            return -2;
        }
    }

    zresult = fread( &h.hc, 1, 1, fp );
    if(1!=zresult)
    {
        // on fail file read header checksum 1 byte
        return -3;
    }

    *ph = h;

    // on success
    return 0;

}   // end lz4c_read_header


/*
https://github.com/Cyan4973/lz4/blob/master/lz4_Frame_format.md

General Structure of LZ4c Frame format
--------------------------------------
4		BYTES:	Canonical signature
12		BYTES:	Frame Descriptor
Block
(...)
4		BYTES:	EndMark
4		BYTES:	Final Checksum
--------------------------------------

Magic Number
------------
4 Bytes, Little endian format. Value : 0x184D2204

Frame Descriptor
----------------
12 bytes
Details are stored in the descriptor

Data Blocks
-----------
Compressed data is stored inside the data blocks

EndMark
-------
The flow of blocks ends when the last data block has a size of “0”. 
The size is expressed as a 32-bits value.

Content Checksum
----------------
Content Checksum verify that the full content has been decoded correctly. 
The content checksum is the result of xxh32() hash function digesting the original (decoded) data as input, 
and a seed of zero. 
Content checksum is only present when its associated flag is set in the frame descriptor. 
Content Checksum validates the result, 
that all blocks were fully transmitted in the correct order and without error, 
and also that the encoding/decoding process itself generated no distortion. 
Its usage is recommended.

LZ4c Frame Descriptor
-------------------------------
1 byte		: FLG
1 byte		: BD
1 byte		: HC
1 byte		: RFFC
8 bytes		: Content Size
-------------------------------
The descriptor uses a minimum of 3 bytes, 
and up to 11 bytes depending on optional parameters.

FLG byte layout
---------------------------------------------------------------
7-6       5         4            3        2            1-0
---------------------------------------------------------------
Version   B.Indep   B.Checksum   C.Size   C.Checksum   Reserved
---------------------------------------------------------------

BD byte layout
----------------------------------
7          6-5-4         3-2-1-0
----------------------------------
Reserved   Block MaxSize Reserved 
----------------------------------

===================================================================================
FLG BYTE FIELDS
====================================================================================
Version Number  FLG BITS 7-6
----------------------------
2-bits field, must be set to “01”. 
Any other value cannot be decoded by this version of the specification. 
Other version numbers will use different flag layouts.

Block Independence flag  FLG BIT 5
----------------------------------
If this flag is set to “1”, blocks are independent. 
If this flag is set to “0”, each block depends on previous ones (up to LZ4 window size, which is 64 KB). 
In such case, it’s necessary to decode all blocks in sequence.
Block dependency improves compression ratio, especially for small blocks. 
On the other hand, it makes direct jumps or multi-threaded decoding impossible.

Block checksum flag   FLG BIT 4
-------------------------------
If this flag is set, 
each data block will be followed by a 4-bytes checksum, 
calculated by using the xxHash-32 algorithm on the raw (compressed) data block. 
The intention is to detect data corruption (storage or transmission errors) immediately, 
before decoding. 
Block checksum usage is optional.

Content Size flag  FLG BIT 3
----------------------------
If this flag is set, 
the uncompressed size of data included within the frame will be present as 
an 8 bytes unsigned little endian value, 
after the flags. 
Content Size usage is optional.

Content checksum flag   FLG BIT 2
---------------------------------
If this flag is set, 
a content checksum will be appended after the EndMark.
Recommended value : “1” (content checksum is present)

===================================================================================
BD BYTE FIELDS
====================================================================================
Block Maximum Size
------------------
This information is intended to help the decoder allocate memory. 
Size here refers to the original (uncompressed) data size. 
Block Maximum Size is one value among the following table :
 0   1   2   3   4     5    6   7
----------------------------------
N/A N/A N/A N/A 64KB 256KB 1MB 4MB 
----------------------------------

The decoder may refuse to allocate block sizes above a (system-specific) size. 
Unused values may be used in a future revision of the spec. 
A decoder conformant to the current version of the spec is only able to decode blocksizes defined in this spec.

Reserved bits
-------------
Value of reserved bits must be 0 (zero). 
Reserved bit might be used in a future version of the specification, 
typically enabling new optional features. 
If this happens, 
a decoder respecting the current version of the specification shall not be able to decode such a frame.

Content Size
------------
This is the original (uncompressed) size. 
This information is optional, 
and only present if the associated flag is set. 
Content size is provided using unsigned 8 Bytes, 
for a maximum of 16 HexaBytes. 
Format is Little endian. 
This value is informational, 
typically for display or memory allocation. 
It can be skipped by a decoder, 
or used to validate content correctness.

===================================
HEADER CHECKSUM
===================================
Header Checksum
---------------
One-byte checksum of combined descriptor fields, 
including optional ones. 
The value is the second byte of xxh32() :  (xxh32()>>8) & 0xFF  using zero as a seed, 
and the full Frame Descriptor as an input (including optional fields when they are present). 
A wrong checksum indicates an error in the descriptor. 
Header checksum is informational and can be skipped.
*/

lz4f_header_s lz4f_init_header()
{
	lz4f_header_s h;
	h.chL = 'L';
	h.chZ = 'Z';
	h.ch4 = '4';
	h.chf = 'f';
	h.lz4c = lz4c_init_header();
	return h;
}

size_t min( const size_t a, const size_t b )
{
	return (a<=b)?a:b;
}

struct lz4fbuf_s
{
	unsigned char* _heap;
	unsigned char* _buf0;
	unsigned char* _bufi;
	unsigned char* _bufz;

#define BUFSIZE 0x10000

#ifdef _WIN32
	bool set_buffer_wall( const bool pref, const bool suff )
	{
		return(true
			&& set_page_lock( _buf0-8, pref )
			&& set_page_lock( _buf0+BUFSIZE, suff )
		);
	}
#else
	bool set_buffer_wall( const bool pref, const bool suff )
	{
		// add linux and mac support here
	}
#endif

	lz4fbuf_s():_heap(NULL),_buf0(NULL),_bufi(NULL),_bufz(NULL)
	{
		_heap = new unsigned char[ BUFSIZE * 3 ];
		if(NULL==_heap)
		{
			lz4ferr = lz4f_fail_heap;
			return;
		}
		unsigned long long a = (unsigned long long)_heap;
		/*
			position the working _buf0 such that it is
			64k page aligned and there is a minimum of 
			one prefix page allocated between _heap and
			_buf0
		*/
		size_t page_size = get_page_size();

		a += (page_size + BUFSIZE - 1); a = ~a; a |= (BUFSIZE-1); a = ~a;
		//////////////////////////////////////////
		_buf0 = (unsigned char*)a;
		_bufi = _buf0;
		_bufz = _buf0 + BUFSIZE;
		assert( page_size <= (size_t)(_buf0-_heap) );
		set_buffer_wall(true,true);
	}

	void init( const char bmode, const char fmode )
	{
		_bufi = _buf0;
		_bufz = _buf0 + BUFSIZE;
		if('d'==bmode)
		{
			if('r'==fmode)
				_bufz = _buf0;
		}
		else
		if('c'==bmode)
		{
			if('w'==fmode)
			{
				/*
					The act of compression "could" have a negative result
					where the compressed data is larger than the source
					and in this would be a buffer overflow.  We detect 
					this in the result value and take action there by
					setting the not-compressed bit.
				*/
				set_buffer_wall(true,false);
			}
		}
		else
		{
			assert(false);
		}
	}

	~lz4fbuf_s()
	{
		if(NULL!=_heap)
		{
			set_buffer_wall(false,false);
			delete _heap;
			_heap=NULL;
		}
	}
	size_t remaining()const
	{
		return _bufz - _bufi;
	}
	size_t write( void* pbytes, const size_t nbytes )
	{
		size_t rem = min(remaining(),nbytes);
		memcpy( _bufi,pbytes,rem );
		_bufi += rem;
		return rem;
	}
	size_t read( void* pbytes, const size_t nbytes )
	{
		size_t rem = min(remaining(),nbytes);
		memcpy( pbytes,_bufi,rem );
		_bufi += rem;
		return rem;
	}
	size_t gets( char* pbytes, const size_t nbytes )
	{
		char* pfr = pbytes;
		char* pto = pbytes+nbytes-1;
		while ( pfr < pto && _bufi < _bufz )
		{
			if('\n' == (*pfr++ = *_bufi++))
			{
				break;
			}
		}
		*pfr=0;
		return pfr-pbytes;
	}
};

struct lz4f_sizes_s
{
	int d_size;	// uncompressed size
	int c_size;	// compressed size
};

struct lz4f_buffers_s
{
	lz4fbuf_s	c;	// compressed
	lz4fbuf_s	d;	// decompressed
	int complvl;	// compression level
	char fmode;		// 'r' or 'w'

	void init(const char m, const int cl )
	{
		complvl = cl;
		fmode=m;
		c.init('c',m);
		d.init('d',m);
	}

	lz4f_error_t flush( FILE* fp )
	{
		return ('w'==fmode) ? push_w(fp) : lz4f_ok;
	}

	lz4f_error_t push_w( FILE* fp )
	{
		size_t ibytes = d._bufi - d._buf0;
		if(0>=ibytes)
			return lz4f_ok;
		assert( BUFSIZE >= ibytes );
		int iresult = LZ4_compress_HC
		(
			 (const char*) d._buf0
			,(char*) c._buf0
			,(int) ibytes
			,BUFSIZE*3/2
			,complvl
		);
		if(0>=iresult)
			return lz4f_fail_compress;
		size_t obytes = iresult;
		lz4f_sizes_s zz;
		zz.d_size = (int)ibytes;
		zz.c_size = (int)obytes;
		unsigned char* pwbuf = c._buf0;
		if(obytes > BUFSIZE)
		{
			// special case where compression increases size
			pwbuf = d._buf0;
			obytes = ibytes;
			zz.d_size = (int)ibytes;
			zz.c_size = (int)ibytes;
#define NCBIT 0x80000000
			// set the not-compressed bit
			zz.c_size |= NCBIT; 
		}
		if(1!=fwrite( &zz,sizeof(zz),1,fp ))
			return lz4f_fail_write;
		if(1!=fwrite( pwbuf, obytes, 1, fp ))
			return lz4f_fail_write;
		d._bufi=d._buf0;
		return lz4f_ok;
	}

	lz4f_error_t pull_r( FILE* fp )
	{
		lz4f_sizes_s zz;
		if(1!=fread( &zz,sizeof(zz),1,fp ))
			return lz4f_fail_read;

		if(0 == (zz.c_size & NCBIT))
		{
			// normal case is compressed
			if(1!=fread( c._buf0, zz.c_size, 1, fp ))
				return lz4f_fail_read;
			int result = LZ4_decompress_fast
			(
				 (const char*) c._buf0
				,(char*) d._buf0
				,(int) zz.d_size
			);
			if(0>=result)
				return lz4f_fail_decompress;
			if(result != zz.c_size)
				return lz4f_fail_decompress;
		}
		else
		{
			// special case is not compressed
			zz.c_size &= (~NCBIT);
			if( zz.c_size != zz.d_size )
				return lz4f_bad_frame;
			if(1!=fread( d._buf0, zz.c_size, 1, fp ))
				return lz4f_fail_read;
		}

		d._bufi = d._buf0;
		d._bufz = d._buf0 + zz.d_size;
		return lz4f_ok;
	}

	size_t write( FILE* fp, const unsigned char* pbytes, const size_t nbytes )
	{
		unsigned char* pfr = (unsigned char*)pbytes;
		unsigned char* pto = pfr + nbytes;
		while(pfr < pto)
		{
			if(0==d.remaining())
			{
				lz4f_error_t e = push_w(fp);
				if(lz4f_ok != e)
				{
					lz4ferr = e;
					return 0;
				}
			}
			pfr += d.write(pfr,pto-pfr);
		}
		lz4ferr = lz4f_ok;
		return pfr-pbytes;
	}

	size_t read( FILE* fp, unsigned char* pbytes, const size_t nbytes )
	{
		unsigned char* pfr = (unsigned char*)pbytes;
		unsigned char* pto = pfr + nbytes;
		while(pfr < pto)
		{
			if(0==d.remaining())
			{
				lz4f_error_t e = pull_r(fp);
				if(lz4f_ok != e)
				{
					lz4ferr = e;
					break;
				}
			}
			pfr += d.read(pfr,pto-pfr);
		}
		return pfr-pbytes;
	}

	size_t gets( FILE* fp, char* pbytes, const size_t nbytes )
	{
		char* pfr = (char*)pbytes;
		char* pto = pfr + nbytes;
		while(pfr < pto)
		{
			if(0==d.remaining())
			{
				lz4f_error_t e = pull_r(fp);
				if(lz4f_ok != e)
				{
					lz4ferr = e;
					return 0;
				}
			}
			pfr += d.gets(pfr,pto-pfr);
			if(pfr>pbytes)
			{
				if(0==pfr[0])
				if('\n'==pfr[-1])
					break;
			}
		}
		lz4ferr = lz4f_ok;
		return pfr-pbytes;
	}
};

int lz4eof		( lz4File f )
{
	if(NULL==f)
	{
		return lz4ferr = lz4f_bad_arg;
	}
	return feof(f->fp);
}

int lz4close	( lz4File f )
{
	if(NULL==f)
	{
		return lz4ferr = lz4f_bad_arg;
	}
	if(NULL==f->fp)
	{
		return lz4ferr = lz4f_bad_arg;
	}

	if('w'==f->pb->fmode)
	{

		lz4ferr = f->pb->flush(f->fp);

		if(lz4ferr == lz4f_ok)
		{
			unsigned int zero=0;
			size_t result = fwrite(&zero,4,2,f->fp);
			if(2!=result)
			{
				lz4ferr = lz4f_fail_write;
			}
		}

		if(lz4ferr == lz4f_ok)
		{
			f->h.lz4c.c_size = (0<f->h.lz4c.content_size) ? 1 : 0;
			size_t zz = sizeof(f->h);
			f->h.lz4c.hc = 0;
			f->h.lz4c.hc = ((XXH32(&f->h,zz,0))>>8) & 0xff;

			//write the final header
			rewind(f->fp);
			if(1!=fwrite(&(f->h),zz,1,f->fp))
				lz4ferr = lz4f_fail_write;
		}

	}

	fclose(f->fp);
	delete f;
	return lz4ferr;
}

lz4File lz4open (const char * fname, const char * fmode)
{
	if(NULL==fname)
	{
		lz4ferr = lz4f_bad_arg;
		return NULL;
	}

	if(NULL==fmode)
	{
		lz4ferr = lz4f_bad_arg;
		return NULL;
	}

	if(true
		&& 'w' != fmode[0] 
		&& 'r' != fmode[0]
	//	&& 'a' != fmode[0] // not supported
	)
	{
		lz4ferr = lz4f_bad_arg;
		return NULL;
	}

	int compression_level=9;
	if(true
		&&  0  != fmode[1]
		&& 'b' != fmode[1]
	)
	{
		if('w' == fmode[0])
		{
			if(!isdigit(fmode[1]))
			{
				lz4ferr = lz4f_bad_arg;
				return NULL;
			}
			compression_level = fmode[1]-'0';
		}
	}

	FILE* fp = fopen(fname,('w'==fmode[0])?"wb":"rb");
	if(NULL==fp)
	{
		lz4ferr = lz4f_fail_open;
		return NULL;
	}

	lz4f_header_s h = lz4f_init_header();
	assert( true == h.is_valid_header_signature() );

	if('r'==fmode[0])
	{
		size_t result = fread( &h, sizeof(h), 1, fp );
		if(false
			|| 1!=result
			|| false == h.is_valid_header_signature()
		)
		{
			lz4ferr = lz4f_bad_header;
			fclose(fp);
			return NULL;
		}
	}
	else
	if('w'==fmode[0])
	{
		size_t result = fwrite( &h, sizeof(h), 1, fp );
		if(1!=result)
		{
			lz4ferr = lz4f_fail_write;
			fclose(fp);
			return NULL;
		}
	}
	else
	{
		assert(false);
	}

	lz4File f = new lz4File_s;
	if(NULL==f)
	{
		lz4ferr = lz4f_fail_heap;
		fclose(fp);
		return NULL;
	}
	lz4f_buffers_s* pb = new lz4f_buffers_s;
	if(NULL==pb)
	{
		lz4ferr = lz4f_fail_heap;
		delete f;
		fclose(fp);
		return NULL;
	}

	f->fp = fp;
	f->pb = pb;
	f->pb->init(fmode[0],compression_level);
	f->h = h;

	lz4ferr = lz4f_ok;

	return f;
}

size_t lz4write	( lz4File f, const void* pbytes, const size_t nbytes )
{
	if(NULL==f || NULL==pbytes)
	{
		lz4ferr = lz4f_bad_arg;
		return 0;
	}
	if(0==nbytes)
		return 0;

	size_t nw = f->pb->write( f->fp, (const unsigned char*)pbytes, nbytes );
	f->h.lz4c.content_size += nw;
	return nw;
}

size_t lz4read	( lz4File f, void* pbytes, const size_t nbytes )
{
	if(NULL==f || NULL==pbytes)
	{
		lz4ferr = lz4f_bad_arg;
		return 0;
	}
	if(0==nbytes)
		return 0;

	size_t nr = f->pb->read( f->fp, (unsigned char*)pbytes, nbytes );
	return nr;
}

char* lz4gets	( lz4File f, char *pbytes, const size_t nbytes )
{
	if(NULL==f || NULL==pbytes)
	{
		lz4ferr = lz4f_bad_arg;
		return 0;
	}
	if(0==nbytes)
		return 0;

	f->pb->gets( f->fp, pbytes, nbytes );
	return pbytes;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// OPERATING SYSTEM SPECIFIC FUNCTIONS - BEGIN
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

size_t get_page_size()
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
}

bool set_page_lock( unsigned char* a, const bool v )
{
	DWORD prev = 0;
	DWORD next = (v ? PAGE_NOACCESS : PAGE_READWRITE);
	BOOL bresult = VirtualProtect( a, 1, next, &prev );
	return (0!=bresult);
}

#else

// GNU GCC specific functions

#include <unistd.h>
#include <sys/mman.h>
size_t get_page_size()
{
	return getpagesize();
}
bool set_page_lock( unsigned char* a, const bool v )
{
	int iresult=0;
	if(true==v)
		iresult = mlock(a,1);
	else
		iresult = munlock(a,1);
	return (0==iresult);
}

#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// OPERATING SYSTEM SPECIFIC FUNCTIONS - END
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
