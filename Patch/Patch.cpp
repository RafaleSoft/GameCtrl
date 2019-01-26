// Patch.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h> 
#include <sys/stat.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

unsigned char *search_pattern(unsigned char* buffer, size_t bufferlen, const char* pattern)
{
	if ((NULL == buffer) || (0 == bufferlen) || (NULL == pattern))
		return NULL;
	size_t patternlen = strlen(pattern);
	if (bufferlen < patternlen)
		return NULL;

	unsigned char *result = NULL;
	size_t pos = 0;
	size_t ppos = 0;
	while (pos < bufferlen - patternlen)
	{
		if (buffer[pos++] != pattern[ppos++])
		{
			ppos = 0;
			continue;
		}
		else
		{
			if (ppos == patternlen)
			{
				result = &buffer[pos - patternlen];
				break;
			}
		}
	}

	return result;
}


int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2)
		return -1;

	int retcode = 0;
	const char *filepath = argv[1];

	struct _stat buffer;
	if (0 != _stat(filepath, &buffer))
		retcode = -1;
	else
	{
		FILE *f = NULL;
		errno_t err = fopen_s(&f, filepath, "r+b");
		if (0 != err)
			retcode = -2;
		else if (NULL == f)
			retcode = -2;
		else
		{
			unsigned char *filebuffer = (unsigned char*)malloc(buffer.st_size);
			if (NULL == filebuffer)
				retcode = -3;
			else
			{
				size_t nb_read = fread(filebuffer, 1, buffer.st_size, f);
				if (buffer.st_size != nb_read)
					retcode = -4;
				else
				{
					if (0 != fseek(f, 0, SEEK_SET))
						retcode = -4;
					else
					{
						unsigned char *pattern = search_pattern(filebuffer, buffer.st_size, "A,NCK+\"2=4");
						if (NULL == pattern)
							retcode = -5;
						else
						{
							SYSTEMTIME SystemTime;
							GetSystemTime(&SystemTime);
							FILETIME FileTime;
							SystemTimeToFileTime(&SystemTime, &FileTime);

							ULARGE_INTEGER next;
							next.HighPart = FileTime.dwHighDateTime;
							next.LowPart = FileTime.dwLowDateTime;
							ULARGE_INTEGER delta;
							delta.QuadPart = 15 * 24 * 60 * 60;	// nb of days granted: 15 days of 24 hours of 3600 seconds
							delta.QuadPart *= 1000 * 1000 * 10;	// in 100ns intervals.

							next.QuadPart = next.QuadPart + delta.QuadPart;

							FileTime.dwHighDateTime = next.HighPart;
							FileTime.dwLowDateTime = next.LowPart;
							FileTimeToSystemTime(&FileTime, &SystemTime);

							*((unsigned char*)(pattern + 10)) = (unsigned char)(SystemTime.wDay & 0xff);
							*((unsigned char*)(pattern + 11)) = (unsigned char)(SystemTime.wMonth & 0xff);
							*((unsigned short*)(pattern + 12)) = (unsigned short)(SystemTime.wYear & 0xffff);

							size_t nb_write = fwrite(filebuffer, 1, buffer.st_size, f);
							if (buffer.st_size != nb_write)
								retcode = -6;
							else
							{
								if (0 != fflush(f))
									retcode = -7;
								if (EOF == fclose(f))
									retcode = -7;
							}
						}
					}
				}

				free(filebuffer);
			}
		}
	}

	//printf("resultat: %d\n", retcode);

	return retcode;
}

