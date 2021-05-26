
#include <windows.h>
#include <iostream>
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"

// Also used by corrections and time base scales calib
double* gTimebaseShifts = NULL;
double* gPeak1Pos = NULL;
int gnTimebaseShifts = 0;

double find_peak(UINT* trans, int nbins)
{
    // Use the centroid method on the top few points
    // https://mathworld.wolfram.com/FunctionCentroid.html

    // find peak bin
    int peak_bin;
    UINT max = 0;
    for (int i = 0; i < nbins; i++) {
        if (trans[i] > max) {
            max = trans[i];
            peak_bin = i;
        }
    }

    // Check for close too close to start or end, return something, TODO Should I care about this?
    if (peak_bin < 2) return(peak_bin);
    if (peak_bin > nbins - 3) return(peak_bin);

    // Find range for centroiding
    int b1 = peak_bin - 2;
    //int b2 = peak_bin + 2;

    int Sfx = 0;  // Sum of x.f(x)
    int Sf = 0;   // Sum of f(x)
    for (int i = 0; i <= 4; i++) {
        Sfx += i * trans[i + b1];
        Sf += trans[i + b1];
    }
    double peak_time = (double)Sfx / (double)Sf + b1;

    return(peak_time);
}

int check_timebase_shifts_space(int width, int height)
{
    // get space for height shifts + 1 mean position for later scale calcs
	int nPixels = width * height;

    if (!gTimebaseShifts) {
        gTimebaseShifts = (double*)malloc((nPixels + 1) * sizeof(double));  // one value for each sensor
        gnTimebaseShifts = nPixels;
    }

    if (!gPeak1Pos) {
        gPeak1Pos = (double*)malloc((nPixels + 1) * sizeof(double));  // one value for each sensor
    }

    if (gTimebaseShifts && gPeak1Pos)
        return(0);
    else
        return(-1);
}

double mean_shift(double* data, int width, int height) {
 	int nPixels = width * height;
    double m = 0;
    double* p = data;
    for (int k = 0; k < nPixels; k++) {
        m = m + *p;
        p++;
    }
    m = m / (double)nPixels;
    return(m);
}

int SPAD_intialise_timebase_shifts(USHORT* histogram, int width, int height, int timebins)
{
    UINT* trans = (UINT*)malloc(timebins * sizeof(UINT));

    if (!histogram) return(-1);

    if (check_timebase_shifts_space(width, height) < 0) return(-2);

    double* shift = gTimebaseShifts;
    USHORT* pi = histogram; // ptr for summing

    for (int i = 0; i < gnTimebaseShifts; i++) {

		// Get one transient
        for (int k = 0; k < timebins; k++) {
            trans[k] = *pi;
            pi++;
        }

		// Find the peak
		double peak_time = find_peak(trans, timebins);

		// store peak position
		gPeak1Pos[i] = peak_time;
	}

    // Find the average peak position
    double m = mean_shift(gPeak1Pos, width, height);

    // Calculate the required shift for each row (detector)
    for (int k = 0; k < gnTimebaseShifts; k++) {
        gTimebaseShifts[k] = gPeak1Pos[k] - m;
    }

    // store mean pos as last element
    gTimebaseShifts[gnTimebaseShifts] = m;

    return(0);
}

int SPAD_reset_timebase_shifts(int width, int height, int timebins)
{

    if (check_timebase_shifts_space(width, height) < 0) return(-1);

    for (int i = 0; i < gnTimebaseShifts; i++) {
        gTimebaseShifts[i] = 0.0;
    }

    gTimebaseShifts[gnTimebaseShifts] = (double)timebins / 6.0;  // some default peak position, 1/6 into transient !!!

    // Test code for fake time shifts
    //    for (int k = 0; k < height; k++) {
    //       gTimebaseShifts[k] = -(double)k/gnTimebaseShifts * 3.0;
    //    }

    return(0);
}

double* SPAD_get_timebase_shifts_ptr()
{
    return gTimebaseShifts;
}

int SPAD_write_timebase_shifts_to_file(char filepath[])
{
    FILE* fp;
    size_t nWrote;

    fopen_s(&fp, filepath, "wb");
    if (!fp) {
        printf("ERROR: Could not save timebase shifts file.\n");
        return(-1);
    }

    nWrote = fwrite(gTimebaseShifts, sizeof(double), gnTimebaseShifts + 1, fp);

    fclose(fp);

    if (nWrote != gnTimebaseShifts + 1)
        printf("ERROR: Timebase shifts were not written to file correctly.\n");

    return(0);
}

int SPAD_read_timebase_shifts_from_file(char filepath[], int width, int height)
{
    FILE* fp;
    size_t nRead;

    fopen_s(&fp, filepath, "rb");
    if (!fp) {
        printf("ERROR: Could not open timebase shifts file.\n");
        return(-1);
    }

    if (check_timebase_shifts_space(width, height) < 0) return(-2);

    nRead = fread(gTimebaseShifts, sizeof(double), gnTimebaseShifts + 1, fp);

    fclose(fp);

    if (nRead != gnTimebaseShifts + 1)
        printf("ERROR: Timebase shifts were not read from file correctly.\n");

    return(0);
}

int SPAD_dump_timebase_shifts_to_text_file(char filepath[])
{
    FILE* fp;

    fopen_s(&fp, filepath, "w");
    if (!fp) {
        printf("ERROR: Could not dump timebase shifts to file.\n");
        return(-1);
    }

    for (int i = 0; i < gnTimebaseShifts; i++) {
        fprintf(fp, "%f\n", gTimebaseShifts[i]);
    }

    fclose(fp);

    return(0);
}
