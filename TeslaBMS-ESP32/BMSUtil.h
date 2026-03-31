// BMSUtil.h — backward-compatibility shim.
// All communication logic has moved to BMSComm.h / BMSComm.cpp.
// Existing code that includes BMSUtil.h will continue to compile; new code
// should include BMSComm.h directly and use BMSComm::.
#pragma once
#include "BMSComm.h"
typedef BMSComm BMSUtil;
