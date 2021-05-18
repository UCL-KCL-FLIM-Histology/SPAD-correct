// SPAD-correct.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// Debug command lines:
//  -i "D:\Images\SPAD\UCL CD80-PDL1 Sep 2020\FLIM_Well4_400s_4\FLIM_Well4_400s_4.ics" -bwf "D:\Images\SPAD\binwidth_factors.dat" -tsh "D:\Images\SPAD\timebase_shifts.dat" -tsc "D:\Images\SPAD\timebase_scales.dat" -scr "D:\Images\SPAD\screamers_list.txt"
//  -i "D:\Images\SPAD\KCL\calibration\inl shift.ics" -bwf "D:\Images\SPAD\KCL\calibration\binwidth_factors.dat" -tsh "D:\Images\SPAD\KCL\calibration\timebase_shifts.dat" -tsc "D:\Images\SPAD\KCL\calibration\timebase_scales.dat" -nscr


#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <time.h>
#include <pathcch.h>
#include "cmdparser.hpp"
#include "SPAD-correct.h"
#include "SPAD-correct_internal.h"

size_t get_file_list(const char* searchkey, std::vector<std::string>& list)
{
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(searchkey, &fd);   // FindFirstFileA uses non-wide strings, FindFirstFile uses wide strings but cmdparser does not support them

    if (h == INVALID_HANDLE_VALUE)
    {
        return 0; // no files found
    }

    while (1)
    {
        list.push_back(fd.cFileName);

        if (FindNextFileA(h, &fd) == FALSE)
            break;
    }
    return list.size();
}

void setup_file_paths(const char* path, const char* filename, const char* suffix, char* datafilepath, char* savefilepath, char* intensitysavefilepath)
{
    char extension[] = { ".ics" };

    strcpy_s(datafilepath, MAX_PATH, path);
    strcat_s(datafilepath, MAX_PATH, "\\");
    strcat_s(datafilepath, MAX_PATH, filename);

    char name[MAX_PATH];
    strcpy_s(name, MAX_PATH, filename);
    char* last_dot = strrchr(name, '.');
    if (last_dot) {
        *last_dot = '\0';  // terminate string here
    }

    strcpy_s(savefilepath, MAX_PATH, path);
    strcat_s(savefilepath, MAX_PATH, "\\");
    strcat_s(savefilepath, MAX_PATH, name);
    strcat_s(savefilepath, MAX_PATH, suffix);
    strcat_s(savefilepath, MAX_PATH, extension);

    strcpy_s(intensitysavefilepath, MAX_PATH, path);
    strcat_s(intensitysavefilepath, MAX_PATH, "\\");
    strcat_s(intensitysavefilepath, MAX_PATH, name);
    strcat_s(intensitysavefilepath, MAX_PATH, suffix);
    strcat_s(intensitysavefilepath, MAX_PATH, "_clean_intensity");
    strcat_s(intensitysavefilepath, MAX_PATH, extension);
}

void configure_parser(cli::Parser& parser) {

    // Default all settings files to those located next to the exe
    char exedir[MAX_PATH];
    char default_binwidth_factors_path[MAX_PATH];
    char default_timebase_shifts_path[MAX_PATH];
    char default_timebase_scales_path[MAX_PATH];
    GetModuleFileNameA(0, exedir, MAX_PATH - 1);
    *strrchr(exedir, 'S') = 0;    // the module filename will be ...\\SPAD-correct.exe, terminate the string at the last 'S'

    sprintf_s(default_binwidth_factors_path, "%sbinwidth_factors.dat", exedir);
    sprintf_s(default_timebase_shifts_path, "%stimebase_shifts.dat", exedir);
    sprintf_s(default_timebase_scales_path, "%stimebase_scales.dat", exedir);

    parser.set_required<std::string>("i", "input", "Path to input file. Wildcards allowed in the filename.");
    parser.set_optional<std::string>("s", "suffix", "_corrected", "Output filename suffix.");
    parser.set_optional<std::string>("bwf", "binwidth-factors-file", default_binwidth_factors_path, "Bin width factors calibration file (binary).");
    parser.set_optional<std::string>("tsh", "timebase-shifts-file", default_timebase_shifts_path, "Timebase shifts calibration file (binary).");
    parser.set_optional<std::string>("tsc", "timebase-scales-file", default_timebase_scales_path, "Timebase scales calibration file (binary).");
    parser.set_optional<bool>("nbwf", "no-binwidth-factors", false, "Turn off the bin width correction.");
    parser.set_optional<bool>("ntsh", "no-timebase-shifts", false, "Turn off the timebase shift correction.");
    parser.set_optional<bool>("ntsc", "no-timebase-scales", false, "Turn off the timebase scale correction.");
    parser.set_optional<int>("b", "binning", 0, "Bin after correction by b x b.");

    // Examples
    //parser.set_optional<std::string>("o", "output", "data", "Strings are naturally included.");
    //parser.set_optional<int>("n", "number", 8, "Integers in all forms, e.g., unsigned int, long long, ..., are possible. Hexadecimal and Ocatl numbers parsed as well");
    //parser.set_optional<cli::NumericalBase<int, 10>>("t", "temp", 0, "integer parsing restricted only to numerical base 10");
    //parser.set_optional<double>("b", "beta", 11.0, "Also floating point values are possible.");
    //parser.set_optional<bool>("a", "all", false, "Boolean arguments are simply switched when encountered, i.e. false to true if provided.");
    //parser.set_required<std::vector<std::string>>("v", "values", "By using a vector it is possible to receive a multitude of inputs.");
}

