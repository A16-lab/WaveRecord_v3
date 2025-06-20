#pragma once

#include "dwflib.h"

#ifdef _WIN32
    #ifdef EXPORT_FCNS
        #define EXPORTED_FUNCTION __declspec(dllexport)
    #else
        #define EXPORTED_FUNCTION __declspec(dllimport)
    #endif
#else
    #define EXPORTED_FUNCTION
#endif

/* ---- Function declarations ---- */
#include <time.h>
EXPORTED_FUNCTION void sleep( clock_t wait );

EXPORTED_FUNCTION int Array2In2OutConfigure(HDWF, double *, double *, double *, double *, double);

EXPORTED_FUNCTION int Array2In2OutRun(HDWF, double *, double *, double *, double *);

EXPORTED_FUNCTION int Stream2In2OutRun(HDWF, double  *, double *, double  *, double *, double, int);

EXPORTED_FUNCTION HDWF OpenDigilent( void );
 
EXPORTED_FUNCTION int CloseDigilent( HDWF hdwf );
