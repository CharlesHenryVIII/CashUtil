#include "CashRendering.h"
#include "CashDebug.h"
#include "CashSystem.h"
#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
//#define SOKOL_IMPL //Already defined in CashOsWindows.cpp
#ifdef WIN32
#define SOKOL_D3D11
#elif defined(LINUX)
#define SOKOL_VULKAN
#elif defined(MACOS)
#define SOKOL_METAL
#else
#error Add support for other OS
#endif

#define SOKOL_IMGUI_IMPL
#include "sokol/sokol_gfx.h"
#include "sokol/util/sokol_imgui.h"
Renderer gfx;

//typedef std::array<sg_vertex_attr_state, SG_MAX_VERTEX_ATTRIBUTES> SokolVertexLayout;
//typedef sg_vertex_attr_state SokolVertexLayout[SG_MAX_VERTEX_ATTRIBUTES];
//using SokolVertexLayout = sg_vertex_attr_state[SG_MAX_VERTEX_ATTRIBUTES];
using SokolVertexLayout = std::array<sg_vertex_attr_state, SG_MAX_VERTEX_ATTRIBUTES>;

struct GfxDevice {
    SokolVertexLayout vertex_layouts[VertexType_Count]= {};
    GpuBuffer* fullscreen_verts = {};
    sg_pass_action pass_action = {};
};
static GfxDevice s_gfx;

struct GfxTexture;
struct GfxSampler;
struct GfxGpuBuffer;
struct GfxGpuBinding;

//void SysGetRenderEnvironment(sg_environment* env);

sg_color ToSgColor(Color color)
{
    sg_color r = {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
    return r;
}

void SgLogFunc(
        const char* tag,                // always "sg"
        uint32_t log_level,             // 0=panic, 1=error, 2=warning, 3=info
        uint32_t log_item_id,           // SG_LOGITEM_*
        const char* message_or_null,    // a message string, may be nullptr in release mode
        uint32_t line_nr,               // line number in sokol_gfx.h
        const char* filename_or_null,   // source filename, may be nullptr in release mode
        void* user_data
)
{
    ASSERT(user_data == nullptr);
    std::string log_level_s;
    switch (log_level)
    {
    case 0: log_level_s = "PANIC";      break;
    case 1: log_level_s = "ERROR";      break;
    case 2: log_level_s = "Warning";    break;
    case 3: log_level_s = "Info";       break;
    default: log_level_s = ToString("UNKNOWN LOG LEVEL (%s)", log_level); FAIL; break;
    }

    const std::string log = std::format("{} {}({}) SG_LOGITEM_{}: {}",
        log_level_s.c_str(),
        filename_or_null ? filename_or_null : "Unknown File",
        line_nr,
        log_item_id,
        message_or_null ? message_or_null : "(No Message)");
    DebugPrint("%s", log.c_str());
}

bool RenderInitSokol()
{
    SysRenderInitDesc sys_desc = {};
    sys_desc.size = gfx.window_size;
    sys_desc.sample_count = 1;
    sys_desc.no_depth_buffer = true;
    bool result = SysRenderInit(&sys_desc);
    sg_desc desc = { };
    //desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    //desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    SysGetRenderEnvironment(&desc.environment);
    //TODO(CSH): override allocator with custom frame temp memory?
    desc.allocator;     // optional memory allocation hooks.  Default is malloc and free
    desc.logger = { SgLogFunc, nullptr };
    sg_setup(&desc);


    static_assert(VertexType_Count == 2); //NOTE(CSH): If this is wrong then you will need to update the vertex layout descriptions here:
    //Vertex_2D
    s_gfx.vertex_layouts[VertexType_2D][0] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, position),  .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][1] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, uv),        .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][2] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, color),     .format = SG_VERTEXFORMAT_UINT };
    //Vertex_PNTC
    s_gfx.vertex_layouts[VertexType_PNTC][0] = { .buffer_index = 0, .offset = offsetof(Vertex_PNTC, position),  .format = SG_VERTEXFORMAT_FLOAT3 };
    s_gfx.vertex_layouts[VertexType_PNTC][1] = { .buffer_index = 0, .offset = offsetof(Vertex_PNTC, normal),    .format = SG_VERTEXFORMAT_FLOAT3 };
    s_gfx.vertex_layouts[VertexType_PNTC][2] = { .buffer_index = 0, .offset = offsetof(Vertex_PNTC, uv),        .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_PNTC][2] = { .buffer_index = 0, .offset = offsetof(Vertex_PNTC, color),     .format = SG_VERTEXFORMAT_FLOAT4 };
    //

    {
        Vertex_2D verts[] = {
            { { -1.0f, +1.0f }, {}, ToColorI(White) },  // Top Left
            { { -1.0f, -3.0f }, {}, ToColorI(White) },  // Bot Left
            { { +3.0f, +1.0f }, {}, ToColorI(White) }   // Top Right
        };

        CreateGpuBuffer(&s_gfx.fullscreen_verts, "Fullscreen Triangle VB", GpuBufferType_Vertex, GpuBufferFlag_Immutable);
        s_gfx.fullscreen_verts->Upload(verts);
    }

    s_gfx.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    s_gfx.pass_action.colors[0].clear_value = ToSgColor(background_color);

    return true;
}
void RenderDestroySokol()
{
    sg_shutdown();
    SysRenderDestroy();
}


