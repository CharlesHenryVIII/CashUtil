#pragma once
#include "CashMath.h"

struct Theme {
    const char* name;
    void (*function)(void);
};

enum ThemeColor : i32 {
    ThemeColor_DefaultDark,
    ThemeColor_DefaultLight,
    ThemeColor_DefaultClassic,
    ThemeColor_GreenAccent,
    ThemeColor_RedAccent,
    ThemeColor_Grey,
    ThemeColor_Grey2,
    ThemeColor_WildCard,
    ThemeColor_Quantum,
    ThemeColor_Count,
};

enum ThemeStyle : i32 {
    ThemeStyle_Basic,
    ThemeStyle_Original,
    ThemeStyle_SimpleRounding,
    ThemeStyle_Grey,
    ThemeStyle_Count,
};


struct ThemeSettings {
    ThemeColor color = ThemeColor_Quantum;
    ThemeStyle style = ThemeStyle_SimpleRounding;
};
extern ThemeSettings g_theme_settings;

extern Theme g_ThemeColorOptions[ThemeColor_Count];
extern Theme g_ThemeStyleOptions[ThemeStyle_Count];


void ThemesInit();
void ThemeSetColor(i32 color);
void ThemeSetStyle(i32 style);
const char* GetCStringFromThemes(void* data, int idx);
