#pragma once
#include "CashMath.h"
#include "CashArrayView.h"

#include "SDL3/SDL.h"
//#define CASH_SDL_RENDER 1
#define CASH_SOKOL_RENDER 1

#define CASH_GFX_MAX_MIPS 16

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

bool CashRenderInit(ArrayView<const ArrayView<const u8>> app_icons);
void CashRenderDestroy();
void CashRender();
void CashImguiInit();
void CashImguiDestroy();
void CashImguiNewFrame(double delta_time);




//========================
//        Texture
//========================

enum TextureDimension : u32 {
    TextureDimension_Invalid,
    TextureDimension_2D,
    TextureDimension_3D,
    TextureDimension_CUBE,
    TextureDimension_ARRAY,
    TextureDimension_Count,
};
ENUMOPS(TextureDimension);

enum TextureFormat : u32 {
    TextureFormat_UNKNOWN,
    TextureFormat_RGBA32_FLOAT,
    TextureFormat_RGBA32_UINT,
    TextureFormat_RG32_FLOAT,
    TextureFormat_RGBA16_UINT,
    TextureFormat_RG11B10_FLOAT,
    TextureFormat_R32_FLOAT,
    TextureFormat_RGBA8_UNORM,
    TextureFormat_RGBA8_UNORM_SRGB,
    TextureFormat_RGBA8_UINT,
    TextureFormat_RG8_UINT,
    TextureFormat_R8_UNORM,
    TextureFormat_R8_UINT,
    TextureFormat_Depth,
    TextureFormat_DepthStencil,
    TextureFormat_Count,
};
ENUMOPS(TextureFormat);

enum TextureType : u32 {
    TextureType_Invalid,
    TextureType_Texture,
    TextureType_Depth,
    TextureType_Count,
};
ENUMOPS(TextureType);

enum TextureFlag : u32 {
    TextureFlag_None = 0,
    TextureFlag_RenderTarget = BIT(0), //Texture can be rendered to
    TextureFlag_DepthStencil = BIT(1), //Texture is a depth buffer otherwise its for color
    TextureFlag_Immutable    = BIT(3), //Texture Updated by CPU: Never
    TextureFlag_Dynamic      = BIT(2), //Texture Updated by CPU: Infrequently
    TextureFlag_StreamUpdate = BIT(4), //Texture Updated by CPU: Every Frame
    TextureFlag_All          = BIT(5) - 1,
};
ENUMOPS(TextureFlag);

struct TextureParams {
    Vec3I size; // .x = width, .y = height, .z = "Depth" for 3D texture / "Slices" for Array / 6 Slices for cubemap
    i32 msaa_samples = 1; //1 = No MSAA, 2 = 2x MSAA, 4 = 4x MSAA, etc
    i32 mip_count = 1;

    TextureFlag flags = TextureFlag_None;
    TextureDimension dimension = TextureDimension_2D;
    TextureFormat format = TextureFormat_RGBA8_UINT;
    TextureType type = TextureType_Texture;
};

struct Texture {
    std::string name;
    TextureParams parameters;
    u32 mip_levels = 1;
};

bool CreateTextureAndUpload(Texture** texture, const char* name, const TextureParams& tp, ArrayView<u8> data);
bool CreateTextureAndUpload(Texture** texture, const char* name, const TextureParams& tp, ArrayView<u8> data[CASH_GFX_MAX_MIPS]);
//bool CreateTexture(Texture** texture, Vec3I size, TextureFormat format, i32 bytes_per_pixel, const std::string& name, TextureType type = TextureType_Texture);
//bool CreateTexture(Texture** texture, TextureFormat format, TextureFilter filter, const std::string& name, TextureType type = TextureType_Texture);
//bool UpdateTexture(Texture** texture, u32 mip_slice, void* data, u32 row_pitch_bytes, u32 depth_pitch_bytes);
void DeleteTexture(Texture** texture);




//========================
//       Sampler
//========================

enum SamplerFilter : u32 {
    SamplerFilter_Nearest,
    SamplerFilter_Linear,
    SamplerFilter_Count,
};
ENUMOPS(SamplerFilter);

