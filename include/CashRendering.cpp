#include "CashRendering.h"
#include "CashDebug.h"
#include "CashSystem.h"
#include "resource.h"
#include "CashIdArray.h"

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
#define SOKOL_SHDC_IMPL
#include "CashUtil/include/Shaders/Blit2D.h"

Renderer gfx;

struct VertexData {
    VertexID data_id;
    sg_vertex_layout_state state = {};
    sg_shader_vertex_attr attr[SG_MAX_VERTEX_ATTRIBUTES] = {};
    VertexParams params[SG_MAX_VERTEX_ATTRIBUTES] = {};
    i32 count = 0;
};


static struct GfxDevice {
    StaticIdArray<VertexData, VertexID, 256> vertex_layouts;
    sg_pass_action pass_action = {};
    sg_pipeline final_render_pipeline = {};
    sg_sampler final_render_sampler = {};
} s_gfx;

struct GfxTexture;
struct GfxSampler;
struct GfxGpuBuffer;
//struct GfxGpuBinding;
struct GfxShader;
struct GfxPipeline;
//struct GfxDrawCall;
void RenderDrawCalls();

//========================
//        Utility
//========================

#define _ASGFX_DEFINITION(_Public, _Private) inline [[nodiscard]] _Private* AsGfx(_Public* c) { return reinterpret_cast<_Private*>(c); }\
inline [[nodiscard]] const _Private* AsGfx(const _Public* c) { return reinterpret_cast<const _Private*>(c); }
#define _ASGFX_DEFINITION_COMMON(_name) _ASGFX_DEFINITION(_name, Gfx ## _name)

_ASGFX_DEFINITION_COMMON(Texture);
_ASGFX_DEFINITION_COMMON(GpuBuffer);
//_ASGFX_DEFINITION_COMMON(GpuBinding);
_ASGFX_DEFINITION_COMMON(Sampler);
_ASGFX_DEFINITION_COMMON(Shader);
_ASGFX_DEFINITION_COMMON(Pipeline);
//_ASGFX_DEFINITION_COMMON(DrawCall);

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
    (*object) = AsGfx(ob);
    ob->name = name;
    return ob;
}

constexpr sg_backend ToSokol(CashRenderBackend d)
{
    switch (d)
    {
    case CashRenderBackend_Glcore:          return SG_BACKEND_GLCORE;
    case CashRenderBackend_Gles3:           return SG_BACKEND_GLES3;
    case CashRenderBackend_D3D11:           return SG_BACKEND_D3D11;
    case CashRenderBackend_Metal_Ios:       return SG_BACKEND_METAL_IOS;
    case CashRenderBackend_Metal_Macos:     return SG_BACKEND_METAL_MACOS;
    case CashRenderBackend_Metal_Simulator: return SG_BACKEND_METAL_SIMULATOR;
    case CashRenderBackend_WGpu:            return SG_BACKEND_WGPU;
    case CashRenderBackend_Vulkan:          return SG_BACKEND_VULKAN;
    case CashRenderBackend_Count:           [[fallthrough]];
    default: FAIL;                          return SG_BACKEND_DUMMY;
    }
}

sg_color ToSokol(Color color)
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



//static const char* vertex_shader_text_2d = R"TERM(
//cbuffer ShaderConstants_2D : register (b0) {
//    float4x4 orthographic;
//};
//
//struct VS_INPUT
//{
//    float2 pos : POSITION;
//    float4 col : COLOR;
//    float2 uv  : TEXCOORD;
//};
//
//struct VS_OUTPUT
//{
//    float4 pos : SV_POSITION;
//    float4 col : COLOR;
//    float2 uv  : TEXCOORD;
//};
//
//VS_OUTPUT main(VS_INPUT input)
//{
//    VS_OUTPUT output;
//    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
//    output.col = input.col;
//    output.uv  = input.uv;
//    return output;
//}
//)TERM";
//
//static const char* pixel_shader_text_2d = R"TERM(
//struct PS_INPUT
//{
//    float4 pos : SV_POSITION;
//    float4 col : COLOR;
//    float2 uv  : TEXCOORD;
//};
//struct PS_OUTPUT
//{
//    float4 col : SV_Target;
//}
//
//sampler     sampler : register(s0);
//Texture2D   texture : register(t0)
//
//PS_OUTPUT main(PS_INPUT input) : SV_Target
//{
//    PS_OUTPUT output;
//    output.col = input.col * texture.Sample(sampler0, input.uv);
//    return output;
//}
//)TERM";








//========================
//      Vertex Data
//========================

constexpr sg_vertex_format ToSokol(VertexFormat d)
{
    switch (d)
    {
        case VertexFormat_Float:    return SG_VERTEXFORMAT_FLOAT;
        case VertexFormat_Float2:   return SG_VERTEXFORMAT_FLOAT2;
        case VertexFormat_Float3:   return SG_VERTEXFORMAT_FLOAT3;
        case VertexFormat_Float4:   return SG_VERTEXFORMAT_FLOAT4;
        case VertexFormat_Int:      return SG_VERTEXFORMAT_INT;
        case VertexFormat_Int2:     return SG_VERTEXFORMAT_INT2;
        case VertexFormat_Int3:     return SG_VERTEXFORMAT_INT3;
        case VertexFormat_Int4:     return SG_VERTEXFORMAT_INT4;
        case VertexFormat_UInt:     return SG_VERTEXFORMAT_UINT;
        case VertexFormat_UInt2:    return SG_VERTEXFORMAT_UINT2;
        case VertexFormat_UInt3:    return SG_VERTEXFORMAT_UINT3;
        case VertexFormat_UInt4:    return SG_VERTEXFORMAT_UINT4;
        case VertexFormat_Byte4:    return SG_VERTEXFORMAT_BYTE4;
        case VertexFormat_Byte4N:   return SG_VERTEXFORMAT_BYTE4N;
        case VertexFormat_UByte4:   return SG_VERTEXFORMAT_UBYTE4;
        case VertexFormat_UByte4N:  return SG_VERTEXFORMAT_UBYTE4N;
        case VertexFormat_Short2:   return SG_VERTEXFORMAT_SHORT2;
        case VertexFormat_Short2N:  return SG_VERTEXFORMAT_SHORT2N;
        case VertexFormat_UShort2:  return SG_VERTEXFORMAT_USHORT2;
        case VertexFormat_UShort2N: return SG_VERTEXFORMAT_USHORT2N;
        case VertexFormat_Short4:   return SG_VERTEXFORMAT_SHORT4;
        case VertexFormat_Short4N:  return SG_VERTEXFORMAT_SHORT4N;
        case VertexFormat_UShort4:  return SG_VERTEXFORMAT_USHORT4;
        case VertexFormat_UShort4N: return SG_VERTEXFORMAT_USHORT4N;
        case VertexFormat_Int10_N2: return SG_VERTEXFORMAT_INT10_N2;
        case VertexFormat_UInt10_N2:return SG_VERTEXFORMAT_UINT10_N2;
        case VertexFormat_Half2:    return SG_VERTEXFORMAT_HALF2;
        case VertexFormat_Half4:    return SG_VERTEXFORMAT_HALF4;
        case VertexFormat_Invalid:  [[fallthrough]];
        case VertexFormat_Count:    [[fallthrough]];
        default: FAIL;              return SG_VERTEXFORMAT_INVALID;
    }
}

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