int once_only(const char* path, const char* filename, cli::Parser& parser, int w, int h, int t)
{
    std::string sss;
    char* dataPath;
    bool nbwf = parser.get<bool>("nbwf");
    bool ntsh = parser.get<bool>("ntsh");
    bool ntsc = parser.get<bool>("ntsc");

    // If none of the following corrections, shortcut to end, this will also shortcut SPAD_CorrectTransients
    if (nbwf && ntsh && ntsc)
        return(0);

    if (nbwf) {
        if (SPAD_reset_bin_width_factors(w, h, t) < 0)
            return(-1);
    }
    else {
        printf("SPAD_read_bin_width_factors_from_file\n");
        sss = parser.get<std::string>("bwf");
        dataPath = (char*)sss.c_str();
        if (SPAD_read_bin_width_factors_from_file(dataPath, w, h, t) < 0)
            return(-2);
    }

    if (ntsh) {
        if (SPAD_reset_timebase_shifts(w, h, t) < 0)
            return(-3);
    }
    else {
        printf("SPAD_read_timebase_shifts_from_file\n");
        sss = parser.get<std::string>("tsh");
        dataPath = (char*)sss.c_str();
        if (SPAD_read_timebase_shifts_from_file(dataPath, w, h) < 0)
            return(-4);
    }

    if (ntsc) {
        if (SPAD_reset_timebase_scales(w, h) < 0)
            return(-5);
    }
    else {
        printf("SPAD_read_timebase_scales_from_file\n");
        sss = parser.get<std::string>("tsc");
        dataPath = (char*)sss.c_str();
        if (SPAD_read_timebase_scales_from_file(dataPath, w, h) < 0)
            return(-6);
    }

    return(0);
}

int process(const char* path, const char* filename, cli::Parser& parser)
{
    USHORT* image;
    int w, h, t, final_w, final_h;
    char datafilepath[MAX_PATH];
    char savefilepath[MAX_PATH];
    char intensitysavefilepath[MAX_PATH];
    double xy_microns_per_pixel, ns_per_bin, scale;
    static int first_time = 1;

    setup_file_paths(path, filename, parser.get<std::string>("s").c_str(), datafilepath, savefilepath, intensitysavefilepath);

    printf("SPAD_load3DICSfile...");
    clock_t tStart = clock();
    if (SPAD_load3DICSfile(datafilepath, &image, &w, &h, &t) < 0) {
        printf("\nERROR: Failed to load %s\n", datafilepath);
        return(-1);
    }
    printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

    if (first_time) {
        if (once_only(path, filename, parser, w, h, t) < 0) {
            free(image);
            return(-2);
        }
        first_time = 0;
    }

    printf("get metadata...");
    tStart = clock();
    ICS* ip;
    IcsOpen(&ip, datafilepath, "r");  // should work since we just opened above
    IcsGetPosition(ip, 0, NULL, &ns_per_bin, NULL);
    IcsGetPosition(ip, 1, NULL, &xy_microns_per_pixel, NULL);
    IcsClose(ip);
    printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

    printf("SPAD_CorrectTransients...");
    tStart = clock();
    if (SPAD_CorrectTransients(image, w, h, t) < 0) {
        free(image);
        return(-3);
    }
    printf(" time taken: %.3fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);


    int b = parser.get<int>("b");
    if (b > 1) {
        printf("SPAD_bin...");
        tStart = clock();
        SPAD_bin(image, w, h, t, b, &final_w, &final_h);
        printf(" time taken: %.3fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);
        scale = (double)w / (double)final_w;
    }
    else {
        final_w = w;
        final_h = h;
        scale = 1.0;
    }

    printf("SPAD_save3DICSfile: %s ...", savefilepath);
    tStart = clock();

    double new_ns_per_bin = SPAD_get_calibrated_timebase();
    if (new_ns_per_bin > 0) {   // a value was calculated
        ns_per_bin = new_ns_per_bin;
    }
    
    SPAD_save3DICSfile(savefilepath, image, final_w, final_h, t, 1, NULL, 0, scale*xy_microns_per_pixel, ns_per_bin);
    printf(" time taken: %.2fs\n", ((double)clock() - (double)tStart) / CLOCKS_PER_SEC);

    free(image);

    return(0);
}

int main(int argc, char** argv)
{
    std::vector<std::string> list;
    cli::Parser parser(argc, argv);

    configure_parser(parser);
    parser.run_and_exit_if_error();

    // Get search spec for files
    std::string searchpath = parser.get<std::string>("i");

    // Get path from file spec
    char path[MAX_PATH];
    strcpy_s(path, MAX_PATH, searchpath.c_str());
    char *last_slash = strrchr(path, '\\');
    if (last_slash) {
        *last_slash = '\0';  // terminate string here
    }
    else {
        strcpy_s(path, MAX_PATH, ".");
    }

    // Get filenames process each one
    size_t count = get_file_list(searchpath.c_str(), list);
    printf("Path: %s\n", path);

    if (count == 0) {
        printf("ERROR: Input filename did not match any existing files.\n");
        return(-1);
    }

    for (size_t i = 0; i < count; i++)
    {
        const char *filename = list[i].c_str();
        printf("%zd/%zd: %s\n", i+1, count, filename);
        if (process(path, filename, parser) < 0)
            break;   // error occurred
    }

    return(0);
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
