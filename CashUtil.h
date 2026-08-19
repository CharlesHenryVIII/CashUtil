#include "include/CashDebug.h"
#include "include/CashMath.h"
#include "include/CashArrayView.h"
#include "include/CashString.h"
#include "include/CashThemes.h"
#include "include/CashThreading.h"
#include "include/CashWinInterop_File.h"
#include "include/CashRendering.h"
#include "include/CashSystem.h"


//TODO(CSH): redo wininterop_file
//TODO(CSH): move this project to a seperate dependency in premake
//TODO(CSH): Fix ArrayView.h and logging so that ArrayView.h can have access to logging
//TODO(CSH): Add Packager project
//TODO(CSH): Rewrite gbRect and Rectangle to be consistant and use TopLeft vs BotRight or have those as functions
// Maybe also do Rect2 and Rect3 for 2d and 3d.
//TODO(CSH): Look into stb_sprintf