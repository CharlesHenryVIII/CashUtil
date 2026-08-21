#include "CashRendering.h"
#include "CashDebug.h"
#include "CashSystem.h"
#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"
#include "bgfx/bgfx.h"

Renderer gfx;

struct DxRenderer {
    bgfx::VertexLayout vert_layouts[VertexType_Count] = {};
    bgfx::VertexBufferHandle fullscreen_verts;
    bgfx::VertexBufferHandle fullscreen_verts;
};
static DxRenderer s_gfx;


bool RenderInitBgfx()
{
    bgfx::Init bgfx_init;
    bgfx_init.type = bgfx::RendererType::Count;
    bgfx_init.vendorId = BGFX_PCI_ID_NONE;
#if _DEBUG
    //bgfx_init.debug = true;
    //bgfx_init.profile = true;
#endif
    bgfx_init.fallback = true;
    //bgfx_init.platformData.ndt = entry::getNativeDisplayHandle();   //!< Native display type (*nix specific).
    bgfx_init.platformData.nwh = SysGetWindowHandle(gfx.window);
    bgfx_init.platformData.context = nullptr; //bgfx will create
    bgfx_init.platformData.queue = nullptr; //bgfx will create

    //TODO(CSH): Create myself
    bgfx_init.platformData.backBuffer = nullptr; //bgfx will create
    bgfx_init.platformData.backBufferDS = nullptr; //bgfx will create

#ifdef LINUX
    bgfx_init.platformData.type = bgfx::NativeWindowHandleType::Enum::Wayland; //!< Handle type. Needed for platforms having more than one option.
#else
    bgfx_init.platformData.type = bgfx::NativeWindowHandleType::Enum::Default; //!< Handle type. Needed for platforms having more than one option.
#endif

    //bgfx_init.limits;
    bgfx_init.resolution.formatColor = bgfx::TextureFormat::Enum::RGBA8U;
    bgfx_init.resolution.formatDepthStencil = bgfx::TextureFormat::Enum::UnknownDepth; //dont want/need
    bgfx_init.resolution.width = gfx.window_size.x;
    bgfx_init.resolution.height = gfx.window_size.y;
    bgfx_init.resolution.reset = BGFX_RESET_VSYNC;
    bgfx_init.resolution.numBackBuffers = 2;
    //bgfx_init.resolution.maxFrameLatency;//!< Maximum frame latency.
	//bgfx_init.resolution.debugTextScale;                 //!< Scale factor for debug text.

    if (!bgfx::init(bgfx_init))
    {
        DebugPrint("Failed to initialize bgfx");
        return false;
    }
#if _DEBUG
    //bgfx::setDebug(BGFX_DEBUG_TEXT || BGFX_DEBUG_STATS);
#endif

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH | BGFX_CLEAR_STENCIL, ToColorI(backgroundColor).rgba, 1.0f, 0);

    
    static_assert(VertexType_Count == 2); //NOTE(CSH): If this is wrong then you will need to update the vertex layout descriptions here:
    {
        //Vertex_2D
        s_gfx.vert_layouts[VertexType_2D]
            .begin()
            .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
        bgfx::createVertexLayout(s_gfx.vert_layouts[VertexType_2D]);
    }
    {
        //Vertex_PNTC
        s_gfx.vert_layouts[VertexType_PNTC]
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float, true)
            .end();
        bgfx::createVertexLayout(s_gfx.vert_layouts[VertexType_PNTC]);
    }

    Vec2  position;
    Vec2  uv;
    ColorI color;
    Vertex_2D verts[] = {
        { { -1.0f, +1.0f }, {}, ToColorI(White) }, // Top Left
        { { -1.0f, -3.0f }, {}, ToColorI(White) }, // Bot Left
        { { +3.0f, +1.0f }, {}, ToColorI(White) } // Top Right
    };

    s_gfx.fullscreen_verts = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);

    //bgfx::createIndexBuffer();
    //s_gfx.
    CreateTexture();
    return true;
}

bool RenderInit(ArrayView<const ArrayView<const u8>> app_icons)
{
    SDL_Init(SDL_INIT_VIDEO);

    {
        const float normalRatio = 16.0f / 9.0f;
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
#if 1
    RenderInitBgfx();
#else
#if defined(WIN32)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
#elif defined(LINUX)
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "vulkan");
#else
    #error "Unsupported platform for render driver configuration!"
#endif
    gfx.context = SDL_CreateRenderer(gfx.window, nullptr);
    if (!gfx.context)
    {
        DebugPrint("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(gfx.context, 1);
#endif

    SDL_SetWindowPosition(gfx.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    SDL_Surface* icons = nullptr;
    std::vector<void*> pixels_to_free;
    std::vector<SDL_Surface*> surfaces_to_free;
    const i32 min = Min((i32)EmbededIcon_Count, (i32)app_icons.size());
    for (i32 i = EmbededIcon_Invalid + 1; i < min; i++)
    {
        ArrayView<const u8> icon = app_icons[i];
        if (!icon.data)
        {
            DebugPrint("Error: failed to get data from resource: %i", i);
            FAIL;
            continue;
        }

        // ---- Decode PNG from memory ----
        Vec2I image_size;
        stbi_uc* pixels = stbi_load_from_memory(
            (const stbi_uc*)icon.data,
            (i32)icon.Bytes(),
            &image_size.x,
            &image_size.y,
            nullptr,
            4
        );
        if (!pixels)
        {
            DebugPrint("Warning: Failed to get pixels from memory icon_id: %i", i);
            FAIL;
            continue;
        }


        //NOTE(CSH): Not sure why the pixels are inverted should be loaded as RGBA8888 but SDL_CreateSurface is seeing them as BGRA8888
        if (i == EmbededIcon_FullSize)
        {
            icons = SDL_CreateSurfaceFrom(image_size.x, image_size.y, SDL_PIXELFORMAT_BGRA8888, pixels, sizeof(u32) * image_size.x);
        }
        else
        {
            SDL_Surface* temp_icon = SDL_CreateSurfaceFrom(image_size.x, image_size.y, SDL_PIXELFORMAT_BGRA8888, pixels, sizeof(u32) * image_size.x);
            SDL_AddSurfaceAlternateImage(icons, temp_icon);
            surfaces_to_free.push_back(temp_icon);
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

bool CreateTexture(Texture** texture, const void* data, Vec3I size, TextureFormat format, i32 bytes_per_pixel, const std::wstring& name, TextureType type = TextureType_Texture);
bool CreateTexture(Texture** texture, const char* fileLocation, TextureFormat format, TextureFilter filter, const std::wstring& name, TextureType type = TextureType_Texture);
bool CreateTexture(Texture** texture, const TextureParams& tp, u32 mip_levels, const u8* data);
bool CreateTexture(Texture** texture, const TextureParams& tp, const void* data);
bool UpdateTexture(Texture** texture, u32 mip_slice, void* data, u32 row_pitch_bytes, u32 depth_pitch_bytes);
void DeleteTexture(Texture** texture)
{

}
void* GetShaderResourceView(TextureIndex t);