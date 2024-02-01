/****************************  instrset_enum.h   **********************************
* Author:        Agner Fog
* Date created:  2024-02-01
* Last modified: 2024-02-01
* Version:       2.02.02
* Project:       vector class library
* Description:
* An `enum class` wrapper around the INSTRSET macro. This file contains:
*
* > An *instrset* `enum class`.
* > *detected_instrset* value of *instrset* corresponding to the INSTRSET macro value.
*
* For instructions, see vcl_manual.pdf
*
* © Copyright 2012-2024 Agner Fog.
* Apache License version 2.0 or later.
******************************************************************************/

#pragma once
#ifndef VECTORCLASS_instrset_enum_
#define VECTORCLASS_instrset_enum_ 20240201L

#include "instrset.h"

#ifdef VCL_NAMESPACE
namespace VCL_NAMESPACE {
#endif

// See §9.9 of https://github.com/vectorclass/manual/raw/master/vcl_manual.pdf
enum class instrset
{
	X386 = 0,
	SSE = 1,
	SSE2 = 2,
	SSE3 = 3,
	SSSE3 = 4, // Supplementary SSE3
	SSE4_1 = 5, // SSE4.1
	SSE4_2 = 6,
	AVX = 7,
	AVX2 = 8,
	AVX512F = 9,
	AVX512VL = 10, AVX512BW = 10, AVX512DQ = 10,

	// From **instrset.h**: "In the future, INSTRSET = 11 may include AVX512VBMI and AVX512VBMI2 ..."
	// AVX512VBMI = 11, AVX512VBMI2 = 11,
};
constexpr auto detected_instrset = static_cast<instrset>(INSTRSET);

//// "Additional instruction set extensions are not necessarily part of a linear sequence."
//enum class instrset_extensions
//{
//	FMA3,
//	AVX512ER,
//	AVX512VBMI,
//	AVX512VBMI2,
//	F16C,
//	AVX512FP16
//};

#ifdef VCL_NAMESPACE
}
#endif

#endif // VECTORCLASS_instrset_enum_