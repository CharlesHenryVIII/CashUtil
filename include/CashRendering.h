#pragma once
#include "CashMath.h"
#include "CashArrayView.h"

#include "SDL3/SDL.h"

enum EmbededIcon : u32
{
    EmbededIcon_Invalid,
    EmbededIcon_16,
    EmbededIcon_32,
    EmbededIcon_64,
    EmbededIcon_128,
    EmbededIcon_256,
    EmbededIcon_512,
    EmbededIcon_FullSize,
    EmbededIcon_Count,
};

struct Renderer
{
    SDL_Window* window;
    SDL_Renderer* context;
    Vec2I screen_size;
    Vec2I window_size;
};
extern Renderer gfx;

bool RenderInit(ArrayView<const ArrayView<const u8>> app_icons);
void RenderPresent();
void RenderDestroy();
