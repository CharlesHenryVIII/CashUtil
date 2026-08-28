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
struct VertexData {
    sg_vertex_attr_state state[SG_MAX_VERTEX_ATTRIBUTES] = {};
    sg_shader_vertex_attr attr[SG_MAX_VERTEX_ATTRIBUTES] = {};
    i32 count = 0;
};
//using SokolVertexLayout = std::array<VertexData, SG_MAX_VERTEX_ATTRIBUTES>;

struct GfxDevice {
    VertexData vertex_layouts[VertexType_Count]= {};
    GpuBuffer* fullscreen_verts = {};
    sg_pass_action pass_action = {};
};
static GfxDevice s_gfx;

struct GfxTexture;
struct GfxSampler;
struct GfxGpuBuffer;
struct GfxGpuBinding;
struct GfxShader;

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

#pragma("pack(push, 1)")
struct ShaderConstants_2D {
    Mat4 orthographic;
};
#pragma("pack(pop)")


static const char* vertex_shader_text_2d = R"TERM(
cbuffer ShaderConstants_2D : register (b0) {
    float4x4 orthographic;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}
)TERM";

static const char* pixel_shader_text_2d = R"TERM(
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};
struct PS_OUTPUT
{
    float4 col : SV_Target;
}

sampler     sampler : register(s0);
Texture2D   texture : register(t0)

PS_OUTPUT main(PS_INPUT input) : SV_Target
{
    PS_OUTPUT output;
    output.col = input.col * texture.Sample(sampler0, input.uv);
    return output;
}
)TERM";

//semantic name: https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics
//semantic index: incrementing amount of that specific semantic name.  IE:
// POSITION = 0
// COLOR = 0,
// POSITION = 1,
// COLOR = 0,
// POSITION = 2, etc
enum VertexSemantic : u32 {
    VertexSemantic_Binormal,
    VertexSemantic_BlendIndices,
    VertexSemantic_BlendWeight,
    VertexSemantic_Color,       //Diffuse and specular color
    VertexSemantic_Normal,      //Normal Vector
    VertexSemantic_Position,    //Vertex Position
    VertexSemantic_PositionT,   //Transformed Vertex Position
    VertexSemantic_PSize,       //Point size
    VertexSemantic_Tangent,
    VertexSemantic_TexCoord,
    VertexSemantic_Count,
};

static const char* s_vertex_semantic_strings[VertexSemantic_Count] = {
    "BINORMAL",
    "BLENDINDICES",
    "BLENDWEIGHT",
    "COLOR, ",
    "NORMAL",
    "POSITION",
    "POSITIONT",
    "PSIZE,  ",
    "TANGENT",
    "TEXCOORD",
};

