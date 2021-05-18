
#include <windows.h>
#include <iostream>
#include "SPAD-correct_internal.h"
#include "SPAD-correct.h"


/**
SPAD_readHeader

Read the header information (metadata) from the beginning of the data stream.
Expect this to be in ICS format, tab separated, starting with "\t\r" and ending in "\rend\t\r"
It isolates this section and saves to a temp file which is loaded by libics as an ics file.
If an open ICS file is provided the extracted history is copied into it.

*/
int SPAD_readHeader(BYTE* data, unsigned long long nBytes, ICS* imagefile)
{
	unsigned long long p = 0;
	BYTE header_end_match[] = { 0x0a, 0x65, 0x6e, 0x64, 0x09, 0x0a }; // "\rend\t\r"
	BYTE first_line_match[] = { 0x09, 0x0a }; // "\t\r"
	BYTE* d = data;
	int found = 0;
	int header_bytes;
	unsigned long long max_bytes_to_search = min(nBytes, 1000000);  // Header must be in the first 1 MB, to stop searching many GB for a header that is not there.

	// isolate the header
	d = data;
	found = 0;
	while (d < data + max_bytes_to_search) {
		if (!memcmp(d, header_end_match, 6)) {
			found = 1;
			break;
		}
		d++;
	}

	if (found) {
		header_bytes = (int)(d - data) + 6;
		//		printf("header bytes = %d\n", header_bytes);
	}
	else {
		printf("SPAD_readHeader: header end not found\n");
		return(-1);
	}

	// Check the beginning, should be "\t\r"
	d = data;
	found = 0;
	while (d < data + header_bytes) {
		if (!memcmp(d, first_line_match, 2)) {
			found = 1;
			break;
		}
		d++;
	}

	if (found) {
		int bytes = (int)(d - data);
		//		printf("first line bytes = %d\n", bytes);
		header_bytes -= bytes;
	}
	else {
		printf("SPAD_readHeader: line start not found\n");
		return(-2);
	}

	// Now d points to start of header, and header_bytes is the length
	// Save to a temporary file (must have ics extension!)
	FILE* fp = NULL;
	char tmpfilename[256], * dot;
	tmpnam_s(tmpfilename);
	dot = strchr(tmpfilename, '.');  // find extension
	*dot = '\0';  // terminate string there
	strcat_s(tmpfilename, ".ics"); // add ics extension

	fopen_s(&fp, tmpfilename, "w");
	fwrite(d, 1, header_bytes, fp);
	fclose(fp);

	// open as ics file
	ICS* headerfile;
	Ics_Error retval = IcsOpen(&headerfile, tmpfilename, "r");
	if (retval != IcsErr_Ok) {
		printf("SPAD_readHeader ERROR: Cannot open temp ics header file for reading.\n");
		goto Error;
	}

	//	printf("sim file open as ics.\n");
	{
		int lines = -1;
		IcsGetNumHistoryStrings(headerfile, &lines);
		//printf("%d history lines\n", lines);

		Ics_HistoryIterator it;
		retval = IcsNewHistoryIterator(headerfile, &it, "");
		if (retval != IcsErr_Ok) {
			printf("SPAD_readHeader ERROR: Could not make new history iterator: %s\n", IcsGetErrorText(retval));
			goto Error;
		}

		int count = 0;
		while (retval == IcsErr_Ok) {
			char value[ICS_LINE_LENGTH];
			char key[ICS_STRLEN_TOKEN];
			retval = IcsGetHistoryKeyValueI(headerfile, &it, key, value);
			if (retval == IcsErr_Ok) {
				if (imagefile != NULL)
					IcsAddHistoryString(imagefile, key, value);

				count++;
				//				printf("%d %s: %s\n", count, key, value);
			}
			//			else {
			//				printf("End of history (%d): %s\n", retval, IcsGetErrorText(retval));
			//			}
		}
	}

Error:
	IcsClose(headerfile);
	remove(tmpfilename);

	return 0;
}
