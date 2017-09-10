#include "lz4fio.h"
#include <string.h>

int main( int argc, char* argv[] )
{
	char utext[128];utext[0]=0;
	sprintf(utext,"Hello from lz4fio %s\n", lz4f_version_string );

	const char *fnhw="hw.lz4";
	lz4File f = lz4open(fnhw,"wb");
	if(NULL==f)
	{
		printf("lz4open(%s,wb) failed with error %d\n",fnhw,lz4ferr);
		return 0;
	}

	printf("uncompressed text: %s",utext);
	size_t zz = strlen(utext);
	size_t zw = lz4write( f, utext, zz );
	if(zz!=zw)
	{
		printf("lz4write failed with error %d\n",lz4ferr);
		lz4close(f);
		return 0;
	}
	lz4close(f);

	f = lz4open(fnhw,"rb");
	if(NULL==f)
	{
		printf("lz4open(%s,rb) failed with error %d\n",fnhw,lz4ferr);
		return 0;
	}

	char dtext[128];dtext[0]=0;
	zz = sizeof(dtext);
	lz4gets(f,dtext,zz);
	if(0==zz)
	{
		printf("lz4gets failed with error %d\n",lz4ferr);
		lz4close(f);
		return 0;
	
	}
	lz4close(f);
	printf("decompressed text: %s",dtext);
	if(0==strncmp(utext,dtext,zz))
		printf("success!\n");
	else
		printf("error: decompressed text does not match original uncompressed text\n");

	return 0;

}