bool CashRenderInit(ArrayView<const ArrayView<const u8>> app_icons)
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
        FAIL;
        return false;
    }

#if 1
    RenderInitSokol();
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

void CashImguiInit()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.IniFilename = NULL;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup scaling
    float main_scale = SysMonitorScale();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    simgui_desc_t desc = {};
    desc.max_vertices;// default: 65536
    desc.color_format;
    desc.depth_format;
    desc.sample_count;
    desc.ini_filename;
    desc.no_default_font = true;
    desc.disable_paste_override;
    desc.disable_set_mouse_cursor;
    desc.disable_windows_resize_from_edges;
    desc.write_alpha_channel;
    desc.allocator;
    desc.logger = { SgLogFunc, nullptr };

    ImGui_ImplSDL3_InitForOther(gfx.window);
    simgui_setup(&desc);
}
void CashImguiDestroy()
{
    ImGui_ImplSDL3_Shutdown();
    simgui_shutdown();
}

void CashImguiNewFrame(double delta_time)
{
    ZoneScoped;
    {
        ZoneScopedN("ImGui SDL Renderer3 New Frame");
        simgui_frame_desc_t desc = {};
        desc.width = gfx.window_size.x;
        desc.height = gfx.window_size.y;
        desc.delta_time = delta_time;
        desc.dpi_scale = 1.0f; //unsure
        simgui_new_frame(&desc);
    }
    {
        ZoneScopedN("ImGui SDL3 New Frame");
        //ImGui_ImplSDL3_NewFrame();
    }
    {
        ZoneScopedN("ImGui New Frame");
        //ImGui::NewFrame();
    }
}

void CashRender()
{
    ZoneScoped;
    sg_pass pass = {};
    pass.action = s_gfx.pass_action;
    SysGetRenderSwapchain(&pass.swapchain);
    sg_begin_pass(&pass);
    {
        ZoneScopedN("ImGui Render");
        simgui_render();
        //ImGui::Render();
    }

#ifdef CASH_SOKOL_RENDER
    {
        ZoneScopedN("Sokol End Pass");
        sg_end_pass();
        sg_commit();
    }
    //sg_apply_pipeline(pip);
    //sg_apply_bindings(&bindings);
    //sg_apply_uniforms(...);
    //sg_draw(...);
    SysRenderPresent();
#elif CASH_SDL_RENDER
    static const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    SDL_SetRenderScale(gfx.context, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
    SDL_SetRenderDrawColorFloat(gfx.context, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    SDL_RenderClear(gfx.context);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), gfx.context);
    SDL_RenderPresent(gfx.context);
#endif

}

void CashRenderDestroy()
{
    SDL_DestroyRenderer(gfx.context);
}




//========================
//        Utility
//========================

