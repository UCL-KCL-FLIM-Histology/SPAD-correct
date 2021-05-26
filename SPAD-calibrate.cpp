// SPAD-calibrate.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <time.h>
#include <pathcch.h>
#include "cmdparser.hpp"
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"

void configure_parser(cli::Parser& parser) {
    parser.set_required<std::string>("p1", "peak1_image", "Path to input ics file with a peak in the normal position.");
    parser.set_required<std::string>("p2", "peak2_image", "Path to input ics file with a peak in a delayed position.");
	parser.set_required<std::string>("w", "white_image", "Path to input ics file acquired with constant light source.");
	parser.set_required<int>("st", "start_bin", "The first timebin to use from each signal. Usually 10 (UCL) or 40 (KCL).");
	parser.set_required<int>("sp", "stop_bin", "The lasst timebin to use from each signal. Usually 245 (UCL) or 230 (KCL).");
	parser.set_optional<double>("d", "delta", -1.0, "The delay between peaks 1 and 2 in real time will be used to calbrate the time axis. If -1.0 (default) the median time from the data will be used.");
	parser.set_optional<bool>("t", "test-files", false, "Produce text files and detector signals as well as data files to collect statistics on this detector array and test the output.");

    // Examples
    //parser.set_optional<std::string>("o", "output", "data", "Strings are naturally included.");
    //parser.set_optional<int>("n", "number", 8, "Integers in all forms, e.g., unsigned int, long long, ..., are possible. Hexadecimal and Ocatl numbers parsed as well");
    //parser.set_optional<cli::NumericalBase<int, 10>>("t", "temp", 0, "integer parsing restricted only to numerical base 10");
    //parser.set_optional<double>("b", "beta", 11.0, "Also floating point values are possible.");
    //parser.set_optional<bool>("a", "all", false, "Boolean arguments are simply switched when encountered, i.e. false to true if provided.");
    //parser.set_required<std::vector<std::string>>("v", "values", "By using a vector it is possible to receive a multitude of inputs.");
}

void add_images(USHORT* src1, USHORT* src2, USHORT* dst, int w, int h, int t)
{
	USHORT* pdst = dst;
	USHORT* psrc1 = src1;
	USHORT* psrc2 = src2;

	int size = w * h * t;
	for (int i = 0; i < size; i++) {
		*pdst = *psrc1 + *psrc2;
		pdst++;
		psrc1++;
		psrc2++;
	}
}

void sum_all_signals(USHORT* src, UINT* trans, int w, int h, int t)
{
	// Sum all into the first transient
	USHORT* pi = src;
	memset(trans, 0, t * sizeof(UINT)); // reset trans to zero

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			for (int k = 0; k < t; k++) {
				trans[k] += *pi;
				pi++;
			}
		}
	}
}

