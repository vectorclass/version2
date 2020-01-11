/****************************  easy_dispatch.h  *******************************
Author:        Bastian Erdnuess
Date created:  2020-01-05
Last modified: 2020-01-07
Version:       2.01.00
Project:       vector class library
Description:   Support for automatic CPU dispatching

This file defines three main macros that help to write dispatched function
versions.  These are: VCL_DISPATCH, VCL_DISPATCHED, and VCL_DISPATCHER.

VCL_DISPATCH is to be used in a header file to write a wrapper function around
the function pointer with the dispatched version:

    inline int myfunction(MyClass obj, float * ptr, int mode = 0) {
        VCL_DISPATCH(myfunction, std::move(obj), ptr, mode);
    }

This forwards the call to the dispatched function.  Note that lager objects
(like 'obj') should probably be forwarded by rvalue reference (std::move).
Alternatively the dispatch function pointer (which will be named
'vcl_myfunction_ptr' by 'VCL_DISPATCHER(myfunction)', see below) could be
called directly like 'vcl_myfunction_ptr(obj, ptr, mode)'.  The advantage
of using a wrapper like above is that one can specify default values for
parameters and that IDEs are able to analyse the function signature and give
proper suggestions and documentation for the prameters.  As the wrapper
should go in a header file as replace for the usual function declaration,
the wrapper must be 'inline' to not violate the one definition rule (ODR)
when the header gets included in several translation units.

VCL_DISPATCHED is used for the function definition in a cpp file and tags
the function name with the current instruction set that the cpp file is
compiled for:

    int VCL_DISPATCHED(myfuntion)(MyClass obj, float * ptr, int mode) {
        // do the work
    }

VCL_DISPATCHER should be usually included in the same cpp file and defines
the dispatcher:

    VCL_DISPATCHER(myfunction);

It ensures the dispatcher to be only included in one translation unit to
not violate the ODR.  For this to work correctly, the macro VCL_INSTRSETS
must be correctly specified before including this header file.

VCL_INSTRSETS has to be a comma separated list of the instruction sets
that the dispatched function shoud be compiled for, where 'instruction
set' denotes the number that the macro INSTRSET is set to by the header
'intsrsets.h' in the vector class library during compilation.  The list
has to be sorted ascendent as the dispatcher will chose the last
instruction set compatible with the host machine (first from the end of
the list).  The macro will be usually set as compiler switch by the
build system, e.g.

    g++ <other flags> -DVCL_INSTRSETS=5,8,10 <other parameter>

and hast to be supplied to all compiled versions of the tranlation unit.
The dispatcher will be only implemented in the first (lowest) INSTRSET
(e.g. here: 5).  The translation unit implementing the dispatcher will
also define the macro VCL_DISPATCHER_IMPLEMENTED_HERE.

If VCL_INSTRSETS is not defined, assumes 'single dispatch build' (e.g.
DEBUG build in IDE) and set VCL_INSTRSETS to INSTRSET so that the
dispatcher gets also implemented in the current (single) translation
unit.

The translation unit must be compiled for all instruction sets specified
in VCL_INSTRSETS otherwise linking will fail.  If it is compiled for
additional instruction sets not specified in VCL_INSTRSTETS they will
not be considered for dispatch and effectively ignored and probably
removed during linking an executable.

(c) Copyright 2020 Bastian Erdnuess.
Apache License version 2.0 or later.
******************************************************************************/
#ifndef EASY_DISPATCH_H
#define EASY_DISPATCH_H

#include <utility>

#include "instrset.h"

#define VCL_CONCAT_(fun, n) vcl_##fun##_##n // naming convention of derived symbols
#define VCL_CONCAT(fun, n) VCL_CONCAT_(fun, n) // add extra pass of macro substitution to reduce 'n'
#define VCL_DISPATCH(fun, ...) \
    extern decltype(fun) * VCL_CONCAT(fun, ptr); \
    return VCL_CONCAT(fun, ptr)(__VA_ARGS__)

//^ VCL_DISPATCH
//
//  Example:
//
//      inline void myfun(int i, int j = 0) {
//          VCL_DISPATCH(myfun, i, j);
//      }
//
//  Expands to:
//
//      inline void myfun(int i, int j = 0) {
//          extern decltype(myfun) * vcl_myfun_ptr;
//          return vcl_myfun_ptr(i, j);
//      }
//
//  This declares the function ptr 'vcl_myfun_ptr' with the signature
//  of 'myfun' and delegates the call to it.  The function is 'inline'
//  because it is supposed to be in a header file replacing the usual
//  function declaration.  'vcl_myfun_ptr' points to the best suited
//  implementation for the running machine (and initially to the dispatcher).
//  Note that despite the 'return' statement, this also works with
//  'void' functions!

#define VCL_DISPATCHED(fun) VCL_CONCAT(fun, INSTRSET)

//^ VCL_DISPATCHED
//
//  Example:
//
//      void VCL_DISPATCHED(myfun)(int i, int j) {
//          // do something interesting
//      }
//
//  Here, VCL_DISPATCHED(myfun) expands e.g. to 'vcl_myfun_5' if the
//  translation unit gets compiled with 'INSTRSET == 5'.

template <typename T, T * f, int i>
T * vcl_select(int, std::integer_sequence<int, i>) {
    return f; // in any case return the last possibility
}

template <typename T, T * f, T * ... fs, int i, int ... is>
T * vcl_select(int n, std::integer_sequence<int, i, is ...>) {
    T * g = vcl_select<T, fs ...>(n, std::integer_sequence<int, is ...>()); // transverse the list from the end
    if (g) return g;
    if (i <= n) return f;
    return nullptr;
}

