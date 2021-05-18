/*
	Header for the SPAD Correct dll.
	
	P Barber, Created May 2021

*/

#pragma once

extern "C" {

	/** 
	SPAD_loadfile

	Load a .sim file as binary data into memory.

	\param filepath The path of the file to load.
	\param file_bytes A returned pointer to where the data has been stored. Free it with free when finished.
	\param nBytes The returned size of the data in bytes.
	*/
	__declspec(dllexport) int SPAD_loadfile(char filepath[], BYTE **file_bytes, unsigned long long *nBytes);

	/**
	SPAD_load3DICSfile

	Load a 3D ICS image file into a 3D image buffer. Expects this to be a SPAD image with strides the correct way around
	(i.e. time bins are contiguous, next larger stride is width, then height), and unsigned short data (uint16).

	\param filepath The path of the file to load.
	\param image A returned pointer to where the image data has been stored. Free it with free when finished.
	\param width Returns the width of the image.
	\param height Returns the height of the image.
	\param timebins Returns the number of time bins in each transient of the image.
	*/
	__declspec(dllexport) int SPAD_load3DICSfile(char filepath[], USHORT** image, int* width, int* height, int* timebins);

	/**
	SPAD_load3DICSfile_LV

	Version of SPAD_load3DICSfile that requires a preallocated buffer, primarily for use from LabView.

	Load a 3D ICS image file into a 3D image buffer. Expects this to be a SPAD image with strides the correct way around
	(i.e. time bins are contiguous, next larger stride is width, then height), and unsigned short data (uint16).

	\param filepath The path of the file to load.
	\param image A pointer to the image buffer. Must be pre-allocated.
	\param width The expected width of the image.
	\param height The expected height of the image.
	\param timebins The expected number of time bins in each transient of the image.
	*/
	__declspec(dllexport) int SPAD_load3DICSfile_LV(char filepath[], USHORT* image, int width, int height, int timebins);


	/**
	SPAD_save3DICSfile

	Takes 3D histogram data and saves to an ICS file.

	\param filepath The path to save the file to.
	\param histogram 3D histogram to save. timebins is the finest stride and height is the longest.
	\param width The image width.
	\param height The image height.
	\param timebine The number of time resolved time bins.
	\param compression_level The level of gzip compression to use, 0=none, 1=fast, 9=best, -1=default(???).
	\param header (optional, can be NULL) A text stream or array of header info in ICS format
	\param max_header_bytes The max size of the header array. The 'end' marker will be searched for, this size should include that.
	\param xy_microns_per_pixel Defines the real scale of the image in the x-y dimensions
	\param ns_per_bin Defines the timebase of the time resolved data dimension
	*/
	__declspec(dllexport) int SPAD_save3DICSfile(char filepath[], USHORT *histogram, int width, int height, int timebins, int compression_level, BYTE *header, 
		unsigned long long max_header_bytes, double xy_microns_per_pixel, double ns_per_bin);

	/**
	SPAD_save2DICSfile

	Takes 2D image data and saves to an ICS file.

	\param filepath The path to save the file to.
	\param buffer 2D image to save. width is the finest stride and height is the longest.
	\param width The image width.
	\param height The image height.
	\param bit_depth The bit detpth of the buffer. Can be 8, 16 or 32 corresponding to types unsigned char, unsigned short or unsigned int.
	\param compression_level The level of gzip compression to use, 0=none, 1=fast, 9=best, -1=default(???).
	\param header (optional, can be NULL) A text stream or array of header info in ICS format
	\param max_header_bytes The max size of the header array. The 'end' marker will be searched for, this size should include that.
	\param xy_microns_per_pixel Defines the real scale of the image in the x-y dimensions
	*/
	__declspec(dllexport) int SPAD_save2DICSfile(char filepath[], void *buffer, int width, int height, int bit_depth, int compression_level,
		BYTE *header, unsigned long long max_header_bytes, double xy_microns_per_pixel);

	/**
	SPAD_find_integrations

	Expect uncompressed data from a sim file.
	Run through the data and find the start of each integration marked by "Integration Number: "
	Also, estimate the number of frames that are present for each pixel (where clock values are constant).
	It fills up the dll global variables integration_list and integration_count with a max of 100 integrations.

	\param data The data stream to search
	\param nBytes The number of bytes in the data stream, it will stop searching when the end is reached
	\param clock_max The max value of the clocks
	\param estimated_frames_per_clock Returns the estimate the number of frames that are present for each pixel
	\param list Returns a pointer to integration_list of BYTE*. Can be NULL if not needed.
	\return integration_count The number of integrations found.
	*/
	__declspec(dllexport) int SPAD_find_integrations(BYTE *data, unsigned long long nBytes, unsigned int clock_max, unsigned int *estimated_frames_per_clock, BYTE ***list);

	/**
	SPAD_bin_by_2

	Bin groups of 4 pixels into 1. Reduces image width and height by 2.

	\param histogram Image input data, should be size: width * height * timebins * size(USHORT) bytes. Size of this buffer is not changed, operation is performed in place.
	\param width Pixel width of input image
	\param height Pixel height of input image
	\param timebins Number of timebins in each transient of input image
	\param new_width Returns the width of the new image
	\param new_height Returns the height of the new image
	*/
	__declspec(dllexport) void SPAD_bin_by_2(USHORT *histogram, int width, int height, int timebins, int *new_width, int *new_height);
	
	/**
	SPAD_bin

	Bin according to a bin size by repeated use of SPAD_bin_by_2.

	\param histogram Image input data, should be size: width * height * timebins * size(USHORT) bytes. Size of this buffer is not changed, operation is performed in place.
	\param width Pixel width of input image
	\param height Pixel height of input image
	\param timebins Number of timebins in each transient of input image
	\param bin_size Amount to bin, even numbers only. 2 = bin 2x2, 3 = 2x2, 4 = 4x4, 5 = 4x4 etc.
	\param new_width Returns the width of the new image
	\param new_height Returns the height of the new image
	*/
	__declspec(dllexport) void SPAD_bin(USHORT *histogram, int width, int height, int timebins, int bin_size, int *new_width, int *new_height);

	/**
	SPAD_makeIntensityImage

	Make an intensity image from a time resolved image.

	\param histogram The buffer holding the time resolved image.
	\param width The width of the time resolved image and the resulting intensity image.
	\param height The height of the time resolved image and the resulting intensity image.
	\param timebins The number of timebins in the time resolved image.
	\param buffer A pointer to the created intensity image. Free the memory with free() when done. May be NULL if only count is required.
	\return total photon count in histogram or error code if less than zero.
	*/
	__declspec(dllexport) int SPAD_makeIntensityImage(USHORT *histogram, int width, int height, int timebins, UINT *buffer);

	/**
	SPAD_makeCleanIntensityImage

	SPAD_makeIntensityImage with row based median filter.
	*/
	__declspec(dllexport) int SPAD_makeCleanIntensityImage(USHORT* histogram, int width, int height, int timebins, UINT* buffer);


	/**
	SPAD_initialise_bin_width_factors

	Calculates the bin widith factors for correcting DNL from a time resolved image. Should supply an image taken with a constant 
	light source, i.e. not time correlated with the laser sync. It stores the factors in global buffers ready for use on future data.

	\param histogram The buffer holding the time resolved image.
	\param width The width of the time resolved image.
	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	\param start_bin The first time bin to use, set > 0 to ignore grot at start of signal. For UCL this can be 10, for KCL 40.
	\param stop_bin The last time bin to use, set > 0 to ignore grot at end of signal. For UCL this can be 245, for KCL 230.
	\return error code
	*/
	__declspec(dllexport) int SPAD_initialise_bin_width_factors(USHORT* histogram, int width, int height, int timebins, int start_bin, int stop_bin);

	/**
	SPAD_reset_bin_width_factors

	Reset all factors to 1.0

	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	*/
	__declspec(dllexport) int SPAD_reset_bin_width_factors(int width, int height, int timebins);

	/**
	SPAD_get_bin_width_factors_ptr

	\return pointer to the stored array
	*/
	__declspec(dllexport) double* SPAD_get_bin_width_factors_ptr();

	/**
	SPAD_write_bin_width_factors_to_file

	Write the stored factors to a binary file for later use.

	\param filepath Path to save to.
	\return error code
	*/
	__declspec(dllexport) int SPAD_write_bin_width_factors_to_file(char filepath[]);

	/**
	SPAD_read_bin_width_factors_from_file

	Read and store factors from a file for later use.

	\param filepath Path of file.
	\param height Image height.
	\param timebins Number of timebins.
	\return error code
	*/
	__declspec(dllexport) int SPAD_read_bin_width_factors_from_file(char filepath[], int width, int height, int timebins);

	/**
	SPAD_dump_timebase_shifts_to_text_file

	Write the bin_width_factors to a text file to examine them or get stats on the sensor.

	\param filepath The path to write text file to.
	\param height Image height.
	\param timebins Number of timebins.
	\return error code
	*/
	__declspec(dllexport) int SPAD_dump_bin_width_factors_to_text_file(char filepath[], int width, int height, int timebins);

	/**
	SPAD_intialise_timebase_shifts

	Calculates the time base shift factors for correcting INL from a time resolved image. Should supply a image taken with a sample with a short lifetime. 
	It stores the factors in global buffers ready for use on future data.

	\param histogram The buffer holding the time resolved image.
	\param width The width of the time resolved image.
	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	\return error code
	*/
	__declspec(dllexport) int SPAD_intialise_timebase_shifts(USHORT* histogram, int width, int height, int timebins);

	/**
	SPAD_reset_timebase_shifts

	Reset all factors to 0.0
	
	TODO: The entry tagged on the end of the array is the average peak position found in the calibration data. If we reset, we do not have data, so what
	to set this value to. Currently it is hard coded to 3.65 ns, since this was similar to that found in initial UCL data.

	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	*/
	__declspec(dllexport) int SPAD_reset_timebase_shifts(int width, int height, int timebins);

	/**
	SPAD_get_timebase_shifts_ptr

	\return pointer to the stored array
	*/
	__declspec(dllexport) double* SPAD_get_timebase_shifts_ptr();

	/**
	SPAD_write_bin_width_factors_to_file

	Write the stored factors to a file for later use.

	\param filepath Path to save to.
	\return error code
	*/
	__declspec(dllexport) int SPAD_write_timebase_shifts_to_file(char filepath[]);

	/**
	SPAD_read_timebase_shifts_from_file

	Read and store factors from a binary file for later use.

	\param filepath Path of file.
	\return error code
	*/
	__declspec(dllexport) int SPAD_read_timebase_shifts_from_file(char filepath[], int width, int height);

	/**
	SPAD_dump_timebase_shifts_to_text_file

	Write the shifts to a text file to examine them or get stats on the sensor.

	\param filepath The path to write text file to.
	\return error code
	*/
	__declspec(dllexport) int SPAD_dump_timebase_shifts_to_text_file(char filepath[]);

	/**
	SPAD_intialise_timebase_scales

	Calculates the time base scale factors for correcting INL from a time resolved image. Should supply a image taken with a sample with a short lifetime
	that has been delayed in some way compared to the image used for SPAD_intialise_timebase_shifts. Usually we remove 1m of cable from the laser sync line.
	SPAD_intialise_timebase_shifts must be called before this function.
	It stores the factors in global buffers ready for use on future data.

	\param histogram The buffer holding the time resolved image.
	\param width The width of the time resolved image.
	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	\param delta The known delay in real time. Will be used to reset the timebase scale in the metadata of corrected images.
	\return error code
	*/
	__declspec(dllexport) int SPAD_intialise_timebase_scales(USHORT* histogram, int width, int height, int timebins, double delta);

	/**
	SPAD_get_calibrated_timebase

	Use this when correcting data to get the timebase (ns per bin) that was calculated from the delay between peaks of the calibration data.
	\return The ns per bin value, or zero if it was not calculated.
	*/
	__declspec(dllexport) double SPAD_get_calibrated_timebase(void);
	__declspec(dllexport) void SPAD_set_calibrated_timebase(double ns_per_bin);

	/**
	SPAD_reset_timebase_scales

	Reset all factors to 1.0

	\param height The height of the time resolved image.
	*/
	__declspec(dllexport) int SPAD_reset_timebase_scales(int width, int height);

	/**
	SPAD_get_timebase_scales_ptr

	\return pointer to the stored array
	*/
	__declspec(dllexport) double* SPAD_get_timebase_scales_ptr();

	/**
	SPAD_write_bin_width_factors_to_file

	Write the stored factors to a binary file for later use.

	\param filepath Path to save to.
	\return error code
	*/
	__declspec(dllexport) int SPAD_write_timebase_scales_to_file(char filepath[]);

	/**
	SPAD_read_timebase_scales_from_file

	Read and store factors from a file for later use.

	\param filepath Path of file.
	\return error code
	*/
	__declspec(dllexport) int SPAD_read_timebase_scales_from_file(char filepath[], int width, int height);

	/**
	SPAD_dump_timebase_scales_to_text_file

	Write the scales to a text file to examine them or get stats on the sensor.

	\param filepath The path to write text file to.
	\return error code
	*/
	__declspec(dllexport) int SPAD_dump_timebase_scales_to_text_file(char filepath[]);

	/**
	SPAD_CorrectTransients

	Correct the supplied image for the calibrated factors (bin widths, timebase shifts and scales).
	If factors have not been previously stored, factors will be reset (using the functions above) to have no effect.
	Store factors for use by using the relevant read factors from file functions.
	Alternatively, use the relevant initialise function and supply a suitable image.
	Recommeded use is to use initialise functions, then the save functions, and then in future the read functions can
	be used before this function is used.
	Correction in done in place (ie data in the image is corrected and replaced) for higher speed and lower memory use, 
	so no output image need be supplied.

	\param image Time resolve image to be corrected. Correction is done in place.
	\param width The width of the time resolved image.
	\param height The height of the time resolved image.
	\param timebins The number of timebins in the time resolved image.
	\param ns_per_bin Calibration of time per bin to calibrate the shifts.
	\return Error code.
	*/
	__declspec(dllexport) int SPAD_CorrectTransients(USHORT* image, int width, int height, int timebins);





	/* test functions */

	/**
	SPAD_find_test
	
	Looks for integrations and frames with the dataset.
	Writes a file of frame counts and clock transitions to the TEMP folder (probably %LocalAppData%\temp)

	\param data The data stream to search
	\param nBytes The number of bytes in the data stream, it will stop searching when the end is reached
	*/
	__declspec(dllexport) int SPAD_find_test(BYTE *data, unsigned long long nBytes);


    /**
	SPAD_simpletest

	Just checks that we can open a file, verifies that the dll is being called.

	\param filepath Path to the file to try and open.
	*/
	__declspec(dllexport) int SPAD_simpletest(char filepath[]);


}
