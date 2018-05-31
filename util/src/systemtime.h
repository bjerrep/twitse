#pragma once

#ifdef VCTCXO
#include "systemtime_vctcxo.h"
#else
#include "systemtime_std.h"
#endif

extern SystemTime* s_systemTime;
