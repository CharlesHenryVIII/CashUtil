#pragma once
#include "CashMath.h"
#include "CashArrayView.h"

#include "SDL3/SDL.h"
//#define CASH_SDL_RENDER 1
#define CASH_SOKOL_RENDER 1

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


//========================
//        Texture
//========================

enum TextureIndex : u32 {
    TextureIndex_Invalid,
    TextureIndex_Minecraft,
    TextureIndex_Plain,
    TextureIndex_Random,
    TextureIndex_Backbuffer_Silhouette,
    TextureIndex_Backbuffer_Depth,
    TextureIndex_Backbuffer_HDR,
    TextureIndex_Backbuffer_Shadow_Depth,
    TextureIndex_FontStandard,
    TextureIndex_FontBold,
    TextureIndex_FontOptimusPrinceps,
    TextureIndex_SkyboxDay,
    TextureIndex_SkyboxNight,
    //TextureIndex_Shadow_Map,
    TextureIndex_EditorAssetCover,
    TextureIndex_Count,
};
ENUMOPS(TextureIndex);

enum TextureDimension : u32 {
    TextureDimension_Invalid,
    TextureDimension_1D,
    TextureDimension_2D,
    TextureDimension_3D,
    TextureDimension_Count,
};
ENUMOPS(TextureDimension);

enum TextureFilter : u32 {
    TextureFilter_Invalid,
    TextureFilter_MIN_MAG_MIP_POINT,
    TextureFilter_MIN_MAG_POINT_MIP_LINEAR,
    TextureFilter_MIN_POINT_MAG_LINEAR_MIP_POINT,
    TextureFilter_MIN_POINT_MAG_MIP_LINEAR,
    TextureFilter_MIN_LINEAR_MAG_MIP_POINT,
    TextureFilter_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    TextureFilter_MIN_MAG_LINEAR_MIP_POINT,
    TextureFilter_MIN_MAG_MIP_LINEAR,
    TextureFilter_ANISOTROPIC,
    TextureFilter_COMPARISON_MIN_MAG_MIP_POINT,
    TextureFilter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
    TextureFilter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
    TextureFilter_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
    TextureFilter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
    TextureFilter_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    TextureFilter_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
    TextureFilter_COMPARISON_MIN_MAG_MIP_LINEAR,
    TextureFilter_COMPARISON_ANISOTROPIC,
    TextureFilter_Count,
};
ENUMOPS(TextureFilter);

enum TextureAddressMode : u32 {
    TextureAddressMode_Invalid,
    TextureAddressMode_Wrap,
    TextureAddressMode_Mirror,
    TextureAddressMode_Clamp,
    TextureAddressMode_Border,
    TextureAddressMode_MirrorOnce,
    TextureAddressMode_Count,
};
ENUMOPS(TextureAddressMode);

enum TextureFormat : u32 {
    TextureFormat_UNKNOWN,
    TextureFormat_R32G32B32A32_FLOAT,
    TextureFormat_R32G32B32A32_UINT,
    TextureFormat_R32G32B32_FLOAT,
    TextureFormat_R32G32B32_UINT,
    TextureFormat_R32G32_FLOAT,
    TextureFormat_R16G16B16A16_UINT,
    TextureFormat_R11G11B10_FLOAT,
    TextureFormat_D32_FLOAT,
    TextureFormat_R32_FLOAT,
    TextureFormat_D24_UNORM_S8_UINT,
    TextureFormat_R24_UNORM_X8_TYPELESS,
    TextureFormat_D16_UNORM,
    TextureFormat_A8_UNORM,
    TextureFormat_R8G8B8A8_UNORM,
    TextureFormat_R8G8B8A8_UNORM_SRGB,
    TextureFormat_R8G8B8A8_UINT,
    TextureFormat_R8G8_UINT,
    TextureFormat_R8_UNORM,
    TextureFormat_R8_UINT,
    TextureFormat_Count,
};
ENUMOPS(TextureFormat);

enum TextureType : u32 {
    TextureType_Invalid,
    TextureType_Texture,
    TextureType_Depth,
    TextureType_TextureCube,
    TextureType_Count,
};
ENUMOPS(TextureType);

struct TextureParams {
    Vec3I size;
    TextureFormat format = TextureFormat_R8G8B8A8_UINT;
    TextureAddressMode mode = TextureAddressMode_Wrap;
    TextureFilter filter = TextureFilter_ANISOTROPIC;
    TextureType type = TextureType_Texture;
    bool render_target;
    i32 bytes_per_pixel;
    std::wstring name;
    u32 array_size = 1;
};

struct Texture {
    u32 mip_levels = 1;
    TextureDimension dimension;
    TextureParams parameters;
};

bool CreateTexture(Texture** texture, const void* data, Vec3I size, TextureFormat format, i32 bytes_per_pixel, const std::wstring& name, TextureType type = TextureType_Texture);
bool CreateTexture(Texture** texture, const char* fileLocation, TextureFormat format, TextureFilter filter, const std::wstring& name, TextureType type = TextureType_Texture);
bool CreateTexture(Texture** texture, const TextureParams& tp, u32 mip_levels, const u8* data);
bool CreateTexture(Texture** texture, const TextureParams& tp, const void* data);
bool UpdateTexture(Texture** texture, u32 mip_slice, void* data, u32 row_pitch_bytes, u32 depth_pitch_bytes);
void DeleteTexture(Texture** texture);
void* GetShaderResourceView(TextureIndex t);




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
    GpuBufferType_Constant,
    GpuBufferType_Structure,
    GpuBufferType_RWStructure,
    GpuBufferType_AppendStructure,
    GpuBufferType_IndirectArgs,
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

struct GpuBuffer
{
    bool is_dymamic = true;
    GpuBufferType type = GpuBufferType_Invalid;
    std::wstring name;
    size_t count = 0;
    size_t element_size = 0;

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
    template<typename T>
    inline void Upload(const T& a)
    {
        Upload(&a, 1, sizeof(a));
    }
    //Bind a Constant or Structure buffer
    void Bind(u32 slot, GpuBufferBindLocation binding);
};

bool CreateGpuBuffer(GpuBuffer** buffer, const std::wstring& name, bool is_dynamic, GpuBufferType type);
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

