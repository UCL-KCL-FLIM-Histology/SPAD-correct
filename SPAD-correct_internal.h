#pragma once

#ifndef _INTERNAL_H_ 
#define _INTERNAL_H_

#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "libics-1.6.2\libics.h"   // libics was originally only in the test program, now import it here

// This is turned on for the release version, intended to be used in LV. 
// CLI programs like SPAD-correct will not use the dll but build in the code and so not have this switch
//#define LABVIEW_PRINTF
#ifdef LABVIEW_PRINTF
#include "labview_lib\extcode.h"
#define printf DbgPrintf
#endif


// Globar Vars
extern int frame;

extern BYTE* integration_list[];
extern int integration_count;

// Predeclarations
int SPAD_readHeader(BYTE* data, unsigned long long nBytes, ICS* imagefile);

#endif // _INTERNAL_H_
