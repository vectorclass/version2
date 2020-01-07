/***************************  easy_dispatch.cpp  ******************************
Author:        Bastian Erdnuess
Date created:  2020-01-06
Last modified: 2020-01-06
Version:       2.01.00
Project:       vector class library
Description:   Support for automatic CPU dispatching

Implement one virtual function of 'vcl_dispatch_exception' in a cpp file,
to prevent the emission of its vtable in each translation unit that includes
'easy_dispatch.h' (-Wweak-vtables).

The implementation of vcl_dispatch_exception::what() should be probably
moved to instrset_detect.cpp.

(c) Copyright 2020 Bastian Erdnuess.
Apache License version 2.0 or later.
******************************************************************************/

#include "easy_dispatch.h"

char const * vcl_dispatch_exception::what() const noexcept {
  return "Processor does not support least required instruction set";
}