#define _ASGFX_DEFINITION(_Public, _Private) inline [[nodiscard]] _Private* AsGfx(_Public* c) { return reinterpret_cast<_Private*>(c); }
#define _ASGFX_DEFINITION_COMMON(_name) _ASGFX_DEFINITION(_name, Gfx ## _name)

_ASGFX_DEFINITION_COMMON(Texture);
_ASGFX_DEFINITION_COMMON(GpuBuffer);
_ASGFX_DEFINITION_COMMON(GpuBinding);
_ASGFX_DEFINITION_COMMON(Sampler);

template <typename Public, typename Private>
Private* GfxGenericCreate(Public** object, const char* name)
{
    VALIDATE_V(object, nullptr);
    if (*object != nullptr)
    {
        FAIL;
        Private* ob = AsGfx(*object);
        //Private* ob = reinterpret_cast<Private*>(object);
        DebugPrint("Error: Render object not null during create: '%s'", ob->name.c_str());
        return nullptr;
    }

    Private* ob = new Private;
    (*object) = reinterpret_cast<Public*>(ob);
    ob->name = name;
    return ob;
}




//========================
//        Texture
//========================


struct GfxTexture : public Texture {
    sg_image image = {};
    sg_image_desc desc = {};
};

sg_image_type ToSokol(TextureDimension d)
{
    switch (d)
    {
        case TextureDimension_2D:       return SG_IMAGETYPE_2D;
        case TextureDimension_3D:       return SG_IMAGETYPE_3D;
        case TextureDimension_CUBE:     return SG_IMAGETYPE_CUBE;
        case TextureDimension_ARRAY:    return SG_IMAGETYPE_ARRAY;
        case TextureDimension_Invalid:  [[fallthrough]];
        case TextureDimension_Count:    [[fallthrough]];
        default: FAIL;                  return _SG_IMAGETYPE_DEFAULT;
    }
}

sg_pixel_format ToSokol(const TextureFormat f)
{
    switch (f)
    {
        case TextureFormat_UNKNOWN:             return SG_PIXELFORMAT_NONE;
        case TextureFormat_RGBA32_FLOAT:        return SG_PIXELFORMAT_RGBA32F;
        case TextureFormat_RGBA32_UINT:         return SG_PIXELFORMAT_RGBA32UI;
        case TextureFormat_RG32_FLOAT:          return SG_PIXELFORMAT_RG32F;
        case TextureFormat_RGBA16_UINT:         return SG_PIXELFORMAT_RGBA16UI;
        case TextureFormat_RG11B10_FLOAT:       return SG_PIXELFORMAT_RG11B10F;
        case TextureFormat_R32_FLOAT:           return SG_PIXELFORMAT_R32F;
        case TextureFormat_RGBA8_UNORM:         return SG_PIXELFORMAT_RGBA8;
        case TextureFormat_RGBA8_UNORM_SRGB:    return SG_PIXELFORMAT_SRGB8A8;
        case TextureFormat_RGBA8_UINT:          return SG_PIXELFORMAT_RGBA8UI;
        case TextureFormat_RG8_UINT:            return SG_PIXELFORMAT_RG8UI;
        case TextureFormat_R8_UNORM:            return SG_PIXELFORMAT_R8;
        case TextureFormat_R8_UINT:             return SG_PIXELFORMAT_R8UI;
        case TextureFormat_Depth:               return SG_PIXELFORMAT_DEPTH;
        case TextureFormat_DepthStencil:        return SG_PIXELFORMAT_DEPTH_STENCIL;
        case TextureFormat_Count: [[fallthrough]];
        default: FAIL;                  return _SG_PIXELFORMAT_DEFAULT;
    }
}