enum SamplerWrap : u32 {
    SamplerWrap_Repeat,
    SamplerWrap_ClampToEdge,
    SamplerWrap_ClampToBorder,
    SamplerWrap_Mirrored_Repeat,
    SamplerWrap_Count,
};
ENUMOPS(SamplerWrap);

enum SamplerBorderColor : u32 {
    SamplerBorderColor_TransparentBlack,
    SamplerBorderColor_OpaqueBlack,
    SamplerBorderColor_OpaqueWhite,
    SamplerBorderColor_Count,
};
ENUMOPS(SamplerBorderColor);

enum SamplerCompareFunc : u32 {
    SamplerCompareFunc_Never,
    SamplerCompareFunc_Less,
    SamplerCompareFunc_Equal,
    SamplerCompareFunc_Less_equal,
    SamplerCompareFunc_Greater,
    SamplerCompareFunc_Not_equal,
    SamplerCompareFunc_Greater_equal,
    SamplerCompareFunc_Always,
    SamplerCompareFunc_Count,
};
ENUMOPS(SamplerCompareFunc);

struct SamplerParams {
    SamplerFilter min_filter = SamplerFilter_Linear;
    SamplerFilter mag_filter = SamplerFilter_Linear;
    SamplerFilter mipmap_filter = SamplerFilter_Linear;
    SamplerWrap wrap_u = SamplerWrap_Repeat;
    SamplerWrap wrap_v = SamplerWrap_Repeat;
    SamplerWrap wrap_w = SamplerWrap_Repeat;
    float min_lod = 0.0f;
    float max_lod = FLT_MAX;
    SamplerBorderColor border_color = SamplerBorderColor_TransparentBlack;
    SamplerCompareFunc compare_func = SamplerCompareFunc_Always;
    uint32_t max_anisotropy = 1; //must be between 1 and 16
};

struct Sampler {
    std::string name;
    SamplerParams params;
};

bool CreateSampler(Sampler** buffer, const char* name, const SamplerParams& params);
void DeleteSampler(Sampler** buffer);




//========================
//       GpuBuffer
//========================

typedef u32 Index;

enum VertexType : u32
{
    VertexType_2D,
    VertexType_PNTC,
    VertexType_Count,
};
ENUMOPS_PURE(VertexType);

enum GpuBufferType : u32 {
    GpuBufferType_Invalid,
    GpuBufferType_Vertex,
    GpuBufferType_Index,
    //GpuBufferType_Constant,
    GpuBufferType_Structure,
    //GpuBufferType_RWStructure,
    //GpuBufferType_AppendStructure,
    //GpuBufferType_IndirectArgs,
    GpuBufferType_Count,
};
ENUMOPS(GpuBufferType);

enum GpuBufferBindLocation : u32 {
    GpuBufferBindLocation_Invalid,
    GpuBufferBindLocation_Vertex,
    GpuBufferBindLocation_Pixel,
    GpuBufferBindLocation_Compute,
    GpuBufferBindLocation_All,
    GpuBufferBindLocation_Count,
};
ENUMOPS(GpuBufferBindLocation);

enum GpuBufferFlag : u32 {
    GpuBufferFlag_None = 0,
    GpuBufferFlag_Immutable     = BIT(0), //Buffer Updated by CPU: Never
    GpuBufferFlag_Dynamic       = BIT(1), //Buffer Updated by CPU: Infrequently
    GpuBufferFlag_StreamUpdate  = BIT(2), //Buffer Updated by CPU: Every Frame
    //GpuBufferFlag_WriteUnsealed = BIT(3),
    GpuBufferFlag_All = BIT(3) - 1,
};
ENUMOPS(GpuBufferFlag);

struct GpuBuffer
{
    std::string name;
    GpuBufferFlag flags = GpuBufferFlag_None;
    GpuBufferType type = GpuBufferType_Invalid;
    size_t count = 0;
    size_t element_size = 0;
    bool has_uploaded = false;

