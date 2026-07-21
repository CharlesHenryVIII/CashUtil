#include "CashRendering.h"
#include "CashDebug.h"
#include "CashSystem.h"
#include "resource.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"

Renderer gfx;

bool RenderInit()
{
    SDL_Init(SDL_INIT_VIDEO);

    {
        float normalRatio = 16.0f / 9.0f;
        SDL_Rect screen_size = {};
        SDL_GetDisplayBounds(0, &screen_size);
        gfx.screen_size = { screen_size.w, screen_size.h };
#if 1
        //gfx.window_size = { 1280, 720 };
        gfx.window_size = { 1024, 600 };
        gfx.window_size = gfx.window_size * SysMonitorScale();
#else
        float screen_scale = 1.5;
        if (displayRatio < normalRatio)
        {
            window_size.x = i32(float(mode->width) / screen_scale);
            window_size.y = i32(float(mode->width) / normalRatio);
        }
        else
        {
            window_size.y = i32(mode->height / screen_scale);
            window_size.x = i32(normalRatio * window_size.y);
        }
#endif
    }


    u32 window_flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    gfx.window = SDL_CreateWindow("Quool Tool", gfx.window_size.x, gfx.window_size.y, window_flags);
    if (!gfx.window)
    {
        DebugPrint("Failed to create window");
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
    gfx.context = SDL_CreateRenderer(gfx.window, nullptr);
    if (!gfx.context)
    {
        DebugPrint("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return false;
    }

    SDL_SetRenderVSync(gfx.context, 1);
    SDL_SetWindowPosition(gfx.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_Surface* icons;
    std::vector<void*> pixels_to_free;
    std::vector<SDL_Surface*> surfaces_to_free;
    for (i32 icon_id = IDB_PNGFULL; icon_id < IDB_PNGEND; icon_id++)
    {
        i32 size;
        void* data = SysGetDataFromResource(&size, icon_id);
        if (!data)
        {
            DebugPrint("Error: failed to get data from resource: %i", icon_id);
            FAIL;
            continue;
        }

        // ---- Decode PNG from memory ----
        Vec2I image_size;
        stbi_uc* pixels = stbi_load_from_memory(
            (const stbi_uc*)data,
            size,
            &image_size.x,
            &image_size.y,
            nullptr,
            4
        );
        if (!pixels)
        {
            DebugPrint("Warning: Failed to get pixels from memory icon_id: %i", icon_id);
            FAIL;
            continue;
        }


        //NOTE(CSH): Not sure why the pixels are inverted should be loaded as RGBA8888 but SDL_CreateSurface is seeing them as BGRA8888
        if (icon_id == IDB_PNGFULL)
        {
            icons = SDL_CreateSurfaceFrom(image_size.x, image_size.y, SDL_PIXELFORMAT_BGRA8888, pixels, sizeof(u32) * image_size.x);
        }
        else
        {
            SDL_Surface* icon = SDL_CreateSurfaceFrom(image_size.x, image_size.y, SDL_PIXELFORMAT_BGRA8888, pixels, sizeof(u32) * image_size.x);
            SDL_AddSurfaceAlternateImage(icons, icon);
            surfaces_to_free.push_back(icon);
        }
        pixels_to_free.push_back(pixels);
    }

    SDL_SetWindowIcon(gfx.window, icons);

    for (auto& p : surfaces_to_free)
        SDL_DestroySurface(p);
    for (auto& p : pixels_to_free)
        stbi_image_free(p);
    SDL_DestroySurface(icons);


    SDL_ShowWindow(gfx.window);
    return true;
}

void RenderPresent()
{
    ZoneScoped;
    static const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    SDL_SetRenderScale(gfx.context, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(gfx.context, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    SDL_RenderClear(gfx.context);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gfx.context);
    SDL_RenderPresent(gfx.context);
}

void RenderDestroy()
{
    SDL_DestroyRenderer(gfx.context);
}
