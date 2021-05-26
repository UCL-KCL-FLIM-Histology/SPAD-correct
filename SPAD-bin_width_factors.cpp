
#include <windows.h>
#include <iostream>
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"

extern double* gPeak1Pos;
extern int gnTimebaseShifts;

extern double* gPeak2Pos;
extern int gnTimebaseScales;

// Also used by corrections
double* gBinWidthFactors = NULL;
int gnBinWidthFactors = 0;

int check_bin_width_factors_space(int width, int height, int timebins)
{
    if (!gBinWidthFactors) {
        gBinWidthFactors = (double*)malloc(height * width * timebins * sizeof(double));  // one set for each pixel sensor
        gnBinWidthFactors = height * width * timebins;
    }

    if (gBinWidthFactors)
        return(0);
    else
        return(-1);
}

/*
Extract and store bin width factors from a calibration image
from a constant light source.
*/
int SPAD_initialise_bin_width_factors(USHORT* histogram, int width, int height, int timebins, int start_bin, int stop_bin)
{
    UINT* trans = (UINT*)malloc(timebins * sizeof(UINT));
    //int start_bin = 10, stop_bin = 246;  // ignore first last few bins as signal drop away here

    if (!histogram) return(-1);

    if (start_bin < 0) return(-2);
    if (stop_bin < 0) return(-2);
    if (start_bin > timebins-1) return(-2);
    if (stop_bin > timebins-1) return(-2);

    if (check_bin_width_factors_space(width, height, timebins) < 0) return(-3);

    USHORT* pi = histogram; // ptr for summing
    double* factor = gBinWidthFactors;
    int nPixels = width * height;

    for (int i = 0; i < nPixels; i++) {

		// Get one transient
        for (int k = 0; k < timebins; k++) {
            trans[k] = *pi;
            pi++;
        }

		// Calculate mean of transient, to maintain time calibration this must be done over the bins between the 2 peaks of shift and scale data
		double m = 0;
		int p1 = (int)gPeak1Pos[i];
		int p2 = (int)gPeak2Pos[i] + 1;
		if (p1 > p2) {m = p1; p1 = p2; p2 = m; m = 0;}
		UINT* p = &(trans[(start_bin)]);
//        for (int k = (start_bin); k < (stop_bin); k++) {
		for (int k = p1; k < p2; k++) {
				m = m + *p;
			p++;
		}
//        m = m / (double)(stop_bin - start_bin);
		m = m / (double)(p2 - p1);

		// Get factor for each bin of each detector, fill ends with 1.0
		p = trans;
		for (int k = 0; k < start_bin; k++) {
			*factor = 1.0;
			p++;
			factor++;
		}
		for (int k = start_bin; k < stop_bin; k++) {
			*factor = (double)(*p) / m;
			p++;
			factor++;
		}
		for (int k = stop_bin; k < timebins; k++) {
			*factor = 1.0;
			p++;
			factor++;
		}
    }

    return(0);
}

int SPAD_reset_bin_width_factors(int width, int height, int timebins)
{
    if (check_bin_width_factors_space(width, height, timebins) < 0) return(-1);

    for (int i = 0; i < gnBinWidthFactors; i++) {
        gBinWidthFactors[i] = 1.0;
    }

    return(0);
}

double* SPAD_get_bin_width_factors_ptr()
{
    return gBinWidthFactors;
}

int SPAD_write_bin_width_factors_to_file(char filepath[])
{
    FILE* fp;
    size_t nWrote;

    fopen_s(&fp, filepath, "wb");
    if (!fp) {
        printf("ERROR: Could not save bin width factors file.\n");
        return(-1);
    }

    nWrote = fwrite(gBinWidthFactors, sizeof(double), gnBinWidthFactors, fp);

    fclose(fp);

    if (nWrote != gnBinWidthFactors)
        printf("ERROR: Bin width factors were not written to file correctly.\n");

    return(0);
}

int SPAD_read_bin_width_factors_from_file(char filepath[], int width, int height, int timebins)
{
    FILE* fp;
    size_t nRead;

    fopen_s(&fp, filepath, "rb");
    if (!fp) {
        printf("ERROR: Could not open bin width factors file.\n");
        return(-1);
    }

    if (check_bin_width_factors_space(width, height, timebins) < 0) return (-2);

    nRead = fread(gBinWidthFactors, sizeof(double), gnBinWidthFactors, fp);

    fclose(fp);

    if (nRead != gnBinWidthFactors)
        printf("ERROR: Bin width factors were not read from file correctly.\n");

    return(0);
}

int SPAD_dump_bin_width_factors_to_text_file(char filepath[], int width, int height, int timebins)
{
    FILE* fp;

    fopen_s(&fp, filepath, "w");
    if (!fp) {
        printf("ERROR: Could not dump bin width factors to file.\n");
        return(-1);
    }

    double* f = gBinWidthFactors;
    int nPixels = width * height;

    for (int i = 0; i < nPixels; i++) {
        fprintf(fp, "%f", *f);    // first entry without comma
        f++;
        for (int j = 1; j < timebins; j++) {
            fprintf(fp, ", %f", *f);
            f++;
        }
        fprintf(fp, "\n");
    }

    fclose(fp);

    return(0);
}