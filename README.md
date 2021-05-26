# SPAD-correct

Command line programs to correct for the differences in time-resolved SPAD detectors in an array. Uses ICS v2 files for input and output (as used with TRI2).

Corrects for non-linear timebase, zero shift and overall time scale.

Calibration of the corrections can be done with the SPAD-calibrate program (below). This will also do screamer removal via the supplied list of screaming detectors, and binning if required. Now that the INL and DNL are corrected, functions like binning now behave well.
Type "SPAD-correct -h" for help:

Available parameters:

  -h    --help

   This parameter is optional. The default value is ''.

  -i    --input (required)
   Path to input file. Wildcards allowed in the filename.

  -s    --suffix
   Output filename suffix.
   This parameter is optional. The default value is '_corrected'.

  -bwf  --binwidth-factors-file
   Bin width factors calibration file (binary).
   This parameter is optional. The default value is '{path to executable}\binwidth_factors.dat'.

  -tsh  --timebase-shifts-file
   Timebase shifts calibration file (binary).
   This parameter is optional. The default value is '{path to executable}\timebase_shifts.dat'.

  -tsc  --timebase-scales-file
   Timebase scales calibration file (binary).
   This parameter is optional. The default value is '{path to executable}\timebase_scales.dat'.

  -scr  --screamers-file
   Sorted screamer list file (text).
   This parameter is optional. The default value is '{path to executable}\screamers_list.txt'.

  -nbwf --no-binwidth-factors
   Turn off the bin width correction.
   This parameter is optional. The default value is '0'.

  -ntsh --no-timebase-shifts
   Turn off the timebase shift correction.
   This parameter is optional. The default value is '0'.

  -ntsc --no-timebase-scales
   Turn off the timebase scale correction.
   This parameter is optional. The default value is '0'.

  -nscr --no-screamer-correction
   Turn off screamer correction.
   This parameter is optional. The default value is '0'.

  -rscr --remove-screamers
   Remove screamers rather than interpolating.
   This parameter is optional. The default value is '0'.

  -b    --binning
   Bin after correction by b x b.
   This parameter is optional. The default value is '0'.

# SPAD-calibrate

A command line program to generate calibration files for SPAD-correct
.
To get the full INL and DNL correction for variable bin widths, timebase shifts and timebase scaling, you must supply a "white" image taken with a constant light source, a "peak" image of a short lived fluorophore (or IRF) and a second peak image with a time delay.

SPAD-calibrate -h

  -h    --help

   This parameter is optional. The default value is ''.

  -p1   --peak1_image   (required)
   Path to input ics file with a peak in the normal position.

  -p2   --peak2_image   (required)
   Path to input ics file with a peak in a delayed position.

  -w    --white_image   (required)
   Path to input ics file acquired with constant light source.

  -st   --start_bin     (required)
   The first timebin to use from each signal. Usually 10 or so.

  -sp   --stop_bin      (required)
   The lasst timebin to use from each signal. Usually 245 maybe, from 256 bins.

  -d    --delta
   The delay between peaks 1 and 2 in real time will be used to calbrate the time axis. If -1.0 (default) the median time from the data will be used.
   This parameter is optional. The default value is '-1.000000'.

  -t    --test-files
   Produce text files and detector signals as well as data files to collect statistics on this detector array and test the output.
   This parameter is optional. The default value is '0'.

# Compilation

Only tested on Windows 10 x64 compiled with Microsoft Visual Studio Community 2019.
Use CMAKE and the CMakeLists.txt to create the solution/project, or try to use the sln file for Visual Studio from the build folder.
TODO: Both the above depend on the prebuilt lib for libics. This would have to be built for another platform.

 

