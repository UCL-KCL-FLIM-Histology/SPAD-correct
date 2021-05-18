
#include <windows.h>
#include <iostream>
#include "SPAD-correct_internal.h"
#include "SPAD-correct.h"

/* Load a sim file */
int SPAD_loadfile(char filepath[], BYTE** data, unsigned long long* nBytes)
{
	FILE* fp = NULL;
	long long filelen;

	if (fopen_s(&fp, filepath, "rb") != 0) {
		printf("Could not open file\n");
		return -1;
	}

	_fseeki64(fp, 0, SEEK_END);
	filelen = _ftelli64(fp);
	if (filelen < 0) {
		const int errmsglen = 256;
		char errmsg[errmsglen];
		strerror_s(errmsg, errmsglen, errno);
		printf("ERROR: %s\n", errmsg);
	}
	else {
		printf("File is %lld bytes.\n", filelen);
	}
	rewind(fp);

	BYTE* file_bytes = (BYTE*)malloc(filelen);
	if (!file_bytes) {
		printf("Malloc error!\n");
		return(-2);
	}

	fread_s(file_bytes, filelen, 1, filelen, fp);
	fclose(fp);

	*data = file_bytes;
	*nBytes = filelen;

	return 0;
}

/* load an ics image file */
int SPAD_load3DICSfile(char filepath[], USHORT** image, int* width, int* height, int* timebins)
{
	ICS* ip;
	Ics_DataType dt;
	int ndims;
	size_t dims[ICS_MAXDIM];
	size_t bufsize;
	void* buf;
	Ics_Error retval;

	retval = IcsOpen(&ip, filepath, "r");
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: Cannot open ics file for reading.\n");
		return(-1);
	}

	IcsGetLayout(ip, &dt, &ndims, dims);
	if (dt != Ics_uint16) {
		printf("SPAD_load3DICSfile ERROR: File not UINT16 data type.\n");
		return(-2);
	}

	bufsize = IcsGetDataSize(ip);
	buf = malloc(bufsize);
	if (buf == NULL) {
		printf("SPAD_load3DICSfile ERROR: Cannot malloc buffer.\n");
		return(-3);
	}

	retval = IcsGetData(ip, buf, bufsize);
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: IcsGetData failed.\n");
		return(-4);
	}

	retval = IcsClose(ip);
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: Cannot close ics file.\n");
		return(-5);
	}

	if (ndims != 3) {
		printf("SPAD_load3DICSfile ERROR: Not a 3D image file, %d dims detected\n", ndims);
		return(-6);
	}

	*height = (int)dims[2];
	*width = (int)dims[1];
	*timebins = (int)dims[0];
	*image = (USHORT*)buf;

	return(0);
}

/* load an ics image file into existing buffer - for Labview use*/
int SPAD_load3DICSfile_LV(char filepath[], USHORT* image, int width, int height, int timebins)
{
	ICS* ip;
	Ics_DataType dt;
	int ndims;
	size_t dims[ICS_MAXDIM];
	size_t bufsize;
	Ics_Error retval;

	retval = IcsOpen(&ip, filepath, "r");
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: Cannot open ics file for reading.\n");
		return(-1);
	}

	IcsGetLayout(ip, &dt, &ndims, dims);
	if (dt != Ics_uint16) {
		printf("SPAD_load3DICSfile ERROR: File not UINT16 data type.\n");
		return(-2);
	}

	// Check the image is of the expected size
	if (dims[0] != timebins) {
		printf("SPAD_load3DICSfile ERROR: Expected %d timebins, found %d.\n", timebins, dims[0]);
		return(-11);
	}

	if (dims[1] != width) {
		printf("SPAD_load3DICSfile ERROR: Expected %d width, found %d.\n", width, dims[1]);
		return(-11);
	}

	if (dims[2] != height) {
		printf("SPAD_load3DICSfile ERROR: Expected %d height, found %d.\n", height, dims[2]);
		return(-11);
	}


	bufsize = IcsGetDataSize(ip);
	if (image == NULL) {
		printf("SPAD_load3DICSfile ERROR: Invalid image array supplied.\n");
		return(-3);
	}

	retval = IcsGetData(ip, (void*)image, bufsize);
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: IcsGetData failed.\n");
		return(-4);
	}

	retval = IcsClose(ip);
	if (retval != IcsErr_Ok) {
		printf("SPAD_load3DICSfile ERROR: Cannot close ics file.\n");
		return(-5);
	}

	if (ndims != 3) {
		printf("SPAD_load3DICSfile ERROR: Not a 3D image file, %d dims detected\n", ndims);
		return(-6);
	}

	return(0);
}
int SPAD_save3DICSfile(char filepath[], USHORT* histogram, int width, int height, int timebins, int compression_level,
	BYTE* header, unsigned long long max_header_bytes, double xy_microns_per_pixel, double ns_per_bin)
{
	ICS* imagefile;
	Ics_Error retval;
	size_t dims[3] = { (size_t)timebins, (size_t)width, (size_t)height };
	size_t histsize = width * height * timebins * sizeof(USHORT);

	retval = IcsOpen(&imagefile, filepath, "w2");
	if (retval != IcsErr_Ok) {
		printf("SPAD_save3DICSfile ERROR: Cannot open ics file for writing.\n");
		return -1;
	}

	IcsSetLayout(imagefile, Ics_uint16, 3, dims);
	IcsSetData(imagefile, histogram, histsize);
	if (compression_level == 0) {
		IcsSetCompression(imagefile, IcsCompr_uncompressed, 0);
	}
	else {
		IcsSetCompression(imagefile, IcsCompr_gzip, compression_level);
	}

	// Dimensions
	char buffer1[ICS_LINE_LENGTH];
	sprintf_s(buffer1, "%d %d %d", timebins, width, height);

	if (header != NULL) { // add minimal header info
		SPAD_readHeader(header, max_header_bytes, imagefile);

		// Add correct values after sorting and transform

		// Extents
		/*		Read from the header and scale values according to crop?
				float x = 1.0f, y = 1.0f;
				Ics_HistoryIterator it;
				Ics_Error retval = IcsNewHistoryIterator(imagefile, &it, "extents");
				if (retval == IcsErr_Ok) {  // we found the extents line
					char value[ICS_LINE_LENGTH];
					retval = IcsGetHistoryKeyValueI(imagefile, &it, NULL, value);
					if (retval == IcsErr_Ok) {
						sscanf_s(value, "%f\t%f", &x, &y);
					}
				}
				sprintf_s(buffer2, "%e %e %e", ns_per_bin * timebins * 1E-9, xy_microns_per_pixel * width * 1E-6, xy_microns_per_pixel * height * 1E-6);
		*/
		IcsDeleteHistory(imagefile, "type");
		IcsDeleteHistory(imagefile, "labels");
		IcsDeleteHistory(imagefile, "dimensions");
		IcsDeleteHistory(imagefile, "extents");
		IcsDeleteHistory(imagefile, "units");
	}

	// Extents
	char buffer2[ICS_LINE_LENGTH];
	sprintf_s(buffer2, "%e %e %e", ns_per_bin * timebins * 1E-9, xy_microns_per_pixel * width * 1E-6, xy_microns_per_pixel * height * 1E-6);

	// Add essential metadata
	IcsSetPosition(imagefile, 0, 0.0, ns_per_bin, "ns");
	IcsSetPosition(imagefile, 1, 0.0, xy_microns_per_pixel, "microns");
	IcsSetPosition(imagefile, 2, 0.0, xy_microns_per_pixel, "microns");

	IcsAddHistory(imagefile, "author", "SPAD-sorter");
	IcsAddHistory(imagefile, "type", "Time Resolved");
	IcsAddHistory(imagefile, "labels", "t x y");
	IcsAddHistory(imagefile, "dimensions", buffer1);
	IcsAddHistory(imagefile, "extents", buffer2);
	IcsAddHistory(imagefile, "units", "s m m");

	retval = IcsClose(imagefile);
	if (retval != IcsErr_Ok) {
		printf("SPAD_save3DICSfile ERROR: Cannot close ics file.\n");
		return -2;
	}

	return 0;
}

