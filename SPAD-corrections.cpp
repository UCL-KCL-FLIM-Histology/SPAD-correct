
#include <windows.h>
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"
#include <random>
#include <cmath> 

extern double* gBinWidthFactors;
extern int gnBinWidthFactors;
extern double* gTimebaseShifts;
extern int gnTimebaseShifts;
extern double* gTimebaseScales;
extern int gnTimebaseScales;

// MSVC Binomial random number generation, 39 ms for 16x16

static std::random_device rd;
static std::mt19937_64 gen(rd());
using BinomialDist = std::binomial_distribution<>;

int MSVC_rbinom(int n, double p)
{
    if (p <= 0.0) return(0);
    if (p >= 1.0) return(n);
    BinomialDist rand_binom(n, p);
    return rand_binom(gen);
}


// Brute force photon by photon, 17 ms for 16x16
// Speed depends on n with large n slower
///*
int rbinom(int n, double p)
{
    if (p <= 0.0) return(0);
    if (p >= 1.0) return(n);

//    if (n > 5) return(MSVC_rbinom(n, p)); ????  Makes it 2x slower with my data
    if (n > 100) return(MSVC_rbinom(n, p));

    int x = 0;
    int P = (int)(p * RAND_MAX);

    for (int i = 0; i < n; i++) {
        if (rand() < P) {
            x++;
        }
    }

    return (x);
}
//*/

// Non random alternative for comparison with redistribution of photons fractionally
// Should return n*p
// But result needs to remain integer for saving back to uint16
// To use, uncomment and comment the real version.
// This is better tested in R.
/*
int rbinom(int n, double p)
{
    return ((int)((double)n * p));
}
*/

/*
Set functions for bin_width_factors (timebins * detectors) and time shifts (per detector)

*/

/*

i = in bin
j = out bin
f = amount of time to fill in bin j
t = time available in bin i
n = photons available in bin i
N = photons passing i to j


*/
USHORT* combined_correction(USHORT* trans, int nbins, 
    double bin_borders[], int bin_jindexes[])
{
    USHORT* new_Int;

    new_Int = (USHORT*)calloc(nbins, sizeof(USHORT));
    if (new_Int == NULL) return NULL;

    for (int i = 0; i < nbins; i++) {
        int j, N;
        double f, p;

        double b1 = bin_borders[i];
        double b2 = bin_borders[i + 1];  // by design it has nbins+1 values

        double t = b2 - b1;
        if (t <= 0.0) continue;    // bin i has no width (!)

        int n = (int)trans[i];
        if (n <= 0) continue;      // bin i has no photons

        int bj1 = bin_jindexes[i];
        int bj2 = bin_jindexes[i+1];  // by design it has nbins+1 values

        j = bj1;
        f = min(1 - (b1 - bj1), t); // pre, not more than what is available
        p = f / t;
        N = rbinom(n, p);
        if(j>=0 && j<nbins)
            new_Int[j] += N;
        j = j + 1;
        t = t - f;
        n = n - N;

        if (bj1 < nbins) {   // if lower bin limit is after the transient there is nothing to do
            if (bj2 >= 0) {   // if upper bin limit is before the transient there is nothing to do
                while (j < bj2) {
                    f = 1; // whole
                    p = f / t;
                    N = rbinom(n, p);
                    if (j >= 0 && j < nbins)
                        new_Int[j] += N;
                    j = j + 1;
                    t = t - f;
                    n = n - N;
                }
            }
        }

        //p = 1; # remainder
        N = n;
        if (j >= 0 && j < nbins)
            new_Int[j] += N;

    }
          
    return(new_Int);
}


int correct_transient(USHORT* trans, int nbins, double bin_borders[], int bin_jindexes[])
{
    if (trans == NULL) return(-1);

    USHORT* signal = combined_correction(trans, nbins, bin_borders, bin_jindexes);

    if (signal == NULL) {
        return(-2);
    }

    memcpy(trans, signal, nbins * sizeof(USHORT));

    free(signal);

    return(0);
}

void calc_bin_borders(double* bin_width_factors, int nbins, double shift, double scale, double **times, int **jvals)
{
    int nvals = nbins + 1;
    double* vals = (double*)malloc(nvals * sizeof(double));
    int* jval = (int*)malloc(nvals * sizeof(int));
    double t = -shift;
    for (int i = 0; i < nvals; i++) {
        vals[i] = t;
        jval[i] = (int)ceil(t) - 1;
        t += bin_width_factors[i] * scale;
    }

    *times = vals;
    *jvals = jval;
}

/// Struct to hold info for each thread for thread_correct

