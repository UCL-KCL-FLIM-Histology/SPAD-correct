
#include <windows.h>
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"
#include <vector>
#include <algorithm>

// Timebase scales relies on timebase shifts
extern double* gTimebaseShifts;
extern double* gPeak1Pos;
extern int gnTimebaseShifts;
extern double find_peak(UINT* trans, int nbins);

// Also used by corrections
double* gTimebaseScales = NULL;
double* gPeak2Pos = NULL;
int gnTimebaseScales = 0;


int check_timebase_scales_space(int width, int height)
{
    // get space for height scales + 1 delta between peaks
	int nPixels = width * height;

    if (!gTimebaseScales) {
        gTimebaseScales = (double*)malloc((nPixels + 1) * sizeof(double));  // one value for each pixel sensor
        gnTimebaseScales = nPixels;
    }

    if (!gPeak2Pos) {
        gPeak2Pos = (double*)malloc((nPixels + 1) * sizeof(double));  // one value for each pixel sensor
    }

    if (gnTimebaseScales && gPeak2Pos)
        return(0);
    else
        return(-1);
}

double median(double* data, int nData)
{
    double m;
    std::vector<double> v(data, data + nData);
    std::sort(v.begin(), v.end());
    // Compute Median
    if (v.size() % 2 == 1) //Number of elements are odd
    {
        m = v[v.size() / 2];
    }
    else // Number of elements are even
    {
        int index = (int)v.size() / 2;
        m = (v[index - 1] + v[index]) / 2.0;
    }
    
    return(m);
}

int SPAD_intialise_timebase_scales(USHORT* histogram, int width, int height, int timebins, double delta)
{
    UINT* trans = (UINT*)malloc(timebins * sizeof(UINT));

    if (!histogram) return(-1);

    if (check_timebase_scales_space(width, height) < 0) return(-2);
    if (!gTimebaseShifts) {
        printf("ERROR: Attempt to initialise timebase scales before timebase shifts.\n");
        return(-3);
    }

    double* scale = gTimebaseScales;
    USHORT* pi = histogram; // ptr for summing

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

			// Get one transient
            for (int k = 0; k < timebins; k++) {
                trans[k] = trans[k] + *pi;
                pi++;
            }

			// Find the peak
			double peak_time = find_peak(trans, timebins);

			// store peak position
			gPeak2Pos[i] = peak_time;
		}
	}
	
    // Replace peak position with the delta between peak pos and 1st peak (stored in the shifts wrt the mean position m)
    // Find the average delta, and store delta in d
    double d = 0;
    double* p = gTimebaseScales;
    double* p2 = gPeak2Pos;  // contains the peak 2 positions
    double* p1 = gPeak1Pos;  // contains the peak 1 positions
    double m = gTimebaseShifts[gnTimebaseShifts];   // extra val is mean pos m
    for (int k = 0; k < width * height; k++) {
        *p = fabs((*p2 - *p1));
        d = d + *p;
        p++;
        p1++;
        p2++;
    }
    d = d / (double)(width * height);  // Calculate mean as estimate of delta - can be thrown off by skewed distribution

    d = median(gTimebaseScales, gnTimebaseScales); // Calculate medain as estimate of delta - better than the mean

    // Calculate the required scale to apply each row (detector) and replace the value in gTimebaseScales
    // Use median / delta, if delta is large - store a smaller value to shrink transient when correcting, and vice versa
    for (int k = 0; k < width * height; k++) {
        gTimebaseScales[k] = d / gTimebaseScales[k];
    }

    // Rescale the shifts
    for (int k = 0; k < gnTimebaseShifts; k++) {
        double A = m + gTimebaseShifts[k];
        gTimebaseShifts[k] = gTimebaseScales[k] * A - m;
    }

    // If delta was provided, calculate the overall timebase scale and store as last element
	int last = width * height;
    gTimebaseScales[last] = -1.0;
    if (delta > 0) {
        printf("Using peak delta of %.3f bins and calibrating to %.3f ns\n", d, delta);
        gTimebaseScales[last] = delta / d;
    }
    else {
        printf("Using peak delta of %.3f bins\n", d);
    }

    return(0);
}

double SPAD_get_calibrated_timebase(void)
{
    return (gTimebaseScales[gnTimebaseScales]);  // return the last element, height+1 element
}

void SPAD_set_calibrated_timebase(double ns_per_bin)
{
    gTimebaseScales[gnTimebaseScales] = ns_per_bin;  // set the last element, height+1 element
}

int SPAD_reset_timebase_scales(int width, int height)
{

    if (check_timebase_scales_space(width, height) < 0) return(-1);

    for (int i = 0; i < gnTimebaseScales; i++) {
       gTimebaseScales[i] = 1.0;
    }

    gTimebaseScales[gnTimebaseScales] = 0.0;

    // Test code for fake time scales
//    for (int k = 0; k < gnTimebaseScales; k++) {
//        gTimebaseScales[k] = (double)k / (gnTimebaseScales - 1) + 0.5;
//    }
    // Rescale the shifts
//    double m = gTimebaseShifts[gnTimebaseShifts];   // extra val is mean pos m
//    for (int k = 0; k < gnTimebaseShifts; k++) {
//        double A = m + gTimebaseShifts[k];
//        gTimebaseShifts[k] = gTimebaseScales[k]*A - m;
//    }

    return(0);
}

double* SPAD_get_timebase_scales_ptr()
{
    return gTimebaseScales;
}

int SPAD_write_timebase_scales_to_file(char filepath[])
{
    FILE* fp;
    size_t nWrote;

    fopen_s(&fp, filepath, "wb");
    if (!fp) {
        printf("ERROR: Could not save timebase scales file.\n");
        return(-1);
    }

    nWrote = fwrite(gTimebaseScales, sizeof(double), gnTimebaseScales + 1, fp);

    fclose(fp);

    if (nWrote != gnTimebaseScales + 1)
        printf("ERROR: Timebase scales were not written to file correctly.\n");

    return(0);
}

int SPAD_read_timebase_scales_from_file(char filepath[], int width, int height)
{
    FILE* fp;
    size_t nRead;

    fopen_s(&fp, filepath, "rb");
    if (!fp) {
        printf("ERROR: Could not open timebase scales file.\n");
        return(-1);
    }

    if (check_timebase_scales_space(width, height) < 0) return(-2);

    nRead = fread(gTimebaseScales, sizeof(double), gnTimebaseScales + 1, fp);

    fclose(fp);

    if (nRead != gnTimebaseScales + 1)
        printf("ERROR: Timebase scales were not read from file correctly.\n");

    return(0);
}

int SPAD_dump_timebase_scales_to_text_file(char filepath[])
{
    FILE* fp;

    fopen_s(&fp, filepath, "w");
    if (!fp) {
        printf("ERROR: Could not dump timebase scales to file.\n");
        return(-1);
    }

    for (int i = 0; i < gnTimebaseScales; i++) {
        fprintf(fp, "%f\n", gTimebaseScales[i]);
    }

    fclose(fp);

    return(0);
}