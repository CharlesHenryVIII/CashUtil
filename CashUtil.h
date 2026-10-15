#include "include/CashDebug.h"
#include "include/CashMath.h"
#include "include/CashArrayView.h"
#include "include/CashArray.h"
#include "include/CashString.h"
#include "include/CashThemes.h"
#include "include/CashThreading.h"
#include "include/CashRendering.h"
#include "include/CashSystem.h"
#include "include/CashConsole.h"
#include "include/CashFile.h"


//TODO(CSH): move this project to a seperate dependency in premake
//TODO(CSH): Rewrite gbRect and Rectangle to be consistant and use TopLeft vs BotRight or have those as functions
// Maybe also do Rect2 and Rect3 for 2d and 3d.
//TODO(CSH): Look into stb_sprintf
//TODO(CSH): Seperate DebugPrint() into a seperate CashLog() file
//TODO(CSH): Console allow wordwrap