int SPAD_save2DICSfile(char filepath[], void* buffer, int width, int height, int bit_depth, int compression_level,
	BYTE* header, unsigned long long max_header_bytes, double xy_microns_per_pixel)
{
	ICS* imagefile;
	Ics_Error retval;
	size_t dims[2] = { (size_t)width, (size_t)height };
	size_t buffsize;
	Ics_DataType data_type;

	retval = IcsOpen(&imagefile, filepath, "w2");
	if (retval != IcsErr_Ok) {
		printf("SPAD_save2DICSfile ERROR: Cannot open ics file for writing.\n");
		return -1;
	}

	switch (bit_depth)
	{
	case 8:
		data_type = Ics_uint8;
		buffsize = width * height * sizeof(UCHAR);
		break;
	case 16:
		data_type = Ics_uint16;
		buffsize = width * height * sizeof(USHORT);
		break;
	case 32:
		data_type = Ics_uint32;
		buffsize = width * height * sizeof(UINT);
		break;
	default:
		break;
	}

	IcsSetLayout(imagefile, data_type, 2, dims);
	IcsSetData(imagefile, buffer, buffsize);
	if (compression_level == 0) {
		IcsSetCompression(imagefile, IcsCompr_uncompressed, 0);
	}
	else {
		IcsSetCompression(imagefile, IcsCompr_gzip, compression_level);
	}

	if (header == NULL) { // add minimal header info
		IcsAddHistory(imagefile, "author", "SPAD-sorter");
		IcsAddHistory(imagefile, "type", "Intensity");
		IcsAddHistory(imagefile, "labels", "x y");
	}
	else {
		SPAD_readHeader(header, max_header_bytes, imagefile);

		// Dimensions
		char buffer1[ICS_LINE_LENGTH];
		sprintf_s(buffer1, "%d %d", width, height);

		// Extents
		char buffer2[ICS_LINE_LENGTH];
		sprintf_s(buffer2, "%e %e", xy_microns_per_pixel * width * 1E-6, xy_microns_per_pixel * height * 1E-6);
		IcsSetPosition(imagefile, 0, 0.0, xy_microns_per_pixel, "microns");
		IcsSetPosition(imagefile, 1, 0.0, xy_microns_per_pixel, "microns");

		IcsDeleteHistory(imagefile, "type");
		IcsDeleteHistory(imagefile, "labels");
		IcsDeleteHistory(imagefile, "dimensions");
		IcsDeleteHistory(imagefile, "extents");
		IcsDeleteHistory(imagefile, "units");

		IcsAddHistory(imagefile, "type", "Intensity");
		IcsAddHistory(imagefile, "labels", "x y");
		IcsAddHistory(imagefile, "dimensions", buffer1);
		IcsAddHistory(imagefile, "extents", buffer2);
		IcsAddHistory(imagefile, "units", "m m");

	}

	retval = IcsClose(imagefile);
	if (retval != IcsErr_Ok) {
		printf("SPAD_save2DICSfile ERROR: Cannot close ics file.\n");
		return -2;
	}

	return 0;
}