template <typename T, T * & ptr, typename Instrsets, T * ... fs,
          typename R, typename ... A, R(A ...) = static_cast<T *>(nullptr)> // return type and argument deduction from T
R vcl_dispatch(A ... a) {
    ptr = vcl_select<T, fs ...>(instrset_detect(), Instrsets());
    return f(std::forward<A>(a) ...);
}

//^ vcl_dispatch: Selects from a pair of lists with available instruction sets 'Instrsets'
//  (e.g. Instrsets = std::integer_sequence<int, 5, 8>) and according function pointer 'fs'
//  (e.g. fs = <vcl_myfun_5, vcl_myfun_8> parameter pack) the last function pointer (first
//  from the end of the list) that is compatible with the host platform (therefore, the
//  list must be ordered asscending) and replaces the global function pointer of the function
//  (e.g. &ptr == &vcl_myfunc_ptr) with it for immediate access in subsequent calls.  Finally,
//  the current call is forwarded to the selected implementation 'f'.  In case no better
//  implementation is found, the first in the list (last from the end) is selected.  This is
//  the "safest" bet as it should correspond to the implementation with the most basic
//  instruction set

#define VCL_FUNLIST_1(fun,  n)      VCL_CONCAT(fun, n)
#define VCL_FUNLIST_2(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_1(fun, __VA_ARGS__)
#define VCL_FUNLIST_3(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_2(fun, __VA_ARGS__)
#define VCL_FUNLIST_4(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_3(fun, __VA_ARGS__)
#define VCL_FUNLIST_5(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_4(fun, __VA_ARGS__)
#define VCL_FUNLIST_6(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_5(fun, __VA_ARGS__)
#define VCL_FUNLIST_7(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_6(fun, __VA_ARGS__)
#define VCL_FUNLIST_8(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_7(fun, __VA_ARGS__)
#define VCL_FUNLIST_9(fun,  n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_8(fun, __VA_ARGS__)
#define VCL_FUNLIST_10(fun, n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_9(fun, __VA_ARGS__)
#define VCL_FUNLIST_11(fun, n, ...) VCL_CONCAT(fun, n), VCL_FUNLIST_10(fun, __VA_ARGS__)
#define VCL_CALL(MACRO, ...) MACRO(__VA_ARGS__) // add extra pass of macro substitution to reduce macro name
#define VCL_FUNLIST_(fun, n) VCL_CALL(VCL_FUNLIST_##n, fun, VCL_INSTRSETS)
#define VCL_FUNLIST(fun, n) VCL_FUNLIST_(fun, n)

#define VCL_GET_12TH(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, N, ...) N
#define VCL_COUNT_ARGS(...) VCL_GET_12TH(__VA_ARGS__, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) // poor man's counter
#define VCL_DISPATCHER_IMPLEMENTATION(fun) VCL_DISPATCHER_IMPLEMENTATION_(fun, VCL_FUNLIST(fun, VCL_COUNT_ARGS(VCL_INSTRSETS)))
#define VCL_DISPATCHER_IMPLEMENTATION_(fun, ...) \
    extern decltype(fun) __VA_ARGS__; \
    extern decltype(fun) * VCL_CONCAT(fun, ptr); /* prevent warnings */ \
    decltype(fun) * VCL_CONCAT(fun, ptr) = vcl_dispatch<decltype(fun), VCL_CONCAT(fun, ptr), std::integer_sequence<int, VCL_INSTRSETS>, __VA_ARGS__>

//^ VCL_DISPATCHER_IMPLEMENTATION
//
//  Implements the actual dispatcher.  Example: VCL_DISPATCHER_IMPLEMENTATION(myfun) expands e.g. to:
//
//      extern decltype(myfun) vcl_myfun_5, vcl_myfun_8;
//      decltype(myfun) * vcl_myfun_ptr = vcl_dispatch<decltype(myfun), vcl_myfun_ptr, std::integer_sequence<int, 5, 8>, vcl_myfun_5, vcl_myfun_8>
//
//  for 'VCL_INSTRSETS = 5, 8'.  This uses that C and C++ are not picky about functions beeing passed
//  as pointers or not (whether vcl_myfun_5 is declared as *vcl_myfun_5 or without * and passed to
//  'vcl_dispatch' as &vcl_myfun_5 or without &.  'vcl_dispatch' selects then the best suited
//  implementation and replaces 'vcl_myfun_ptr' with it in the first call of 'myfun'.

#ifndef VCL_INSTRSETS
#define VCL_INSTRSETS INSTRSET
#endif
//^ If VCL_INSTRSETS is not defined, assume debug build in IDE or therelike and that only
//  one imlplementation of 'fun' is available (the one in the current translation unit).
//  Set VCL_INSTRSETS accordantly so that the dispatcher gets implemented in this unit.

#define VCL_GET_1ST_(N, ...) N
#define VCL_GET_1ST(SET) VCL_GET_1ST_(SET, _) // add extra spacer '_' to ensure __VA_ARGS__ is not empty in VCL_GET_1ST_
#if INSTRSET == VCL_GET_1ST(VCL_INSTRSETS)
#define VCL_DISPATCHER_IMPLEMENTED_HERE // announce that this is the translation unit implementing the dispatcher(s)
#define VCL_DISPATCHER(fun) VCL_DISPATCHER_IMPLEMENTATION(fun)
//^ Only implement the dispatcher in the first translation unit (VCL_DISPATCHER is 'active').
//  The first translation unit is for the most elementary instruction set.  It is choosen,
//  because the dispatcher must be implemented in an instruction set that is compatible with
//  all potential host platforms.
#else
#define VCL_DISPATCHER(fun) // Otherwise 'deactivate' VCL_DISPATCHER macro
#endif

#endif // EASY_DISPATCH_H