    void Upload(const void* data, const size_t count, const u32 element_size, const bool is_byte_format = false);
    template<typename T>
    inline void Upload(const std::vector<T>& a)
    {
        ASSERT(a.size());
        Upload(a.data(), a.size(), sizeof(T), false);
    }
    template<typename T>
    inline void Upload(const ArrayView<T> a, const bool is_byte_format = false)
    {
        ASSERT(a.size());
        Upload(a.data, a.size(), (u32)a.ElementBytes());
    }
    template<typename T, u64 size>
    inline void Upload(const T (&source)[size])
    {
        Upload(source, size, sizeof(source[0]));
    }
    template<typename T>
    inline void Upload(const T& a)
    {
        Upload(&a, 1, sizeof(a));
    }
};

bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, GpuBufferType type, GpuBufferFlag flags);
void DeleteBuffer(GpuBuffer** buffer);




//========================
//        Shader
//========================

struct ShaderMacro {
    std::string name;
    std::string value;
};

enum ShaderType : u32 {
    ShaderType_Invalid,
    ShaderType_Vertex,
    ShaderType_Pixel,
    ShaderType_Compute,
    ShaderType_Count,
};
ENUMOPS(ShaderType);

enum ShaderIndex : u16 {
    ShaderIndex_Invalid,
    ShaderIndex_Cube,
    ShaderIndex_InstancedPrimitiveUnlit,
    ShaderIndex_InstancedPrimitiveLit,
    ShaderIndex_InstancedPrimitiveHighlight,
    ShaderIndex_Copy,
    ShaderIndex_GaussianBlur,
    ShaderIndex_FinalBlend,
    ShaderIndex_GLTF,
    ShaderIndex_MeshDepthOnly,
    ShaderIndex_Line,
    ShaderIndex_TriangleMesh,
    ShaderIndex_Font,
    ShaderIndex_Basic,
    ShaderIndex_EditorGrid,
    ShaderIndex_Skybox,
    ShaderIndex_Billboard,
    ShaderIndex_CSParticleSpawn,
    ShaderIndex_CSParticleUpdate,
    ShaderIndex_CSParticleDispatchArgs,
    ShaderIndex_CSParticleDrawArgs,
    ShaderIndex_Count,
};
ENUMOPS(ShaderIndex);

enum ShaderClass : u32 {
    ShaderClass_Vertex,
    ShaderClass_Instanced,
    ShaderClass_Count,
};
ENUMOPS(ShaderClass);

struct Shader
{
    struct InputElementDesc {
        const char* semantic_name;
        u32 semantic_index = 0;
        TextureFormat Format;
        u32 input_slot;
        u32 AlignedByteOffset;
        ShaderClass input_class;
        u32 instance_step_rate;
    };

    static const u32 vertex_component_max = 4;

    ~Shader();
    void CheckForUpdate();

    std::string name;
    std::string vertex_file;
    std::string pixel_file;
    std::string compute_file;
    std::vector<ShaderMacro> macros;
    u64 vertex_last_write_time = {};
    u64 pixel_last_write_time = {};
    u64 compute_last_write_time = {};
    u32 vertex_component_count = 0;
    u32 input_stride_bytes = 0;
    std::vector<std::string> reference_file_names;
    std::vector<u64>         reference_file_times;

    bool CompileShader(std::string text, const std::string& file_name, ShaderType shader_type, std::string entry_name = "");
    //bool CompileShader(std::string text, const std::string& file_name, ShaderType shader_type);
};
bool CreateShader(Shader** shader,
    const std::string& vertexFileLocation,
    const std::string& pixelFileLocation,
    ArrayView<Shader::InputElementDesc> input_layout,
    std::vector<ShaderMacro> macros = std::vector<ShaderMacro>());
bool CreateComputeShader(Shader** shader,
    const std::string file_location,
    const std::string entry_point);
inline bool CreateShader(Shader** s, const std::string& shader_file_location, ArrayView<Shader::InputElementDesc> input_layout, std::vector<ShaderMacro> macros = std::vector<ShaderMacro>())
{
    return CreateShader(s, shader_file_location, shader_file_location, input_layout, macros);
}


struct GpuBinding
{
    std::string name;

    void BindVertex(const GpuBuffer* buffer, const i32 slot);
    void BindIndex(const GpuBuffer* buffer);
    void BindView(GpuBuffer* view);
    void BindSampler(GpuBuffer* sampler);
    void Apply();
};
bool CreateGpuBinding(GpuBinding** binding, const char* name);
void DeleteBuffer(GpuBinding** binding);