sg_vertex_step ToSokol(const VertexStep a)
{
    switch (a)
    {
        case VertexStep_Vertex: return SG_VERTEXSTEP_PER_VERTEX;
        case VertexStep_Instance: return SG_VERTEXSTEP_PER_INSTANCE;
        default: FAIL; return _SG_VERTEXSTEP_DEFAULT;
    }
}

VertexID CreateVertexLayout(ArrayView<VertexParams> vertex_layout)
{
    VALIDATE_V(vertex_layout.count < SG_MAX_VERTEX_ATTRIBUTES, VertexID());
    VALIDATE_V(vertex_layout.count, VertexID());

    VertexData* _d = s_gfx.vertex_layouts.CreateNew();
    VALIDATE_V(_d, VertexID());
    VertexData& d = *_d;

    for (i32 i = 0; i < vertex_layout.count; i++)
    {
        const VertexParams& p = vertex_layout[i];
        sg_vertex_attr_state&           s = d.state.attrs[p.vertex_index];
        sg_vertex_buffer_layout_state&  b = d.state.buffers[p.vertex_index];
        sg_shader_vertex_attr&          a = d.attr[p.vertex_index];

        ++d.count;
        d.params[i] = p;
        s.buffer_index = p.buffer_index;
        s.offset = p.offset;
        s.format = ToSokol(p.format);

        b.stride = 0;
        b.step_func = ToSokol(p.vertex_step);
        b.step_rate = p.vertex_step_rate;

        switch (s.format)
        {
        case SG_VERTEXFORMAT_HALF2:FAIL; [[fallthrough]];//is this correct?
        case SG_VERTEXFORMAT_HALF4:FAIL; a.base_type = SG_SHADERATTRBASETYPE_UNDEFINED; break;//is this correct?

        case SG_VERTEXFORMAT_FLOAT:     [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT2:    [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT3:    [[fallthrough]];
        case SG_VERTEXFORMAT_FLOAT4:    a.base_type = SG_SHADERATTRBASETYPE_FLOAT; break;

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
        a.hlsl_sem_name = s_vertex_semantic_strings[p.semantic];
        a.hlsl_sem_index = p.semantic_index;
    }

    return d.data_id;
}

bool DeleteVertexLayout(VertexID id)
{
    return s_gfx.vertex_layouts.Erase(id);
}








//========================
//        Texture
//========================


struct GfxTexture : public Texture {
    sg_image image = {};
    sg_image_desc image_desc = {};
    //SRV (Shader Resource View) 
    sg_view read_view = {}; 
    sg_view_desc read_view_desc = {};
    //RTV (Render Target View)
    sg_view write_view = {}; 
    sg_view_desc write_view_desc = {};
};

constexpr sg_image_type ToSokol(TextureDimension d)
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

constexpr sg_pixel_format ToSokol(const TextureFormat f)
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
    //const bool color_target = FlagIntersects(p.flags, TextureFlag_ColorTarget);
    //const bool depth_stencil = FlagIntersects(p.flags, TextureFlag_DepthStencil);
    //VALIDATE_V(color_target != depth_stencil, false);

    //========
    //  Image
    //========

    tex->image_desc.type = ToSokol(p.dimension);
    switch (p.update)
    {
    case TextureUpdateType_Immutable:   tex->image_desc.usage.immutable             = true; break;
    case TextureUpdateType_Dynamic:     tex->image_desc.usage.dynamic_update        = true; break;
    case TextureUpdateType_StreamUpdate:tex->image_desc.usage.stream_update         = true; break;
    case TextureUpdateType_Count:                                               [[fallthrough]];
    default: FAIL;                                                                          break;
    }

    switch (p.type)
    {
    case TextureType_Texture:                                                               break;
    case TextureFlag_ColorTarget:   tex->image_desc.usage.color_attachment          = true; break;
    case TextureFlag_DepthStencil:  tex->image_desc.usage.depth_stencil_attachment  = true; break;
    case TextureType_Invalid:                                                   [[fallthrough]];
    case TextureType_Count:                                                     [[fallthrough]];
    default: FAIL;                                                                          break;
    }
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
    if (data)
    {
        for (i32 i = 0; i < max_mips; i++)
        {
            if (data[i].data)
            {
                tex->image_desc.data.mip_levels[i].ptr = data[i].data;
                tex->image_desc.data.mip_levels[i].size = data[i].Bytes();
            }
        }
    }
    tex->image_desc.label = tex->name.c_str();
    tex->image = sg_make_image(&tex->image_desc);


    //========
    //  View
    //========

    tex->read_view_desc.texture.image = tex->image;
    tex->read_view_desc.label = tex->name.c_str();
    tex->read_view = sg_make_view(&tex->read_view_desc);

    switch (p.type)
    {
    case TextureType_Texture:
    {
        //tex->view_desc.texture.image = tex->image;
        ////do I need these??
        //tex->view_desc.texture.mip_levels = { 0, max_mips }; //Starting mip level are hard coded
        //tex->view_desc.texture.slices = { 0, tex->image_desc.num_slices }; //Starting slice is hard coded
        break;
    }
    case TextureFlag_ColorTarget:
    {
        //color target
        tex->write_view_desc.color_attachment.image = tex->image;
        tex->write_view_desc.color_attachment.mip_level = 0;
        tex->write_view_desc.color_attachment.slice = 0;
        tex->write_view_desc.label = tex->name.c_str();
        tex->write_view = sg_make_view(tex->write_view_desc);
        break;
    }
    case TextureFlag_DepthStencil:
    {
        //depth target
        tex->write_view_desc.depth_stencil_attachment.image = tex->image;
        tex->write_view_desc.depth_stencil_attachment.mip_level = 0;
        tex->write_view_desc.depth_stencil_attachment.slice = 0;
        tex->write_view_desc.label = tex->name.c_str();
        tex->write_view = sg_make_view(tex->write_view_desc);
        break;
    }
    //tex->view_desc.storage_buffer; //Unorderd Access View equivilent for Computer Shaders I think
    //tex->view_desc.storage_image;  //Unorderd Access View equivilent for Computer Shaders I think
    //tex->view_desc.resolve_attachment; //For resolving MSAA... Do I even need these??
    case TextureType_Invalid:       [[fallthrough]];
    case TextureType_Count:         [[fallthrough]];
    default: FAIL; break;
    }
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


constexpr sg_wrap ToSokol(const SamplerWrap a)
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

constexpr sg_filter ToSokol(const SamplerFilter a)
{
    switch (a)
    {
        case SamplerFilter_Nearest: return SG_FILTER_NEAREST;
        case SamplerFilter_Linear:  return SG_FILTER_LINEAR;
        case SamplerFilter_Count: [[fallthrough]];
        default: FAIL;              return _SG_FILTER_DEFAULT;
    }
}

constexpr sg_border_color ToSokol(const SamplerBorderColor a)
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

constexpr sg_compare_func ToSokol(const GpuCompareFunc a)
{
    switch (a)
    {
        case GpuCompareFunc_Never:          return SG_COMPAREFUNC_NEVER;
        case GpuCompareFunc_Less:           return SG_COMPAREFUNC_LESS;
        case GpuCompareFunc_Equal:          return SG_COMPAREFUNC_EQUAL;
        case GpuCompareFunc_Less_equal:     return SG_COMPAREFUNC_LESS_EQUAL;
        case GpuCompareFunc_Greater:        return SG_COMPAREFUNC_GREATER;
        case GpuCompareFunc_Not_equal:      return SG_COMPAREFUNC_NOT_EQUAL;
        case GpuCompareFunc_Greater_equal:  return SG_COMPAREFUNC_GREATER_EQUAL;
        case GpuCompareFunc_Always:         return SG_COMPAREFUNC_ALWAYS;
        case GpuCompareFunc_Count: [[fallthrough]];
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
    GfxSampler* sam = AsGfx(*sampler);
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
    GfxGpuBuffer* buf = AsGfx(*buffer);
    DEBUG_LOG("GPU Buffer deleted '%s': %i\n", buf->name.c_str(), buf->buffer);
    sg_destroy_buffer(buf->buffer);
    delete buf;
}

void GpuBuffer::Upload(const void* data, const size_t in_count, const u32 in_element_size, const bool is_byte_format)
{
    ZoneScoped;
    GfxGpuBuffer* buf = AsGfx(this);
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

//struct GfxGpuBinding : GpuBinding
//{
//    sg_bindings binding;
//};
//
//bool CreateGpuBinding(GpuBinding** binding, const char* name)
//{
//    ZoneScoped;
//    GfxGpuBinding* bind = GfxGenericCreate<GpuBinding, GfxGpuBinding>(binding, name);
//    VALIDATE_V(bind, false);
//    (*binding) = AsGfx(bind);
//    return true;
//}
//
//void DeleteBuffer(GpuBinding** binding)
//{
//    ZoneScoped;
//    VALIDATE(binding);
//    GfxGpuBinding* bin = AsGfx(*binding);
//    DEBUG_LOG("GPU binding deleted '%s'\n", bin->name.c_str());
//    delete bin;
//}
//
//void GpuBinding::BindVertex(const GpuBuffer* buffer, const i32 slot)
//{
//    ZoneScoped;
//    VALIDATE(buffer && buffer->type == GpuBufferType_Vertex);
//    const GfxGpuBuffer* buf = AsGfx(buffer);
//    GfxGpuBinding* bin = AsGfx(this);
//
//    sg_buffer& b = bin->binding.vertex_buffers[slot];
//    if (b.id != 0)
//        DebugPrint("Warning: Overwriting binding(%s) slot(%i) for vertex buffer (%s)", bin->name.c_str(), slot, buf->name.c_str());
//    b = buf->buffer;
//}
//void GpuBinding::BindIndex(const GpuBuffer* buffer)
//{
//    ZoneScoped;
//    VALIDATE(buffer && buffer->type == GpuBufferType_Index);
//    const GfxGpuBuffer* buf = AsGfx(buffer);
//    GfxGpuBinding* bin = AsGfx(this);
//    sg_buffer& b = bin->binding.index_buffer;
//    if (b.id != 0)
//        DebugPrint("Warning: Overwriting binding (%s) for index buffer (%s)", bin->name.c_str(), buf->name.c_str());
//    b = buf->buffer;
//}
//void GpuBinding::BindView(GpuBuffer* view)
//{
//    ZoneScoped;
//    FAIL;
//}
//void GpuBinding::BindSampler(GpuBuffer* sampler)
//{
//    ZoneScoped;
//    FAIL;
//}
//
//void GpuBinding::Apply()
//{
//    ZoneScoped;
//    GfxGpuBinding* bin = AsGfx(this);
//    sg_apply_bindings(bin->binding);
//}






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

constexpr sg_shader_function ToSokol(const ShaderFile& s, const char* entry, const char* target)
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
constexpr sg_uniform_type ToSokol(const ShaderConstantType a)
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

sg_sampler_type  GetSokolSamplerType(const sg_image_sample_type a, const bool is_linear_filter, const GpuCompareFunc compare_func)
{
    switch (a)
    {
    case SG_IMAGESAMPLETYPE_FLOAT:
    {
        ASSERT(compare_func == GpuCompareFunc_Never);
        if (is_linear_filter)
            return SG_SAMPLERTYPE_NONFILTERING;
        else
            return SG_SAMPLERTYPE_FILTERING;
    }
    case SG_IMAGESAMPLETYPE_UNFILTERABLE_FLOAT:
    {
        ASSERT(compare_func == GpuCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_SINT:
    {
        ASSERT(compare_func == GpuCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_UINT:
    {
        ASSERT(compare_func == GpuCompareFunc_Never);
        ASSERT(is_linear_filter);
        return SG_SAMPLERTYPE_NONFILTERING;
    }
    case SG_IMAGESAMPLETYPE_DEPTH:
    {
        ASSERT(compare_func != GpuCompareFunc_Never);
        return SG_SAMPLERTYPE_COMPARISON;
    }
    default: FAIL; return _SG_SAMPLERTYPE_DEFAULT;
    }
}

//#define STB_INCLUDE_IMPLEMENTATION
//#define STB_INCLUDE_LINE_NONE
//#include "stb/stb_include.h"
//bool CreateShader(Shader** shader, const char* name, const Path& vertex_filepath, const Path& pixel_filepath)//, const Path& compute_filepath)
//{
//#error load ourselves!
//    //char *stb_include_file(char *filename, char *inject, char *path_to_includes, char error[256]);
//}

//const sg_shader_desc* Blit2D_shader_desc(sg_backend backend)

bool CreateShader(Shader** shader, const char* name, const sg_shader_desc* shader_desc, VertexID vertex_layout)
{
    VALIDATE_V(shader_desc, false);

    ZoneScoped;
    GfxShader* s = GfxGenericCreate<Shader, GfxShader>(shader, name);
    VALIDATE_V(s, false);
    (*shader) = s;
    s->shader_desc = *shader_desc;
    s->shader = sg_make_shader(s->shader_desc);
    s->params.vertex_layout = vertex_layout;

    return true;
}

//bool CreateShader(Shader** shader, const char* name, const ShaderParams& params)
//{
//    // VALIDATE_V(params.vertex.text || params.pixel.text || params.compute.text, false);
//    VALIDATE_V(params.vertex.text || params.pixel.text, false);
//    VALIDATE_V(params.vertex_layout.e, false);
//
//    ZoneScoped;
//    GfxShader* s = GfxGenericCreate<Shader, GfxShader>(shader, name);
//    VALIDATE_V(s, false);
//    (*shader) = s;
//
//    sg_shader_desc& desc = s->shader_desc;
//
//    //Vertex Pixel Computer files/text
//    if (params.vertex.text)
//        desc.vertex_func = ToSokol(params.vertex, "Vertex_Main", CSH_TOSTRING(D3D11_SHADER_MODEL_VERTEX));
//    if (params.pixel.text)
//        desc.fragment_func = ToSokol(params.pixel, "Pixel_Main", CSH_TOSTRING(D3D11_SHADER_MODEL_PIXEL));
//    //if (params.compute.text)
//    //    desc.compute_func = ToSokol(params.compute, "Main", CSH_TOSTRING(D3D11_SHADER_MODEL_COMPUTE));
//
//    const VertexData* vd = s_gfx.vertex_layouts.TryGet(params.vertex_layout);
//    VALIDATE_V(vd, false);
//    for (i32 i = 0; i < vd->count; i++)
//    {
//        desc.attrs[i] = vd->attr[i];
//    }
//
//    //Shader Constants
//    const ShaderConstantsContainer& cs = params.constants_container;
//    const u8 slot = cs.slot;
//    desc.uniform_blocks[0].size = cs.size;
//    desc.uniform_blocks[0].hlsl_register_b_n =
//        desc.uniform_blocks[0].msl_buffer_n =
//        desc.uniform_blocks[0].wgsl_group0_binding_n =
//        desc.uniform_blocks[0].spirv_set0_binding_n = slot;
//    desc.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
//    for (i32 i = 0; i < SG_MAX_UNIFORMBLOCK_MEMBERS; i++)
//    {
//        if (cs.constants[i].type == ShaderConstant_Invalid)
//            break;
//        const ShaderConstant& c = cs.constants[i];
//        desc.uniform_blocks[0].glsl_uniforms[i].type = ToSokol(c.type);
//        desc.uniform_blocks[0].glsl_uniforms[i].array_count = c.array_count;
//        desc.uniform_blocks[0].glsl_uniforms[i].glsl_name = c.name;
//    }
//
//    //NOTE(CSH): We are halving the amount of textures we can bind for ease of programming
//    // So every shader will have the textures bound to BOTH vertex AND pixel shaders
//    //TODO(CSH): seperate these and create an easy api for it OR try Sokol-SHDC
//
//    //Shader Textures
//    for (i32 i = 0; i < MAX_SHADER_TEXTURES; i++)
//    {
//        const ShaderTexture& t = params.textures[i];
//        if (!t.texture)
//            continue;
//        const GfxTexture* gt = AsGfx(t.texture);
//        const GfxSampler* gs = AsGfx(t.sampler);
//        ASSERT(gt);
//        //TEXTURE
//        //  -> Vertex
//        const sg_image_sample_type image_sample_type = GetSokolSampleType(gt->parameters.format);
//        desc.views[i].texture.stage = SG_SHADERSTAGE_VERTEX;
//        desc.views[i].texture.image_type = gt->image_desc.type;
//        desc.views[i].texture.sample_type = image_sample_type;
//        desc.views[i].texture.multisampled = gt->parameters.msaa_samples > 1;
//        desc.views[i].texture.hlsl_register_t_n =
//            desc.views[i].texture.msl_texture_n =
//            desc.views[i].texture.wgsl_group1_binding_n =
//            desc.views[i].texture.spirv_set1_binding_n = slot;
//        desc.views[i].storage_buffer; //only used for compute
//        desc.views[i].storage_image; //only used for compute
//
//        //  -> Pixel
//        const i32 pixel_view_index_offset = SG_MAX_VIEW_BINDSLOTS >> 1;
//        const i32 pvi2 = i + pixel_view_index_offset;
//        desc.views[pvi2] = desc.views[i];
//        desc.views[pvi2].texture.stage = SG_SHADERSTAGE_FRAGMENT;
//
//
//        //SAMPLER
//        //  -> Vertex
//        desc.samplers[i].stage = SG_SHADERSTAGE_VERTEX;
//        const bool is_linear_filter = gs->params.min_filter == SamplerFilter_Linear &&
//            gs->params.mag_filter == SamplerFilter_Linear &&
//            gs->params.mipmap_filter == SamplerFilter_Linear;
//        desc.samplers[i].sampler_type = GetSokolSamplerType(image_sample_type, is_linear_filter, gs->params.compare_func);
//        desc.samplers[i].hlsl_register_s_n =
//            desc.samplers[i].msl_sampler_n =
//            desc.samplers[i].wgsl_group1_binding_n =
//            desc.samplers[i].spirv_set1_binding_n = slot;
//
//        //  -> Pixel
//        const i32 pixel_sampler_index_offset = SG_MAX_SAMPLER_BINDSLOTS >> 1;
//        const i32 psi2 = i + pixel_sampler_index_offset;
//        desc.samplers[psi2] = desc.samplers[i];
//        desc.samplers[psi2].stage = SG_SHADERSTAGE_FRAGMENT;
//
//
//        //TEXTURE + SAMPLER PAIRS
//        //  -> Vertex
//        desc.texture_sampler_pairs[i].stage = SG_SHADERSTAGE_VERTEX;
//        desc.texture_sampler_pairs[i].view_slot = slot; // must be SG_VIEWTYPE_TEXTURE
//        desc.texture_sampler_pairs[i].sampler_slot = slot;
//        desc.texture_sampler_pairs[i].glsl_name = t.name;          // glsl name binding required because of GL 4.1 and WebGL2
//
//        //  -> Pixel
//        const i32 pixel_st_index_offset = SG_MAX_TEXTURE_SAMPLER_PAIRS >> 1;
//        const i32 psti2 = i + pixel_st_index_offset;
//        desc.texture_sampler_pairs[psti2] = desc.texture_sampler_pairs[i];
//        desc.texture_sampler_pairs[psti2].stage = SG_SHADERSTAGE_FRAGMENT;
//    }
//    desc.mtl_threads_per_threadgroup; //Only used for compute shaders
//    desc.label = s->name.c_str();
//
//    s->shader = sg_make_shader(s->shader_desc);
//    return true;
//}

void DeleteShader(Shader** shader)
{
    ZoneScoped;
    VALIDATE(shader);
    GfxShader* s = AsGfx(*shader);
    sg_destroy_shader(s->shader);
    DEBUG_LOG("GPU SHader deleted '%s'\n", s->name.c_str());
    delete s;
}





//========================
//       Pipeline
//========================

struct GfxPipeline : Pipeline {
    sg_pipeline pipe;
    sg_pipeline_desc pipe_desc;
};

sg_stencil_op ToSokol(const StencilOp a)
{
    switch (a)
    {
        case StencilOp_Keep:        return SG_STENCILOP_KEEP;
        case StencilOp_Zero:        return SG_STENCILOP_ZERO;
        case StencilOp_Replace:     return SG_STENCILOP_REPLACE;
        case StencilOp_IncrClamp:   return SG_STENCILOP_INCR_CLAMP;
        case StencilOp_DecrClamp:   return SG_STENCILOP_DECR_CLAMP;
        case StencilOp_Invert:      return SG_STENCILOP_INVERT;
        case StencilOp_IncrWrap:    return SG_STENCILOP_INCR_WRAP;
        case StencilOp_DecrWrap:    return SG_STENCILOP_DECR_WRAP;
        case StencilOp_Count:       [[fallthrough]];
        default: FAIL;              return _SG_STENCILOP_DEFAULT;
    }
}

sg_stencil_face_state ToSokol(const StencilOpParams& a)
{
    sg_stencil_face_state r = {
        .compare = ToSokol(a.compare),
        .fail_op = ToSokol(a.fail_op),
        .depth_fail_op = ToSokol(a.depth_fail_op),
        .pass_op = ToSokol(a.pass_op),
    };
    return r;
}

sg_stencil_state ToSokol(const StencilState& a)
{
    sg_stencil_state r = {
        .enabled = a.enabled,
        .front = ToSokol(a.front_face),
        .back = ToSokol(a.back_face),
        .read_mask = a.read_mask,
        .write_mask = a.write_mask,
        .ref = a.ref_value,

    };
    return r;
}

sg_blend_factor ToSokol(const BlendFactor a)
{
    switch (a)
    {
        case BlendFactor_Zero:              return SG_BLENDFACTOR_ZERO;
        case BlendFactor_One:               return SG_BLENDFACTOR_ONE;
        case BlendFactor_SrcColor:          return SG_BLENDFACTOR_SRC_COLOR;
        case BlendFactor_OneMinusSrcColor:  return SG_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor_SrcAlpha:          return SG_BLENDFACTOR_SRC_ALPHA;
        case BlendFactor_OneMinusSrcAlpha:  return SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor_DstColor:          return SG_BLENDFACTOR_DST_COLOR;
        case BlendFactor_OneMinusDstColor:  return SG_BLENDFACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor_DstAlpha:          return SG_BLENDFACTOR_DST_ALPHA;
        case BlendFactor_OneMinusDstAlpha:  return SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor_SrcAlphaSaturated: return SG_BLENDFACTOR_SRC_ALPHA_SATURATED;
        case BlendFactor_BlendColor:        return SG_BLENDFACTOR_BLEND_COLOR;
        case BlendFactor_OneMinusBlendColor:return SG_BLENDFACTOR_ONE_MINUS_BLEND_COLOR;
        case BlendFactor_BlendAlpha:        return SG_BLENDFACTOR_BLEND_ALPHA;
        case BlendFactor_OneMinusBlendAlpha:return SG_BLENDFACTOR_ONE_MINUS_BLEND_ALPHA;
        case BlendFactor_Src1Color:         return SG_BLENDFACTOR_SRC1_COLOR;
        case BlendFactor_OneMinusSrc1Color: return SG_BLENDFACTOR_ONE_MINUS_SRC1_COLOR;
        case BlendFactor_Src1Alpha:         return SG_BLENDFACTOR_SRC1_ALPHA;
        case BlendFactor_OneMinusSrc1Alpha: return SG_BLENDFACTOR_ONE_MINUS_SRC1_ALPHA;
        case BlendFactor_Count:             [[fallthrough]];
        default: FAIL;                      return _SG_BLENDFACTOR_DEFAULT;
    }
}

sg_blend_op ToSokol(const BlendOp a)
{
    switch (a)
    {
    case BlendOp_Add:               return SG_BLENDOP_ADD;
    case BlendOp_Subtract:          return SG_BLENDOP_SUBTRACT;
    case BlendOp_ReverseSubtract:   return SG_BLENDOP_REVERSE_SUBTRACT;
    case BlendOp_Min:               return SG_BLENDOP_MIN;
    case BlendOp_Max:               return SG_BLENDOP_MAX;
    case BlendOp_Count:             [[fallthrough]];
    default:    FAIL;               return _SG_BLENDOP_DEFAULT;
    }
}

sg_primitive_type ToSokol(const PrimitiveType a)
{
    switch (a)
    {
        case PrimitiveType_Points:          return SG_PRIMITIVETYPE_POINTS;
        case PrimitiveType_Lines:           return SG_PRIMITIVETYPE_LINES;
        case PrimitiveType_LineStrip:       return SG_PRIMITIVETYPE_LINE_STRIP;
        case PrimitiveType_Triangles:       return SG_PRIMITIVETYPE_TRIANGLES;
        case PrimitiveType_TriangleStrip:   return SG_PRIMITIVETYPE_TRIANGLE_STRIP;
        case PrimitveType_Count:            [[fallthrough]];
        default: FAIL;                      return _SG_PRIMITIVETYPE_DEFAULT;
    }
}

sg_cull_mode ToSokol(const RenderCullMode a)
{
    switch (a)
    {
        case RenderCullMode_None:   return SG_CULLMODE_NONE;
        case RenderCullMode_Front:  return SG_CULLMODE_FRONT;
        case RenderCullMode_Back:   return SG_CULLMODE_BACK;
        case RenderCullMode_Count:  [[fallthrough]];
        default: FAIL;              return _SG_CULLMODE_DEFAULT;
    }
}

sg_blend_state ToSokol(const BlendState& a)
{
    sg_blend_state r = {
        .enabled = a.enabled,
        .src_factor_rgb = ToSokol(a.src_factor_rgb),
        .dst_factor_rgb = ToSokol(a.dst_factor_rgb),
        .op_rgb = ToSokol(a.op_rgb),
        .src_factor_alpha = ToSokol(a.src_factor_alpha),
        .dst_factor_alpha = ToSokol(a.dst_factor_alpha),
        .op_alpha = ToSokol(a.op_alpha),
    };
    return r;
}

bool CreatePipeline(Pipeline** pipe, const char* name, const PipelineParams& params)
{
    ZoneScoped;
    GfxPipeline* p = GfxGenericCreate<Pipeline, GfxPipeline>(pipe, name);
    VALIDATE_V(p, false);
    (*pipe) = p;
    p->name = name;
    const GfxShader* shader = AsGfx(params.shader);
    const GfxTexture* depth = AsGfx(params.depth);

    sg_pipeline_desc& desc = p->pipe_desc;
    desc = {};
    desc.compute = false;
    desc.shader = shader->shader;
    VertexData* vertex_layout = s_gfx.vertex_layouts.TryGet(shader->params.vertex_layout);
    desc.layout = vertex_layout->state;

    //Depth
    if (depth)
    {
        VALIDATE_V(depth->image_desc.sample_count == params.msaa_sample_count, false);
        desc.depth.pixel_format = depth->image_desc.pixel_format;
        desc.depth.compare = ToSokol(params.depth_compare_func);
        desc.depth.write_enabled = false; //NOTE(CSH): Forced no write for now until needed
        desc.depth.bias = params.depth_bias;
        desc.depth.bias_slope_scale = params.depth_bias_slope_scale;
        desc.depth.bias_clamp = params.depth_bias_clamp;
    }

    //stencil
    desc.stencil = ToSokol(params.stencil);

    for (i32 i = 0; i < MAX_SHADER_TEXTURES; i++)
    {
        const RenderTarget& target = params.targets[i];
        if (target.texture)
        {
            GfxTexture* t = AsGfx(target.texture);

            VALIDATE_V(t->image_desc.sample_count == params.msaa_sample_count, false);

            sg_color_target_state& ts = desc.colors[i];
            ts.pixel_format = t->image_desc.pixel_format;
            ts.write_mask = SG_COLORMASK_RGBA;
            ts.blend = ToSokol(target.blend);

            desc.color_count = i + 1;
        }
    }
    desc.primitive_type = ToSokol(params.primitive_type);
    desc.index_type = params.has_index_buffer ? SG_INDEXTYPE_UINT32 : SG_INDEXTYPE_NONE;
    desc.cull_mode = ToSokol(params.cull_mode);
    desc.face_winding = params.front_ccw_winding_order ? SG_FACEWINDING_CCW : SG_FACEWINDING_CW;
    desc.sample_count = params.msaa_sample_count;
    desc.blend_color = {}; //NOTE(CSH): Not sure what to do with this
    desc.alpha_to_coverage_enabled = params.alpha_to_coverage_enabled;
    desc.label = p->name.c_str();
    p->pipe = sg_make_pipeline(desc);
    return true;

}

void DeletePipeline(Pipeline** pipeline)
{
    ZoneScoped;
    VALIDATE(pipeline);
    GfxPipeline* pipe = AsGfx(*pipeline);
    VALIDATE(pipe);
    sg_destroy_pipeline(pipe->pipe);
    DEBUG_LOG("Deleted pipeline'%s': %i\n", pipe->name.c_str(), pipe->pipe);
    delete pipe;
}






//========================
//       Draw Call
//========================


IdArray<DrawCall, DrawID> s_draws;

bool CreateDrawCall(const char* name, const DrawCallParams& params)
{
    ZoneScoped;
    DrawCall* draw = s_draws.CreateNew();
    VALIDATE_V(draw, false);
    draw->name = name;
    draw->params = params;
    bool result = true;

    //TODO(CSH): Remove the constant delete and new allocations and create something more static
    //Copy uniforms to the draw call
    for (i32 i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++)
    {
        const ShaderUniformData& src = params.uniforms[i];
        ShaderUniformData& dest = draw->params.uniforms[i];

        if (src.struct_data.data && src.struct_data.count)
        {
            if (src.slot >= 0 && src.slot < MAX_SHADER_UNIFORMS)
            {
                dest.slot = src.slot;
                u8* buffer = new u8[src.struct_data.Bytes()];
                dest.struct_data.data = buffer;
                dest.struct_data.count = src.struct_data.count;
                CopyArrayView(src.struct_data, dest.struct_data);
                if (src.struct_data.Bytes() != dest.struct_data.Bytes())
                {
                    DebugPrint("Failed to copy uniform data in slot %i", src.slot);
                    result = false;
                    FAIL;
                }
            }
            else
            {
                DebugPrint("Trying to bind at invalid slot: %i", src.slot);
                result = false;
                FAIL;
            }
        }
    }

    return result;
}

//DrawCall& AllocDrawCall()
//{
//    DrawCall* draw = s_draws.CreateNew();
//    if (!draw)
//    {
//        FAIL;
//        DebugPrint("Something seriously wrong CreateNew returned nullptr");
//    }
//    //if (!scissorStack.empty())
//    //{
//    //    info.scissor = scissorStack.back();
//    //}
//
//    return *draw;
//}







//========================
//    General Render
//========================

//TODO(CSH): How do we do scissor rects!?
void RenderDrawCalls()
{
    sg_swapchain swapchain = {};
    SysGetRenderSwapchain(&swapchain);

    foreach(draw, s_draws)
    {
        const DrawCallParams& params = draw->params;
        const GfxPipeline* pipe = AsGfx(params.pipeline);
        const Bindings& bind = params.bindings;
        GfxGpuBuffer* vert = AsGfx(bind.vertex_buffer);
        ASSERT(vert);
        GfxGpuBuffer* ind = AsGfx(bind.index_buffer);
        if (!vert)
        {
            DebugPrint("No Vertex buffer bound to draw, skipping: %s", draw->name.c_str());
            continue;
        }

        sg_pass pass = {};
        pass.compute = false;
        //Set Actions
        //color
        static_assert(SG_MAX_COLOR_ATTACHMENTS == MAX_COLOR_ATTACHMENTS);
        for (i32 i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++)
        {
            pass.action.colors[i].load_action = params.color_actions[i].clear_on_load ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
            pass.action.colors[i].store_action = _SG_STOREACTION_DEFAULT;
            pass.action.colors[i].clear_value = ToSokol(params.color_actions[i].clear_color);
        }
        //depth
        pass.action.depth.load_action = params.depth_action.clear_on_load ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
        pass.action.depth.store_action = _SG_STOREACTION_DEFAULT;
        pass.action.depth.clear_value = params.depth_action.clear_value;
        //stencil
        pass.action.stencil.load_action = params.stencil_action.clear_on_load ? SG_LOADACTION_CLEAR : SG_LOADACTION_LOAD;
        pass.action.stencil.store_action = _SG_STOREACTION_DEFAULT;
        pass.action.stencil.clear_value = params.stencil_action.clear_value;

        //Bind Attachments
        if (params.draw_to_backbuffer)
        {
            //Backbuffer Render Target
            pass.swapchain = swapchain;
        }
        else
        {
            //Offscreen Render Target
            for (i32 i = 0; i < SG_MAX_COLOR_ATTACHMENTS; i++)
            {
                GfxTexture* t = AsGfx(params.color_targets[i]);
                if (t)
                    pass.attachments.colors[i] = t->write_view;

                //pass.attachments.resolves; //NOTE(CSH): Unsure what the point of this is
            }
            GfxTexture* depth_texture = AsGfx(params.depth_stencil_target);
            if (depth_texture)
                pass.attachments.depth_stencil = depth_texture->write_view;
        }

        pass.label = draw->name.c_str();
        sg_begin_pass(&pass);

        sg_apply_pipeline(pipe->pipe);
        sg_bindings sb = {};
        sb.vertex_buffers[0] = vert->buffer;
        sb.vertex_buffer_offsets[0] = {};
        sb.index_buffer = ind ? ind->buffer : sg_buffer();
        sb.index_buffer_offset = {};

        for (i32 i = 0; i < bind.read_textures.used; i++)
        {
            GfxTexture* t = AsGfx(bind.read_textures[i]);
            if (!t)
            {
                DebugPrint("Added element to read_textures but no valid texture: %i '%s'", i, draw->name.c_str());
                FAIL;
                continue;
            }
            sb.views[i] = t->read_view;
        }
        for (i32 i = 0; i < bind.samplers.used; i++)
        {
            StaticArray<Sampler*, MAX_SHADER_SAMPLERS> samplers = {};
            GfxSampler* s = AsGfx(bind.samplers[i]);
            if (!s)
            {
                DebugPrint("Added element to samplers but no valid sampler: %i '%s'", i, draw->name.c_str());
                FAIL;
                continue;
            }
            sb.samplers[i] = s->sampler;
        }

        sb.samplers[SG_MAX_SAMPLER_BINDSLOTS] = {};
        sg_apply_bindings(&sb);

        //Constant Buffer / Uniforms
        static_assert(MAX_SHADER_UNIFORMS == SG_MAX_UNIFORMBLOCK_BINDSLOTS);
        for (i32 i = 0; i < MAX_SHADER_UNIFORMS; i++)
        {
            const ShaderUniformData& u = params.uniforms[i];
            if (u.struct_data.data && u.struct_data.count)
            {
                ASSERT(u.slot >= 0 && u.slot < MAX_SHADER_UNIFORMS);
                const sg_range range = { u.struct_data.data, u.struct_data.Bytes() };
                sg_apply_uniforms(u.slot, &range);
            }
        }

        if (params.scissor.left  != 0.0f &&
            params.scissor.bot   != 0.0f &&
            params.scissor.right != 0.0f &&
            params.scissor.top   != 0.0f)
        {
            const SimpleRect& r = params.scissor;
            sg_apply_scissor_rectf(r.left, r.bot, r.Width(), r.Height(), false);
        }

        sg_draw_ex(0, params.vertex_length, 1, params.vertex_index, 0);

        sg_end_pass();

        //Delete the copied uniforms
        for (i32 i = 0; i < SG_MAX_UNIFORMBLOCK_BINDSLOTS; i++)
        {
            ShaderUniformData& u = draw->params.uniforms[i];
            if (u.struct_data.data)
                delete u.struct_data.data;
        }

    }
}

bool RenderInitSokol()
{
    SysRenderInitDesc sys_desc = {};
    sys_desc.size = gfx.window_size;
    sys_desc.sample_count = 1;
    sys_desc.no_depth_buffer = true;

#ifdef WIN32
    gfx.backend = sys_desc.backend = CashRenderBackend_D3D11;
#elif defined(LINUX)
    sys_desc.backend = CashRenderBackend_Vulkan;
#elif defined(MACOS)
    sys_desc.backend = CashRenderBackend_Metal_Macos;
#else
    sys_desc.backend = CashRenderBackend_Glcore;
#error what is this OS supposed to support?
#endif

    bool result = SysRenderInit(&sys_desc);
    sg_desc desc = { };
    //desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    //desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    SysGetRenderEnvironment(&desc.environment);
    //TODO(CSH): override allocator with custom frame temp memory?
    desc.allocator;     // optional memory allocation hooks.  Default is malloc and free
    desc.logger = { SgLogFunc, nullptr };
    sg_setup(&desc);
    return result;
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

    {// Vertex_2D
        VertexParams params[] = {
            { 0, 0, offsetof(Vertex_2D, position),  VertexFormat_Float2,    VertexSemantic_Position,    0, VertexStep_Vertex, 0 },
            { 1, 0, offsetof(Vertex_2D, color),     VertexFormat_Float4,    VertexSemantic_Color,       0, VertexStep_Vertex, 0 },
            { 2, 0, offsetof(Vertex_2D, uv),        VertexFormat_Float2,    VertexSemantic_TexCoord,    0, VertexStep_Vertex, 0 },
        };
        gfx.vertex_2d_layout = CreateVertexLayout(CreateArrayView(params));
    }
    {// Vertex_PNTC
        VertexParams params[] = {
            { 0, 0, offsetof(Vertex_PNTC, position), VertexFormat_Float,    VertexSemantic_Position,    0, VertexStep_Vertex, 0 },
            { 1, 0, offsetof(Vertex_PNTC, normal),   VertexFormat_Float3,   VertexSemantic_Normal,      0, VertexStep_Vertex, 0 },
            { 2, 0, offsetof(Vertex_PNTC, uv),       VertexFormat_Float2,   VertexSemantic_TexCoord,    0, VertexStep_Vertex, 0 },
            { 3, 0, offsetof(Vertex_PNTC, color),    VertexFormat_Float4,   VertexSemantic_Color,       0, VertexStep_Vertex, 0 },
        };
        gfx.vertex_pntc_layout = CreateVertexLayout(CreateArrayView(params));
    }

    {
        Vertex_2D verts[] = {
            { { -1.0f, +1.0f }, White, { 0.0f, 0.0f }, },  // Top Left
            { { -1.0f, -3.0f }, White, { 0.0f, 2.0f }, },  // Bot Left
            { { +3.0f, +1.0f }, White, { 2.0f, 0.0f }, }   // Top Right
        };

        CreateGpuBuffer(&gfx.vertex_2d_verts, "Fullscreen Triangle VB", GpuBufferType_Vertex, GpuBufferFlag_Immutable);
        gfx.vertex_2d_verts->Upload(verts);
    }

    s_gfx.pass_action.colors[0].load_action = SG_LOADACTION_CLEAR;
    s_gfx.pass_action.colors[0].clear_value = ToSokol(background_color);

    {
        const sg_shader_desc* desc = Blit2D_shader_desc(ToSokol(gfx.backend));
        CreateShader(&gfx.blit2d_shader, "Blit 2D Shader", desc, gfx.vertex_2d_layout);
    }

    gfx.blend_normal = {
        .enabled = true,
        .src_factor_rgb = BlendFactor_SrcAlpha,
        .dst_factor_rgb = BlendFactor_OneMinusSrcAlpha,
        .op_rgb      = BlendOp_Add,
        .src_factor_alpha = BlendFactor_One,
        .dst_factor_alpha = BlendFactor_OneMinusSrcAlpha,
        .op_alpha    = BlendOp_Add,
    };

    gfx.blend_no_color_write = {
        .enabled = false,
        .src_factor_rgb = BlendFactor_SrcAlpha,
        .dst_factor_rgb = BlendFactor_OneMinusSrcAlpha,
        .op_rgb      = BlendOp_Add,
        .src_factor_alpha = BlendFactor_One,
        .dst_factor_alpha = BlendFactor_One,
        .op_alpha    = BlendOp_Add,
    };

    gfx.stencil_2d = {
        .enabled = false,
        .front_face = {
            .compare = GpuCompareFunc_Always,
            .fail_op        = StencilOp_Keep,
            .depth_fail_op  = StencilOp_Keep,
            .pass_op        = StencilOp_Replace,
        },
        .back_face = {
            .compare = GpuCompareFunc_Always,
            .fail_op        = StencilOp_Keep,
            .depth_fail_op  = StencilOp_DecrClamp,
            .pass_op        = StencilOp_Keep,
        },
        .read_mask    = 0,
        .write_mask   = 0,
        .ref_value    = 0,
    };

    {
        TextureParams params = {
            .size = Vec3I(gfx.window_size, 0),
            .msaa_samples = 1,
            .mip_count = 1,
            .dimension = TextureDimension_2D,
            .format = TextureFormat_RG11B10_FLOAT,
            .type = TextureFlag_ColorTarget,
            .update = TextureUpdateType_Immutable,
        };
        //how does this work with uploading "{}"
        CreateTextureAndUpload(&gfx.hdr_target, "HDR Render Target", params, {});
    }

    {
        sg_swapchain swapchain;
        SysGetRenderSwapchain(&swapchain);
        GfxShader* shader = AsGfx(gfx.blit2d_shader);

        VertexData* vertex_layout = s_gfx.vertex_layouts.TryGet(shader->params.vertex_layout);
        sg_pipeline_desc desc = {
            .shader = shader->shader,
            .layout = vertex_layout->state,
            .color_count = 1,
            .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
            .index_type = SG_INDEXTYPE_NONE,
            .cull_mode = SG_CULLMODE_NONE,
            .face_winding = SG_FACEWINDING_CCW,
            .sample_count = 1,
            .label = "Resolve Backbuffer Pipeline",
        };
        desc.depth.compare = SG_COMPAREFUNC_ALWAYS,//ignore depth
        desc.colors[0].pixel_format = swapchain.color_format;
        desc.colors[0].write_mask   = SG_COLORMASK_RGBA;
        desc.colors[0].blend        = ToSokol(gfx.blend_normal);
        desc.colors[0].blend.enabled = false;
        s_gfx.final_render_pipeline = sg_make_pipeline(&desc);
    }
    {
        sg_sampler_desc desc = {
            .min_filter = SG_FILTER_LINEAR,
            .mag_filter = SG_FILTER_LINEAR,
        };
        s_gfx.final_render_sampler = sg_make_sampler(&desc);
    }

    {
        SamplerParams params = {
        .min_filter = SamplerFilter_Linear,
        .mag_filter = SamplerFilter_Linear,
        .mipmap_filter = SamplerFilter_Linear,
        .wrap_u = SamplerWrap_Repeat,
        .wrap_v = SamplerWrap_Repeat,
        .wrap_w = SamplerWrap_Repeat,
        .min_lod = 0.0f,
        .max_lod = FLT_MAX,
        .border_color = SamplerBorderColor_TransparentBlack,
        .compare_func = GpuCompareFunc_Always,
        .max_anisotropy = 16,
        };
        CreateSampler(&gfx.common_anisotropic_sampler, "Basic Anisotropic Sampler", params);
        params.max_anisotropy = 1;
        CreateSampler(&gfx.common_sampler, "Basic Sampler", params);
    }
    {
        u8 pixel_texture_data[] = { 255, 255, 255, 255 };
        TextureParams params = {
            .size = { 1, 1, 0 },
            .msaa_samples = 1,
            .mip_count = 1,
            .dimension = TextureDimension_2D,
            .format = TextureFormat_RGBA8_UNORM_SRGB,

            .type = TextureType_Texture,
            .update = TextureUpdateType_Immutable,
        };
        CreateTextureAndUpload(&gfx.plain_texture, "Plain Texture", params, CreateArrayView(pixel_texture_data));
    }

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
    desc.color_format = AsGfx(gfx.hdr_target)->image_desc.pixel_format;
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

//TODO(CSH): Clean this up
// This needs to all be done under RenderDrawCalls
// the imgui pass needs to be done in RenderDrawCalls to the gfx.hdr_target
void CashRender()
{
    ZoneScoped;
    {
        RenderDrawCalls();
    }

    sg_swapchain swapchain = {};
    SysGetRenderSwapchain(&swapchain);
    {
        ZoneScopedN("ImGui Render");
        sg_pass pass = {};
#if 1
        pass.compute = false;

        pass.action.colors[0].load_action = SG_LOADACTION_LOAD;
        pass.action.colors[0].store_action = SG_STOREACTION_STORE;
        pass.action.colors[0].clear_value = { 0.0f, 0.0f, 0.0f, 1.0f };
        pass.attachments.colors[0] = AsGfx(gfx.hdr_target)->write_view;
        pass.swapchain;
        pass.label = "ImGui Render Pass";
#else

        pass.action = s_gfx.pass_action;
        pass.swapchain = swapchain;
#endif
        sg_begin_pass(&pass);

        simgui_render();
        //ImGui::Render();
        sg_end_pass();
    }

    {
        //Resolve hdr_target to backbuffer
        sg_pass pass = {};

        pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
        pass.action.colors[0].store_action = SG_STOREACTION_STORE;
        pass.action.colors[0].clear_value = ToSokol(background_color);

        //pass.attachments.colors[0] = AsGfx(gfx.hdr_target)->view;
        pass.swapchain = swapchain;
        pass.label = "Resolve HDR to Backbuffer";

        sg_begin_pass(&pass);

        sg_apply_pipeline(s_gfx.final_render_pipeline);
        sg_bindings bindings = {};
        bindings.vertex_buffers[0] = AsGfx(gfx.vertex_2d_verts)->buffer;
        bindings.views[0] = AsGfx(gfx.hdr_target)->read_view;
        bindings.samplers[0] = s_gfx.final_render_sampler;
        sg_apply_bindings(&bindings);

        static_assert(sizeof(ShaderConstants_Blit2D) == sizeof(ShaderConstants_2D_t));
        ShaderConstants_Blit2D uniform = {};
#if 1
        uniform.orthographic = gb_mat4_identity();
#else
        const Vec2 pos = {};
        const Vec2 size = ToVec2(gfx.window_size);
        float L = pos.x;
        float R = pos.x + size.x;
        float T = pos.y;
        float B = pos.y + size.y;
        uniform.orthographic = {
            2.0f/(R-L),   0.0f,           0.0f,       0.0f ,
            0.0f,         2.0f/(T-B),     0.0f,       0.0f ,
            0.0f,         0.0f,           0.5f,       0.0f ,
            (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f ,};
#endif

        const sg_range range = { &uniform, sizeof(ShaderConstants_Blit2D) };
        sg_apply_uniforms(UB_ShaderConstants_2D, &range);
        sg_draw(0, 3, 1);
        sg_end_pass();
    }
    sg_commit();

    SysRenderPresent();
}

void CashRenderDestroy()
{
    SDL_DestroyRenderer(gfx.context);
}
