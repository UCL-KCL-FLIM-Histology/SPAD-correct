CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#SET(CMAKE_CONFIGURATION_TYPES "Debug;VerboseDebug;Release" CACHE STRING "" FORCE)

# Set Debug build type to have the safe linker flags as debug
#SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "" FORCE)
#SET(CMAKE_EXE_LINKER_FLAGS_DEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
#SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)

# Set VerboseDebug build type to have the safe linker flags as debug
#SET(CMAKE_C_FLAGS_VERBOSEDEBUG "${CMAKE_C_FLAGS_DEBUG} /DVERBOSE_DEBUG /DFORTIFY" CACHE STRING "" FORCE)
#SET(CMAKE_EXE_LINKER_FLAGS_VERBOSEDEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)
#SET(CMAKE_SHARED_LINKER_FLAGS_VERBOSEDEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING "" FORCE)

PROJECT(SPAD-correct)

SET(SPAD_correct_SRCS 
	SPAD-correct.cpp
	SPAD-bin_width_factors.cpp
	SPAD-binning.cpp
	SPAD-corrections.cpp
	SPAD-timebase_scales.cpp
	SPAD-timebase_shifts.cpp
	SPAD-correct_IO.cpp
	SPAD-correct_metadata.cpp
	SPAD-correct.h
	SPAD-correct_internal.h
	cmdparser.hpp
)

SET(SPAD_calibrate_SRCS 
	SPAD-calibrate.cpp
	SPAD-bin_width_factors.cpp
	SPAD-binning.cpp
	SPAD-corrections.cpp
	SPAD-timebase_scales.cpp
	SPAD-timebase_shifts.cpp
	SPAD-correct_IO.cpp
	SPAD-correct_metadata.cpp
	SPAD-correct.h
	SPAD-correct_internal.h
	cmdparser.hpp
)

INCLUDE_DIRECTORIES(
	libics-1.6.2
)

#LINK_DIRECTORIES(	
#	libics-1.6.2/build/
#)

find_library(LIBICS_LIBRARY libics_static HINTS ./libics-1.6.2/build/Release)
	
SET(ALL_SRCS ${GELCOUNT_SRCS} ${APP_ACTIVATION_SRCS} ${LIBRARY_SRCS} ${GCI_IMAGE_IO} ${GELCOUNT_CHARM_SRCS} ${CVI_TOOLLIB_SRCS})
              
			
ADD_EXECUTABLE (SPAD-correct ${SPAD_correct_SRCS})
TARGET_LINK_LIBRARIES (SPAD-correct ${LIBICS_LIBRARY})

ADD_EXECUTABLE (SPAD-calibrate ${SPAD_calibrate_SRCS})
TARGET_LINK_LIBRARIES (SPAD-calibrate ${LIBICS_LIBRARY})