bool CreateTextureAndUpload(Texture** texture, const char* name, const TextureParams& tp, ArrayView<u8> data)
{
    ArrayView<u8> arr[CASH_GFX_MAX_MIPS] = {};
    arr[0] = data;
    return CreateTextureAndUpload(texture, name, tp, arr);
}
bool CreateTextureAndUpload(Texture** texture, const char* name, const TextureParams& tp, ArrayView<u8> data[CASH_GFX_MAX_MIPS])
{
    ZoneScoped;
    GfxTexture* tex = GfxGenericCreate<Texture, GfxTexture>(texture, name);
    VALIDATE_V(tex, false);
    tex->parameters = tp;
    TextureParams& p = tex->parameters;
    tex->mip_levels;
    //tex->image = sg_alloc_image();

    tex->desc.type = ToSokol(p.dimension);
    tex->desc.usage.color_attachment         = FlagIntersects(p.flags, TextureFlag_RenderTarget) && !FlagIntersects(p.flags, TextureFlag_DepthStencil);
    tex->desc.usage.depth_stencil_attachment = FlagIntersects(p.flags, TextureFlag_RenderTarget) &&  FlagIntersects(p.flags, TextureFlag_DepthStencil);
    tex->desc.usage.immutable       = FlagIntersects(p.flags, TextureFlag_Immutable);
    tex->desc.usage.dynamic_update  = FlagIntersects(p.flags, TextureFlag_Dynamic);
    tex->desc.usage.stream_update   = FlagIntersects(p.flags, TextureFlag_StreamUpdate);
    //unused:
    tex->desc.usage.storage_image       = false; //only needed for compute shaders?
    tex->desc.usage.resolve_attachment  = false; //enabl if this is the target of a render who will be resolving an MSAA texture/buffer
    tex->desc.usage.write_unsealed      = false; //Unused right now

    tex->desc.width = p.size.x;
    tex->desc.height = p.size.y;
    if (p.dimension == TextureDimension_2D)
    {
        ASSERT(p.size.z == 1 || p.size.z == 0);
        p.size.z = 1;
    }
    tex->desc.num_slices = p.size.z;
    tex->desc.num_mipmaps = 1;
    tex->desc.pixel_format = ToSokol(p.format);
    tex->desc.sample_count = p.msaa_samples;

    const i32 max_mips = Min<i32>(SG_MAX_MIPMAPS, p.mip_count);
    for (i32 i = 0; i < max_mips; i++)
    {
        tex->desc.data.mip_levels[i].ptr = data[i].data;
        tex->desc.data.mip_levels[i].size = data[i].Bytes();
    }
    tex->desc.label = tex->name.c_str();

    tex->image = sg_make_image(&tex->desc);
    return true;
}

void DeleteTexture(Texture** texture)
{
    ZoneScoped;
    VALIDATE(texture);
    GfxTexture* tex = AsGfx(*texture);
    DEBUG_LOG("GPU Buffer deleted '%s': %i\n", tex->name.c_str(), tex->image);
    sg_destroy_image(tex->image);
    delete tex;
}





//========================
//       Sampler
//========================

struct GfxSampler : Sampler {
    sg_sampler sampler;
    sg_sampler_desc desc = {};
};


sg_wrap ToSokol(const SamplerWrap a)
{
    switch (a)
    {
        case SamplerWrap_Repeat:            return SG_WRAP_REPEAT;
        case SamplerWrap_ClampToEdge:       return SG_WRAP_CLAMP_TO_EDGE;
        case SamplerWrap_ClampToBorder:     return SG_WRAP_CLAMP_TO_BORDER;
        case SamplerWrap_Mirrored_Repeat:   return SG_WRAP_MIRRORED_REPEAT;
        case SamplerWrap_Count: [[fallthrough]];
        default: FAIL;                      return _SG_WRAP_DEFAULT;
    }
}

sg_filter ToSokol(const SamplerFilter a)
{
    switch (a)
    {
        case SamplerFilter_Nearest: return SG_FILTER_NEAREST;
        case SamplerFilter_Linear:  return SG_FILTER_LINEAR;
        case SamplerFilter_Count: [[fallthrough]];
        default: FAIL;              return _SG_FILTER_DEFAULT;
    }
}

sg_border_color ToSokol(const SamplerBorderColor a)
{
    switch (a)
    {
        case SamplerBorderColor_TransparentBlack:   return SG_BORDERCOLOR_TRANSPARENT_BLACK;
        case SamplerBorderColor_OpaqueBlack:        return SG_BORDERCOLOR_OPAQUE_BLACK;
        case SamplerBorderColor_OpaqueWhite:        return SG_BORDERCOLOR_OPAQUE_WHITE;
        case SamplerBorderColor_Count: [[fallthrough]];
        default: FAIL;                              return _SG_BORDERCOLOR_DEFAULT;
    }
}