typedef struct
{
    USHORT* image;
    int width;
    int height;
    int timebins;
    int start_row, stop_row;

} thread_correct_info;

void thread_correct(void* param)
{
    USHORT* trans = NULL;
    double* bin_width_factors = NULL;

    thread_correct_info* info = (thread_correct_info*)param;

    USHORT* image = info->image;
    int width = info->width;
    int height = info->height;
    int timebins = info->timebins;
    int start = info->start_row;
    int stop = info->stop_row;

    trans = &(image[start * width * timebins]);   // init to first transient
    bin_width_factors = &(gBinWidthFactors[start * timebins]);  // init to factors for first pixel

    int k = start * width;  // index into gTimebaseShifts and gTimebaseScales

    for (int i = start; i < stop; i++) {
        for (int j = 0; j < width; j++) {

			// calculate the bin borders for transient in this pixel
			double* bin_borders;
			int* bin_jindexes;
			calc_bin_borders(bin_width_factors, timebins, gTimebaseShifts[k], gTimebaseScales[k], &bin_borders, &bin_jindexes);

            correct_transient(trans, timebins, bin_borders, bin_jindexes);
            trans += timebins; // next transient
            k++;

			free(bin_borders);
			free(bin_jindexes);

			bin_width_factors += timebins; // factors for next pixel
        }
    }
}


/*

Correct for INL and DNL

image must be a sorted 3D time resolved image.

uses global bin width factors etc.

*/

int SPAD_CorrectTransients(USHORT* image, int width, int height, int timebins)
{
    const int nThreads = 16;
    HANDLE hThread[16];
    thread_correct_info info[16];
    int rows_per_thread = height / nThreads;
    int i;

    // DEBUG with single thread
    //return (SPAD_CorrectTransients_SingleThread(image, width, height, timebins));

    if (!gBinWidthFactors && !gTimebaseShifts && !gTimebaseScales) {
        printf("Warning: No calibration set, nothing to do!\n");
        return (0);
    }

    if (!gBinWidthFactors) SPAD_reset_bin_width_factors(width, height, timebins);
    if (!gTimebaseShifts) SPAD_reset_timebase_shifts(width, height, timebins);
    if (!gTimebaseScales) SPAD_reset_timebase_scales(width, height);

    // Seed random number generation
    srand((unsigned int)time(NULL));

    clock_t tStart = clock();
    printf("Starting %d threads\n", nThreads);
    for (i = 0; i < nThreads - 1; i++) {

        info[i].image = image;
        info[i].width = width;
        info[i].height = height;
        info[i].timebins = timebins;
        info[i].start_row = rows_per_thread * i;
        info[i].stop_row = info[i].start_row + rows_per_thread;

        hThread[i] = (HANDLE)_beginthread(thread_correct, 0, &info[i]);
    }

    // Last thread gets remaining rows
    info[i].image = image;
    info[i].width = width;
    info[i].height = height;
    info[i].timebins = timebins;
    info[i].start_row = rows_per_thread * i;
    info[i].stop_row = height;

    hThread[i] = (HANDLE)_beginthread(thread_correct, 0, &info[i]);

    // Wait for threads to end
    DWORD ret = WaitForMultipleObjects(nThreads, hThread, TRUE, ULONG_MAX);   // wait for as long as we can
    if (ret == WAIT_TIMEOUT) {
        printf("ERROR: THREAD TIMEOUT\n");
        return(-1);
    }
    printf("Finished threads: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

    return(0);
}


int SPAD_CorrectTransients_SingleThread(USHORT* image, int width, int height, int timebins)
{
    USHORT* trans = NULL;
    double* bin_width_factors = NULL;

    if (!gBinWidthFactors) SPAD_reset_bin_width_factors(width, height, timebins);
    if (!gTimebaseShifts) SPAD_reset_timebase_shifts(width, height, timebins);
    if (!gTimebaseScales) SPAD_reset_timebase_scales(width, height);

    trans = image;   // init to first transient
    bin_width_factors = gBinWidthFactors;  // init to factors for first pixel

    // Seed random number generation
    srand((unsigned int)time(NULL));

    int k = 0;  // index into gTimebaseShifts and gTimebaseScales

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {

			double* bin_borders;
			int* bin_jindexes;
			calc_bin_borders(bin_width_factors, timebins, gTimebaseShifts[k], gTimebaseScales[k], &bin_borders, &bin_jindexes);

            correct_transient(trans, timebins, bin_borders, bin_jindexes);
            trans += timebins; // next transient
            k++;

			free(bin_borders);
			free(bin_jindexes);

			bin_width_factors += timebins; // factors for next pixel
        }
    }
    return(0);
}