void SetVertexData(VertexData& d, i32 vertex_index, i32 buffer_index, i32 offset, sg_vertex_format format, VertexSemantic semantic, i32 semantic_index)
{
    if (d.count >= SG_MAX_VERTEX_ATTRIBUTES)
    {
        FAIL;
        DebugPrint("Trying to add more vertex attributes than supported: %i of %i", d.count, SG_MAX_VERTEX_ATTRIBUTES);
        return;
    }
    ASSERT(d.count < vertex_index); // did you mean to overwrite the vertex data?
    ASSERT(d.count == vertex_index); // did you mean to skip over some vertex data?
    ++d.count;

    sg_vertex_attr_state& s = d.state[vertex_index];
    sg_shader_vertex_attr& a = d.attr[vertex_index];
    s.buffer_index = buffer_index;
    s.offset = offset;
    s.format = format;
    
    switch (format)
    {
        case SG_VERTEXFORMAT_HALF2:FAIL;[[fallthrough]]//is this correct?
        case SG_VERTEXFORMAT_HALF4:FAIL;a.base_type = SG_SHADERATTRBASETYPE_UNDEFINED; break;//is this correct?

        case SG_VERTEXFORMAT_FLOAT:     [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT2:    [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT3:    [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT4:    a.base_type =  SG_SHADERATTRBASETYPE_FLOAT;

        case SG_VERTEXFORMAT_INT:       [[fallthrough]];
        case SG_VERTEXFORMAT_INT2:      [[fallthrough]];
        case SG_VERTEXFORMAT_INT3:      [[fallthrough]];
        case SG_VERTEXFORMAT_INT4:      [[fallthrough]];
        case SG_VERTEXFORMAT_BYTE4:     [[fallthrough]];
        case SG_VERTEXFORMAT_BYTE4N:    [[fallthrough]];
        case SG_VERTEXFORMAT_SHORT2:    [[fallthrough]];
        case SG_VERTEXFORMAT_SHORT2N:   [[fallthrough]];
        case SG_VERTEXFORMAT_SHORT4:    [[fallthrough]];
        case SG_VERTEXFORMAT_SHORT4N:   [[fallthrough]];
        case SG_VERTEXFORMAT_INT10_N2:  a.base_type = SG_SHADERATTRBASETYPE_SINT; break;

        case SG_VERTEXFORMAT_UINT:      [[fallthrough]];
        case SG_VERTEXFORMAT_UINT2:     [[fallthrough]];
        case SG_VERTEXFORMAT_UINT3:     [[fallthrough]];
        case SG_VERTEXFORMAT_UINT4:     [[fallthrough]];
        case SG_VERTEXFORMAT_UBYTE4:    [[fallthrough]];
        case SG_VERTEXFORMAT_UBYTE4N:   [[fallthrough]];
        case SG_VERTEXFORMAT_USHORT2:   [[fallthrough]];
        case SG_VERTEXFORMAT_USHORT2N:  [[fallthrough]];
        case SG_VERTEXFORMAT_USHORT4:   [[fallthrough]];
        case SG_VERTEXFORMAT_USHORT4N:  [[fallthrough]];
        case SG_VERTEXFORMAT_UINT10_N2: a.base_type = SG_SHADERATTRBASETYPE_UINT; break;

        default: FAIL;                  a.base_type = SG_SHADERATTRBASETYPE_UNDEFINED; break;
    }

    //d.attr.glsl_name
    a.hlsl_sem_name = s_vertex_semantic_strings[semantic];
    a.hlsl_sem_index = semantic_index;
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
    SetVertexData(s_gfx.vertex_layouts[VertexType_2D],  0,  0, offsetof(Vertex_2D, position),   SG_VERTEXFORMAT_FLOAT2, VertexSemantic_Position,0);
    SetVertexData(s_gfx.vertex_layouts[VertexType_2D],  1,  0, offsetof(Vertex_2D, color),      SG_VERTEXFORMAT_FLOAT4, VertexSemantic_Color,   0);
    SetVertexData(s_gfx.vertex_layouts[VertexType_2D],  2,  0, offsetof(Vertex_2D, uv),         SG_VERTEXFORMAT_FLOAT2, VertexSemantic_TexCoord,0);
    //Vertex_PNTC
    SetVertexData(s_gfx.vertex_layouts[VertexType_PNTC], 0, 0, offsetof(Vertex_PNTC, position), SG_VERTEXFORMAT_FLOAT3, VertexSemantic_Position,0);
    SetVertexData(s_gfx.vertex_layouts[VertexType_PNTC], 1, 0, offsetof(Vertex_PNTC, normal),   SG_VERTEXFORMAT_FLOAT3, VertexSemantic_Normal,  0);
    SetVertexData(s_gfx.vertex_layouts[VertexType_PNTC], 2, 0, offsetof(Vertex_PNTC, uv),       SG_VERTEXFORMAT_FLOAT2, VertexSemantic_TexCoord,0);
    SetVertexData(s_gfx.vertex_layouts[VertexType_PNTC], 3, 0, offsetof(Vertex_PNTC, color),    SG_VERTEXFORMAT_FLOAT4, VertexSemantic_Color,   0);
    //

    {
        Vertex_2D verts[] = {
            { { -1.0f, +1.0f }, White, { 0.0f, 1.0f }, },  // Top Left
            { { -1.0f, -3.0f }, White, { 0.0f,-3.0f }, },  // Bot Left
            { { +3.0f, +1.0f }, White, { 3.0f, 1.0f}, }   // Top Right
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
_ASGFX_DEFINITION_COMMON(Shader);

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
    sg_image_desc image_desc = {};
    sg_view view = {};
    sg_view_desc view_desc = {};
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

    //=====
    //Image
    //=====

    tex->image_desc.type = ToSokol(p.dimension);
    tex->image_desc.usage.color_attachment         = FlagIntersects(p.flags, TextureFlag_RenderTarget) && !FlagIntersects(p.flags, TextureFlag_DepthStencil);
    tex->image_desc.usage.depth_stencil_attachment = FlagIntersects(p.flags, TextureFlag_RenderTarget) &&  FlagIntersects(p.flags, TextureFlag_DepthStencil);
    tex->image_desc.usage.immutable       = FlagIntersects(p.flags, TextureFlag_Immutable);
    tex->image_desc.usage.dynamic_update  = FlagIntersects(p.flags, TextureFlag_Dynamic);
    tex->image_desc.usage.stream_update   = FlagIntersects(p.flags, TextureFlag_StreamUpdate);
    //unused:
    tex->image_desc.usage.storage_image       = false; //only needed for compute shaders?
    tex->image_desc.usage.resolve_attachment  = false; //enabl if this is the target of a render who will be resolving an MSAA texture/buffer
    tex->image_desc.usage.write_unsealed      = false; //Unused right now

    tex->image_desc.width = p.size.x;
    tex->image_desc.height = p.size.y;
    if (p.dimension == TextureDimension_2D)
    {
        ASSERT(p.size.z == 1 || p.size.z == 0);
        p.size.z = 1;
    }
    tex->image_desc.num_slices = p.size.z;
    tex->image_desc.num_mipmaps = 1;
    tex->image_desc.pixel_format = ToSokol(p.format);
    tex->image_desc.sample_count = p.msaa_samples;

    const i32 max_mips = Min<i32>(SG_MAX_MIPMAPS, p.mip_count);
    for (i32 i = 0; i < max_mips; i++)
    {
        tex->image_desc.data.mip_levels[i].ptr = data[i].data;
        tex->image_desc.data.mip_levels[i].size = data[i].Bytes();
    }
    tex->image_desc.label = tex->name.c_str();
    tex->image = sg_make_image(&tex->image_desc);


    //=====
    // View
    //=====

    tex->view_desc.texture.image = tex->image;
    tex->view_desc.texture.mip_levels = { 0, max_mips }; //Starting mip level are hard coded
    tex->view_desc.texture.slices = { 0, tex->image_desc.num_slices }; //Starting slice is hard coded

    tex->view_desc.storage_buffer; //Unorderd Access View equivilent for Computer Shaders I think
    tex->view_desc.storage_image;  //Unorderd Access View equivilent for Computer Shaders I think
    tex->view_desc.resolve_attachment; //For resolving MSAA

    //NOTE(CSH): The mip level and slice for color_attachment and depth_stencil_attachment are hard coded
    if (FlagIntersects(p.flags, TextureFlag_RenderTarget) && !FlagIntersects(p.flags, TextureFlag_DepthStencil))
    {
        tex->view_desc.color_attachment.image = tex->image;
        tex->view_desc.color_attachment.mip_level = 0;
        tex->view_desc.color_attachment.slice = 0;
    }
    if (FlagIntersects(p.flags, TextureFlag_RenderTarget) && FlagIntersects(p.flags, TextureFlag_DepthStencil))
    {
        tex->view_desc.depth_stencil_attachment.image = tex->image;
        tex->view_desc.depth_stencil_attachment.mip_level = 0;
        tex->view_desc.depth_stencil_attachment.slice = 0;
    }
    tex->view_desc.label = tex->name.c_str();
    tex->view = sg_make_view(tex->view_desc);
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
    sg_sampler_desc sampler_desc = {};
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

    sam->sampler_desc.min_filter    = ToSokol(p.min_filter);
    sam->sampler_desc.mag_filter    = ToSokol(p.mag_filter);
    sam->sampler_desc.mipmap_filter = ToSokol(p.mipmap_filter);
    sam->sampler_desc.wrap_u        = ToSokol(p.wrap_u);
    sam->sampler_desc.wrap_v        = ToSokol(p.wrap_v);
    sam->sampler_desc.wrap_w        = ToSokol(p.wrap_w);
    sam->sampler_desc.min_lod       = p.min_lod;
    sam->sampler_desc.max_lod       = p.max_lod;
    sam->sampler_desc.border_color  = ToSokol(p.border_color);
    sam->sampler_desc.compare       = ToSokol(p.compare_func);
    sam->sampler_desc.max_anisotropy = p.max_anisotropy;
    sam->sampler_desc.label = sam->name.c_str();

    sam->sampler = sg_make_sampler(&sam->sampler_desc);
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






//========================
//        Shader
//========================

struct GfxShader : Shader {
    sg_shader shader;
    sg_shader_desc shader_desc;
};

static_assert(MAX_SHADER_TEXTURES <= SG_MAX_VIEW_BINDSLOTS);
static_assert(MAX_SHADER_TEXTURES <= SG_MAX_SAMPLER_BINDSLOTS);
static_assert(MAX_SHADER_TEXTURES <= SG_MAX_TEXTURE_SAMPLER_PAIRS);

#define D3D11_SHADER_MODEL         _4_0
#define D3D11_SHADER_MODEL_VERTEX  CSH_CONCAT(vs, D3D11_SHADER_MODEL)
#define D3D11_SHADER_MODEL_PIXEL   CSH_CONCAT(ps, D3D11_SHADER_MODEL)
#define D3D11_SHADER_MODEL_COMPUTE CSH_CONCAT(cs, D3D11_SHADER_MODEL)

//struct ShaderMacro {
//    std::string name;
//    std::string value;
//};

//bool CreateShader(Shader** shader,
//    const std::string& vertexFileLocation,
//    const std::string& pixelFileLocation,
//    ArrayView<Shader::InputElementDesc> input_layout,
//    std::vector<ShaderMacro> macros = std::vector<ShaderMacro>())

sg_shader_function ToSokol(const ShaderFile& s, const char* entry, const char* target)
{
    sg_shader_function sf = {
        .source = s.text,
        .entry = entry,
        .d3d11_target = target,
        .d3d11_filepath = ToString(s.filepath).c_str(),
    };
    return sf;
}

//should we be trying ps_5_0/vs_5_0 instead?
sg_uniform_type ToSokol(const ShaderConstantType a)
{
    switch (a)
    {
        case ShaderConstant_Float:  return SG_UNIFORMTYPE_FLOAT;
        case ShaderConstant_Float2: return SG_UNIFORMTYPE_FLOAT;
        case ShaderConstant_Float3: return SG_UNIFORMTYPE_FLOAT;
        case ShaderConstant_Float4: return SG_UNIFORMTYPE_FLOAT;
        case ShaderConstant_Int:    return SG_UNIFORMTYPE_INT;
        case ShaderConstant_Int2:   return SG_UNIFORMTYPE_INT2;
        case ShaderConstant_Int3:   return SG_UNIFORMTYPE_INT3;
        case ShaderConstant_Int4:   return SG_UNIFORMTYPE_INT4;
        case ShaderConstant_Mat4:   return SG_UNIFORMTYPE_MAT4;
        case ShaderConstant_Invalid:[[fallthrough]];
        case ShaderConstant_Count:  [[fallthrough]];
        default: FAIL; return SG_UNIFORMTYPE_INVALID;
    }
}

sg_image_sample_type GetSokolSampleType(const TextureFormat f)
{
    switch (f)
    {
        case TextureFormat_RGBA32_FLOAT:        [[fallthrough]];
        case TextureFormat_RG32_FLOAT:          [[fallthrough]];
        case TextureFormat_R32_FLOAT:           return SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT;

        case TextureFormat_RGBA32_UINT:         [[fallthrough]];
        case TextureFormat_RGBA16_UINT:         [[fallthrough]];
        case TextureFormat_RGBA8_UNORM:         [[fallthrough]];
        case TextureFormat_RGBA8_UNORM_SRGB:    [[fallthrough]];
        case TextureFormat_RGBA8_UINT:          [[fallthrough]];
        case TextureFormat_RG8_UINT:            [[fallthrough]];
        case TextureFormat_R8_UNORM:            [[fallthrough]];
        case TextureFormat_R8_UINT:             return SG_IMAGESAMPLETYPE_UINT;

        case TextureFormat_RG11B10_FLOAT:       return SG_IMAGESAMPLETYPE_FLOAT;

        case TextureFormat_Depth:               [[fallthrough]];
        case TextureFormat_DepthStencil:        return SG_IMAGESAMPLETYPE_DEPTH;

        //case TextureFormat_RGBA32SINT:        return SG_IMAGESAMPLETYPE_SINT;

        case TextureFormat_UNKNOWN:             [[fallthrough]];
        case TextureFormat_Count:               [[fallthrough]];
        default: FAIL;                  return _SG_IMAGESAMPLETYPE_DEFAULT;
    }
}

sg_sampler_type  GetSokolSamplerType(const sg_image_sample_type a, const bool is_linear_filter, const SamplerCompareFunc compare_func)
{
    switch (a)
    {
    case SG_IMAGESAMPLETYPE_FLOAT:
    {
        ASSERT(compare_func == SamplerCompareFunc_Never);
        if (is_linear_filter)
            return SG_SAMPLERTYPE_NONFILTERING;
        else
            return SG_SAMPLERTYPE_FILTERING;
    }
    case SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT:
    {
        ASSERT(compare_func == SamplerCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_SINT:
    {
        ASSERT(compare_func == SamplerCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_UINT:
    {
        ASSERT(compare_func == SamplerCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_DEPTH:
    {
        ASSERT(compare_func != SamplerCompareFunc_Never);
        return SG_SAMPLERTYPE_COMPARISON;
    }
    default: FAIL; return _SG_SAMPLERTYPE_DEFAULT;
    }
}

#define STB_INCLUDE_IMPLEMENTATION
#define STB_INCLUDE_LINE_NONE
#include "stb/stb_include.h"
bool CreateShader(Shader** shader, const char* name, const Path& vertex_filepath, const Path& pixel_filepath)//, const Path& compute_filepath)
{
#error load ourselves!
    char *stb_include_file(char *filename, char *inject, char *path_to_includes, char error[256]);
}

bool CreateShader(Shader** shader, const char* name, const ShaderParams& params)
{
    // VALIDATE_V(params.vertex.text || params.pixel.text || params.compute.text, false);
    VALIDATE_V(params.vertex.text || params.pixel.text, false);

    ZoneScoped;
    GfxShader* s = GfxGenericCreate<Shader, GfxShader>(shader, name);
    VALIDATE_V(s, false);
    (*shader) = reinterpret_cast<Shader*>(s);

    sg_shader_desc& desc = s->shader_desc;

    //Vertex Pixel Computer files/text
    if (params.vertex.text)
        desc.vertex_func = ToSokol(params.vertex, "Vertex_Main", CSH_TOSTRING(D3D11_SHADER_MODEL_VERTEX));
    if (params.pixel.text)
        desc.fragment_func = ToSokol(params.pixel, "Pixel_Main", CSH_TOSTRING(D3D11_SHADER_MODEL_PIXEL));
    //if (params.compute.text)
    //    desc.compute_func = ToSokol(params.compute, "Main", CSH_TOSTRING(D3D11_SHADER_MODEL_COMPUTE));
    
    const VertexData& vd = s_gfx.vertex_layouts[params.vertex_type];
    for (i32 i = 0; i < vd.count; i++)
    {
        desc.attrs[i] = vd.attr[i];
    }

    //Shader Constants
    const ShaderConstantsContainer& cs = params.constants_container;
    const u8 slot = cs.slot;
    desc.uniform_blocks[0].size = cs.size;
    desc.uniform_blocks[0].hlsl_register_b_n =
        desc.uniform_blocks[0].msl_buffer_n =
        desc.uniform_blocks[0].wgsl_group0_binding_n =
        desc.uniform_blocks[0].spirv_set0_binding_n = slot;
    desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
    for (i32 i = 0; i < SG_MAX_UNIFORMBLOCK_MEMBERS; i++)
    {
        if (cs.constants[i].type == ShaderConstant_Invalid)
            break;
        const ShaderConstant& c = cs.constants[i];
        desc.uniform_blocks[0].glsl_uniforms[i].type = ToSokol(c.type);
        desc.uniform_blocks[0].glsl_uniforms[i].array_count = c.array_count;
        desc.uniform_blocks[0].glsl_uniforms[i].glsl_name = c.name;
    }

    //NOTE(CSH): We are halving the amount of textures we can bind for ease of programming
    // So every shader will have the textures bound to BOTH vertex AND pixel shaders
    //TODO(CSH): seperate these and create an easy api for it OR try Sokol-SHDC

    //Shader Textures
    for (i32 i = 0; i < MAX_SHADER_TEXTURES; i++)
    {
        const ShaderTexture& t = params.textures[i];
        if (!t.texture)
            continue;
        const GfxTexture* gt = AsGfx(t.texture);
        const GfxSampler* gs = AsGfx(t.sampler);
        ASSERT(gt);
        //TEXTURE
        //  -> Vertex
        const sg_image_sample_type image_sample_type = GetSokolSampleType(gt->parameters.format);
        desc.views[i].texture.stage = SG_SHADERSTAGE_VERTEX;
        desc.views[i].texture.image_type = gt->image_desc.type;
        desc.views[i].texture.sample_type = image_sample_type;
        desc.views[i].texture.multisampled = gt->parameters.msaa_samples > 1;
        desc.views[i].texture.hlsl_register_t_n =
            desc.views[i].texture.msl_texture_n =
            desc.views[i].texture.wgsl_group1_binding_n =
            desc.views[i].texture.spirv_set1_binding_n = slot;
        desc.views[i].storage_buffer; //only used for compute
        desc.views[i].storage_image; //only used for compute

        //  -> Pixel
        const i32 pixel_view_index_offset = SG_MAX_VIEW_BINDSLOTS >> 1;
        const i32 pvi2 = i + pixel_view_index_offset;
        desc.views[pvi2] = desc.views[i];
        desc.views[pvi2].texture.stage = SG_SHADERSTAGE_FRAGMENT;


        //SAMPLER
        //  -> Vertex
        desc.samplers[i].stage = SG_SHADERSTAGE_VERTEX;
        const bool is_linear_filter = gs->params.min_filter == SamplerFilter_Linear &&
            gs->params.mag_filter == SamplerFilter_Linear &&
            gs->params.mipmap_filter == SamplerFilter_Linear;
        desc.samplers[i].sampler_type = GetSokolSamplerType(image_sample_type, is_linear_filter, gs->params.compare_func);
        desc.samplers[i].hlsl_register_s_n =
            desc.samplers[i].msl_sampler_n =
            desc.samplers[i].wgsl_group1_binding_n =
            desc.samplers[i].spirv_set1_binding_n = slot;

        //  -> Pixel
        const i32 pixel_sampler_index_offset = SG_MAX_SAMPLER_BINDSLOTS >> 1;
        const i32 psi2 = i + pixel_sampler_index_offset;
        desc.samplers[psi2] = desc.samplers[i];
        desc.samplers[psi2].stage = SG_SHADERSTAGE_FRAGMENT;


        //TEXTURE + SAMPLER PAIRS
        //  -> Vertex
        desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_VERTEX;
        desc.texture_sampler_pairs[i].view_slot = slot; // must be SG_VIEWTYPE_TEXTURE
        desc.texture_sampler_pairs[i].sampler_slot = slot;
        desc.texture_sampler_pairs[i].glsl_name = t.name;          // glsl name binding required because of GL 4.1 and WebGL2

        //  -> Pixel
        const i32 pixel_st_index_offset = SG_MAX_TEXTURE_SAMPLER_PAIRS >> 1;
        const i32 psti2 = i + pixel_st_index_offset;
        desc.texture_sampler_pairs[psti2] = desc.texture_sampler_pairs[i];
        desc.texture_sampler_pairs[psti2].stage = SG_SHADERSTAGE_FRAGMENT;
    }
    desc.mtl_threads_per_threadgroup; //Only used for compute shaders
    desc.label = s->name.c_str();

    s->shader = sg_make_shader(s->shader_desc);
    return true;
}