sg_compare_func ToSokol(const SamplerCompareFunc a)
{
    switch (a)
    {
        case SamplerCompareFunc_Never:          return SG_COMPAREFUNC_NEVER;
        case SamplerCompareFunc_Less:           return SG_COMPAREFUNC_LESS;
        case SamplerCompareFunc_Equal:          return SG_COMPAREFUNC_EQUAL;
        case SamplerCompareFunc_Less_equal:     return SG_COMPAREFUNC_LESS_EQUAL;
        case SamplerCompareFunc_Greater:        return SG_COMPAREFUNC_GREATER;
        case SamplerCompareFunc_Not_equal:      return SG_COMPAREFUNC_NOT_EQUAL;
        case SamplerCompareFunc_Greater_equal:  return SG_COMPAREFUNC_GREATER_EQUAL;
        case SamplerCompareFunc_Always:         return SG_COMPAREFUNC_ALWAYS;
        case SamplerCompareFunc_Count: [[fallthrough]];
        default: FAIL;                          return _SG_COMPAREFUNC_DEFAULT;
    }
}


bool CreateSampler(Sampler** sampler, const char* name, const SamplerParams& params)
{
    ZoneScoped;
    GfxSampler* sam = GfxGenericCreate<Sampler, GfxSampler>(sampler, name);
    VALIDATE_V(sam, false);
    sam->params = params;
    SamplerParams& p = sam->params;

    sam->desc.min_filter    = ToSokol(p.min_filter);
    sam->desc.mag_filter    = ToSokol(p.mag_filter);
    sam->desc.mipmap_filter = ToSokol(p.mipmap_filter);
    sam->desc.wrap_u        = ToSokol(p.wrap_u);
    sam->desc.wrap_v        = ToSokol(p.wrap_v);
    sam->desc.wrap_w        = ToSokol(p.wrap_w);
    sam->desc.min_lod       = p.min_lod;
    sam->desc.max_lod       = p.max_lod;
    sam->desc.border_color  = ToSokol(p.border_color);
    sam->desc.compare       = ToSokol(p.compare_func);
    sam->desc.max_anisotropy = p.max_anisotropy;
    sam->desc.label = sam->name.c_str();

    sam->sampler = sg_make_sampler(&sam->desc);
    return true;
}
void DeleteSampler(Sampler** sampler)
{
    ZoneScoped;
    VALIDATE(sampler);
    GfxSampler* sam = reinterpret_cast<GfxSampler*>(*sampler);
    DEBUG_LOG("GPU sampler deleted '%s': %i\n", sam->name.c_str(), sam->sampler);
    sg_destroy_sampler(sam->sampler);
    delete sam;
}





//========================
//       GpuBuffer
//========================

struct GfxGpuBuffer : public GpuBuffer
{
    sg_buffer buffer;
};

bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, GpuBufferType type, GpuBufferFlag flags)
{
    ZoneScoped;
    GfxGpuBuffer* buf = GfxGenericCreate<GpuBuffer, GfxGpuBuffer>(buffer, name);
    VALIDATE_V(buf, false);
    buf->type = type;
    buf->flags = flags;
    buf->buffer = sg_alloc_buffer();
    return true;
}

void DeleteBuffer(GpuBuffer** buffer)
{
    ZoneScoped;
    VALIDATE(buffer);
    GfxGpuBuffer* buf = reinterpret_cast<GfxGpuBuffer*>(*buffer);
    DEBUG_LOG("GPU Buffer deleted '%s': %i\n", buf->name.c_str(), buf->buffer);
    sg_destroy_buffer(buf->buffer);
    delete buf;
}