int main(int argc, char** argv)
{
	cli::Parser parser(argc, argv);
	USHORT *image, *image1, *image2;
	int w, h, t, ret;

	configure_parser(parser);
	parser.run_and_exit_if_error();

	std::string peak1 = parser.get<std::string>("p1");
	std::string peak2 = parser.get<std::string>("p2");
	std::string white = parser.get<std::string>("w");
	int start_bin = parser.get<int>("st");
	int stop_bin = parser.get<int>("sp");
	double delta = parser.get<double>("d");
	bool test_dump = parser.get<bool>("t");

	// TIMEBASE SHIFTS

	printf("SPAD_load3DICSfile %s...", peak1.c_str());
	clock_t tStart = clock();
	ret = SPAD_load3DICSfile((char*)peak1.c_str(), &image1, &w, &h, &t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	if (ret < 0) {
		printf("ERROR: %d Could not load file.\n", ret);
		return(-1);
	}

	printf("SPAD_intialise_timebase_shifts...");
	tStart = clock();
	ret = SPAD_intialise_timebase_shifts(image1, w, h, t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	//free(image1);

	if (ret < 0) {
		printf("ERROR: %d\n", ret);
		return(-2);
	}

	// TIMEBASE SCALES
	// This relies on shifts being done first

	printf("SPAD_load3DICSfile %s...", peak2.c_str());
	tStart = clock();
	ret = SPAD_load3DICSfile((char*)peak2.c_str(), &image2, &w, &h, &t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	if (ret < 0) {
		printf("ERROR: %d Could not load file.\n", ret);
		return(-1);
	}

	printf("SPAD_intialise_timebase_scales...");
	tStart = clock();
	ret = SPAD_intialise_timebase_scales(image2, w, h, t, delta);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	//free(image2);

	if (ret < 0) {
		printf("ERROR: %d\n", ret);
		return(-2);
	}

	// BIN WIDTH FACTORS
	// This relies on shifts and scales being done first
	printf("SPAD_load3DICSfile %s...", white.c_str());
	tStart = clock();
	ret = SPAD_load3DICSfile((char*)white.c_str(), &image, &w, &h, &t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	if (ret < 0) {
		printf("ERROR: %d Could not load file.\n", ret);
		return(-1);
	}

	printf("SPAD_initialise_bin_width_factors...");
	tStart = clock();
	ret = SPAD_initialise_bin_width_factors(image, w, h, t, start_bin, stop_bin);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	//free(image);

	if (ret < 0) {
		printf("ERROR: %d\n", ret);
		return(-2);
	}
	else {
		char datafile[] = "binwidth_factors.dat";
		printf("SPAD_write_bin_width_factors_to_file %s...", datafile);
		tStart = clock();
		ret = SPAD_write_bin_width_factors_to_file(datafile);
		printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

		if (ret < 0) {
			printf("ERROR: %d\n", ret);
			return(-2);
		}

		if (test_dump) {
			char datafile[] = "binwidth_factors_dump.txt";
			printf("SPAD_dump_bin_width_factors_to_text_file %s...", datafile);
			tStart = clock();
			ret = SPAD_dump_bin_width_factors_to_text_file(datafile, w, h, t);
			printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

			if (ret < 0) {
				printf("ERROR: %d\n", ret);
				return(-2);
			}
		}
	}

	// Correct for the bwf first to get more accurate peak positions
	SPAD_reset_timebase_shifts(w, h, t);
	SPAD_reset_timebase_scales(w, h);
	printf("Correcting p1 image...\n");
	SPAD_CorrectTransients(image1, w, h, t);
	printf("Correcting p2 image...\n");
	SPAD_CorrectTransients(image2, w, h, t);

	// Now do shifts again on bwf corrected data for accurate peak positions
	printf("SPAD_intialise_timebase_shifts...");
	tStart = clock();
	ret = SPAD_intialise_timebase_shifts(image1, w, h, t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	//free(image1);

	if (ret < 0) {
		printf("ERROR: %d\n", ret);
		return(-2);
	}
	else {
		char datafile[] = "timebase_shifts.dat";
		printf("SPAD_write_timebase_shifts_to_file %s...", datafile);
		tStart = clock();
		ret = SPAD_write_timebase_shifts_to_file(datafile);
		printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

		if (ret < 0) {
			printf("ERROR: %d\n", ret);
			return(-2);
		}

		if (test_dump) {
			char datafile[] = "timebase_shifts_dump.txt";
			printf("SPAD_dump_timebase_shifts_to_text_file %s...", datafile);
			tStart = clock();
			ret = SPAD_dump_timebase_shifts_to_text_file(datafile);
			printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

			if (ret < 0) {
				printf("ERROR: %d\n", ret);
				return(-2);
			}
		}
	}

	// Now do scales again on bwf corrected data for accurate peak positions
	printf("SPAD_intialise_timebase_scales...");
	tStart = clock();
	ret = SPAD_intialise_timebase_scales(image2, w, h, t, delta);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	//free(image2);

	if (ret < 0) {
		printf("ERROR: %d\n", ret);
		return(-2);
	}
	else {
		char datafile[] = "timebase_scales.dat";
		printf("SPAD_write_timebase_scales_to_file %s...", datafile);
		tStart = clock();
		ret = SPAD_write_timebase_scales_to_file(datafile);
		printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

		if (ret < 0) {
			printf("ERROR: %d\n", ret);
			return(-2);
		}

		if (test_dump) {
			char datafile[] = "timebase_scales_dump.txt";
			printf("SPAD_dump_timebase_scales_to_text_file %s...", datafile);
			tStart = clock();
			ret = SPAD_dump_timebase_scales_to_text_file(datafile);
			printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

			if (ret < 0) {
				printf("ERROR: %d\n", ret);
				return(-2);
			}
		}
	}

	// get metadata
	ICS* ip;
	double xy_microns_per_pixel, ns_per_bin;
	IcsOpen(&ip, (char*)white.c_str(), "r");  // should work since we just opened above
	IcsGetPosition(ip, 0, NULL, &ns_per_bin, NULL);
	IcsGetPosition(ip, 1, NULL, &xy_microns_per_pixel, NULL);
	IcsClose(ip);

	// load p1 and p2 fresh to generate test files
	free(image1);
	free(image2);

	printf("SPAD_load3DICSfile %s...", peak1.c_str());
	tStart = clock();
	ret = SPAD_load3DICSfile((char*)peak1.c_str(), &image1, &w, &h, &t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	if (ret < 0) {
		printf("ERROR: %d Could not load file.\n", ret);
		return(-1);
	}

	printf("SPAD_load3DICSfile %s...", peak2.c_str());
	tStart = clock();
	ret = SPAD_load3DICSfile((char*)peak2.c_str(), &image2, &w, &h, &t);
	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
	if (ret < 0) {
		printf("ERROR: %d Could not load file.\n", ret);
		return(-1);
	}


	// Apply the correction to peak images and check time calibration
	// (Check that timebase to be applied globally works globally. Found that canbe a few % out from local estimate.)

	printf("Generate detector signals before and after correction...\n");
	tStart = clock();

	printf("Correcting p1 image...\n");
	SPAD_CorrectTransients(image1, w, h, t);
	printf("Correcting p2 image...\n");
	SPAD_CorrectTransients(image2, w, h, t);

	if (delta > 0) {
		// Final time calibration
		extern double find_peak(UINT * trans, int nbins);
		UINT* trans1 = (UINT*)malloc(t * sizeof(UINT));
		UINT* trans2 = (UINT*)malloc(t * sizeof(UINT));
		sum_all_signals(image1, trans1, w, h, t);
		sum_all_signals(image2, trans2, w, h, t);
		double peak1 = find_peak(trans1, t);
		double peak2 = find_peak(trans2, t);
		double calibration = delta / abs(peak2 - peak1);

		printf("Time calibration tweaked from %.3f to %.3f ns/bin\n", SPAD_get_calibrated_timebase(), calibration);

		SPAD_set_calibrated_timebase(calibration);

		char datafile[] = "timebase_scales.dat";
		printf("SPAD_write_timebase_scales_to_file %s...", datafile);
		ret = SPAD_write_timebase_scales_to_file(datafile);

	}

	if (test_dump) {
		// save the 'after' case after the final time calibration
		add_images(image1, image2, image, w, h, t);

		char imagefile2[] = "detector_peaks_after.ics";
		SPAD_save3DICSfile(imagefile2, image, w, h, t, 5, NULL, 0, xy_microns_per_pixel, SPAD_get_calibrated_timebase());
	}

	printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

	free(image);
	free(image1);
	free(image2);

	return (0);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
