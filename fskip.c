/*----------------------------------------------------------------
 * skip file position
 *----------------------------------------------------------------*/

#include <stdio.h>

void fskip(int size, FILE *fd, int seekable)
{
	if (seekable)
		fseek(fd, size, SEEK_CUR);
	else {
		char tmp[1024];
		while (size >= sizeof(tmp)) {
			fread(tmp, 1, sizeof(tmp), fd);
			size -= sizeof(tmp);
		}
		if (size > 0)
			fread(tmp, 1, size, fd);
	}
}