void GpuBuffer::Upload(const void* data, const size_t in_count, const u32 in_element_size, const bool is_byte_format)
{
    ZoneScoped;
    GfxGpuBuffer* buf = reinterpret_cast<GfxGpuBuffer*>(this);
    VALIDATE(buf->buffer.id);
    VALIDATE(in_count && in_element_size);
    count = in_count;
    element_size = in_element_size;
    const u64 bytes = in_count * in_element_size;

    if (!buf->has_uploaded)
    {
        sg_buffer_desc desc = {};
        //desc.size = bytes; //Not sure about this
        desc.data.ptr = data;
        desc.data.size = bytes;
        desc.usage.vertex_buffer = buf->type == GpuBufferType_Vertex;
        desc.usage.index_buffer = buf->type == GpuBufferType_Index;
        desc.usage.storage_buffer = buf->type == GpuBufferType_Structure;
        desc.usage.immutable = FlagIntersects(buf->flags, GpuBufferFlag_Immutable);
        desc.usage.dynamic_update = FlagIntersects(buf->flags, GpuBufferFlag_Dynamic);
        desc.usage.stream_update = FlagIntersects(buf->flags, GpuBufferFlag_StreamUpdate);
        desc.usage.write_unsealed = false;//FlagIntersects(buf->flags, GpuBufferFlag_WriteUnsealed);
        desc.label = buf->name.c_str();

        sg_init_buffer(buf->buffer, desc);
        buf->has_uploaded = true;
    }
    else
    {
        VALIDATE(FlagIntersects(buf->flags, GpuBufferFlag_Dynamic) || FlagIntersects(buf->flags, GpuBufferFlag_StreamUpdate));

        sg_range update_data;
        update_data.ptr = data;
        update_data.size = bytes;

        //Appends data to a stream buffer
        //sg_append_buffer(buf->buffer, &update_data);
        sg_update_buffer(buf->buffer, &update_data);
    }
}





//========================
//       GpuBinding
//========================

struct GfxGpuBinding : GpuBinding
{
    sg_bindings binding;
};

bool CreateGpuBinding(GpuBinding** binding, const char* name)
{
    ZoneScoped;
    GfxGpuBinding* bind = GfxGenericCreate<GpuBinding, GfxGpuBinding>(binding, name);
    VALIDATE_V(bind, false);
    (*binding) = reinterpret_cast<GpuBinding*>(bind);
    return true;
}

void DeleteBuffer(GpuBinding** binding)
{
    ZoneScoped;
    VALIDATE(binding);
    GfxGpuBinding* bin = reinterpret_cast<GfxGpuBinding*>(*binding);
    DEBUG_LOG("GPU binding deleted '%s'\n", bin->name.c_str());
    delete bin;
}

void GpuBinding::BindVertex(const GpuBuffer* buffer, const i32 slot)
{
    ZoneScoped;
    VALIDATE(buffer && buffer->type == GpuBufferType_Vertex);
    const GfxGpuBuffer* buf = reinterpret_cast<const GfxGpuBuffer*>(buffer);
    GfxGpuBinding* bin = reinterpret_cast<GfxGpuBinding*>(this);
    
    sg_buffer& b = bin->binding.vertex_buffers[slot];
    if (b.id != 0)
        DebugPrint("Warning: Overwriting binding(%s) slot(%i) for vertex buffer (%s)", bin->name.c_str(), slot, buf->name.c_str());
    b = buf->buffer;
}
void GpuBinding::BindIndex(const GpuBuffer* buffer)
{
    ZoneScoped;
    VALIDATE(buffer && buffer->type == GpuBufferType_Index);
    const GfxGpuBuffer* buf = reinterpret_cast<const GfxGpuBuffer*>(buffer);
    GfxGpuBinding* bin = reinterpret_cast<GfxGpuBinding*>(this);
    sg_buffer& b = bin->binding.index_buffer;
    if (b.id != 0)
        DebugPrint("Warning: Overwriting binding (%s) for index buffer (%s)", bin->name.c_str(), buf->name.c_str());
    b = buf->buffer;
}
void GpuBinding::BindView(GpuBuffer* view)
{
    ZoneScoped;
    FAIL;
}
void GpuBinding::BindSampler(GpuBuffer* sampler)
{
    ZoneScoped;
    FAIL;
}

void GpuBinding::Apply()
{
    ZoneScoped;
    GfxGpuBinding* bin = reinterpret_cast<GfxGpuBinding*>(this);
    sg_apply_bindings(bin->binding);
}


