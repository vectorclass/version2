/***********************  easy_dispatch_example.cpp  **************************
Author:        Agner Fog
Date created:  2012-05-30
Last modified: 2017-07-01
Editors:       Bastian Erdnuess (2020-01-05--2020-01-07)
Version:       2.01.00
Project:       vector class library
Description:   Example of automatic CPU dispatching

# Example of compiling this with GCC compiler:
# Compile dispatch_example.cpp four times for different instruction sets:

# Compile for SSE4.1
g++ -O3 -msse4.1 -std=c++17 -c easy_dispatch_example.cpp -od5.o

# Compile for AVX2
g++ -O3 -mavx2 -mfma -std=c++17 -c easy_dispatch_example.cpp -od8.o

# Compile for AVX512
g++ -O3 -mavx512f -mfma -mavx512vl -mavx512bw -mavx512dq -std=c++17 -c easy_dispatch_example.cpp -od10.o

# The last compilation uses the lowest supported instruction set (SSE2)
# This includes the main program, and links all versions together:
# Note, that some newer compiler might need additionally a '-mno-sse3' switch to ensure SSE2-only
g++ -O3 -msse2 -std=c++17 -otest easy_dispatch_example.cpp easy_dispatch.cpp instrset_detect.cpp d5.o d8.o d10.o

# Run the program
./test

(c) Copyright 2012-2020 Agner Fog.
Apache License version 2.0 or later.
******************************************************************************/

#include <stdio.h>

#include "vectorclass.h"

#define VCL_INSTRSETS 2, 5, 8, 10 // order must be ascending! define before include of 'easy_dispatch.h'
                                  // or alternatively compile with -DVCL_INSTRSETS=2,5,8,10
#include "easy_dispatch.h"

// Define function, this should go in a header file, has to be 'inline' then.
// Here, define the function name, the return type, the argument types,
// the argument names (as they are usually shown in a IDE), and default values.
// Implement with VCL_DISPATCH, the function name and argument names have to be repeated.

inline float myfunc(float * f) {
    VCL_DISPATCH(myfunc, f);
}

// Dispatched version of the function.  Distinct function name is supplied by VCL_DISPATCHED.
// Compile this once for each instruction set:

float VCL_DISPATCHED(myfunc)(float * f) {
    printf("Selected instruction set: %d\n", INSTRSET);  // report selected instruction set

    // This example adds 16 floats
    Vec16f a;                          // vector of 16 floats
    a.load(f);                         // load array into vector
    return horizontal_add(a);          // return sum of 16 elements
}

// Implement dispatcher.  Dispatcher will be only implemented in the first INSTRSET of VCL_INSTRSETS.
// If VCL_INSTRSETS is not defined (at the point where easy_dispatch.h gets included), dispatcher
// will also be implemented (for easy testing and better IDE integration).  In the translation unit
// implementing the dispatcher the macro VCL_DISPATCHER_IMPLEMENTED_HERE will be defined.
// The dispatcher will throw a 'vcl_dispatch_exception' at the first call if the processor
// does not support the least compiled instruction set (here INSTRSET == 2, i.e. SSE2).

VCL_DISPATCHER(myfunc);

// Compile main function in only one translation unit with the most elementar instruction set
// (i.e. where the dispatcher is):

#ifdef VCL_DISPATCHER_IMPLEMENTED_HERE

// Example: main calls myfunc
int main() {
    printf("Supported instruction set: %d\n", instrset_detect());

    float a[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};  // array of 16 floats

    float sum = myfunc(a);                                 // call function with dispatching

    printf("\nsum = %8.2f \n", double(sum));               // print result
    return 0;
}

#endif // VCL_DISPATCHER_IMPLEMENTED_HERE
