#pragma once
#include "CashMath.h"

#include "SDL3/SDL.h"

struct Renderer
{
    SDL_Window* window;
    SDL_Renderer* context;
    Vec2I screen_size;
    Vec2I window_size;
};
extern Renderer gfx;

bool RenderInit();
void RenderPresent();
void RenderDestroy();
