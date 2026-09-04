#pragma once
#include "CashMath.h"
#include "CashArrayView.h"
#include "CashString.h"
#include "CashIdArray.h"

#include "SDL3/SDL.h"

#define CASH_GFX_MAX_MIPS 16
#define MAX_SHADER_UNIFORMS 8
#define MAX_SHADER_UNIFORM_MEMBERS 16
#define MAX_SHADER_TEXTURES 12
#define MAX_COLOR_ATTACHMENTS 8
#define MAX_SHADER_READ_TEXTURES 32
#define MAX_SHADER_SAMPLERS 12

#pragma pack(push, 1)
struct ShaderConstants_Blit2D {
    Mat4 orthographic;
};
#pragma pack(pop)


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
ENUMOPS_PURE(EmbededIcon);

enum CashRenderBackend : u32 {
    CashRenderBackend_Glcore,
    CashRenderBackend_Gles3,
    CashRenderBackend_D3D11,
    CashRenderBackend_Metal_Ios,
    CashRenderBackend_Metal_Macos,
    CashRenderBackend_Metal_Simulator,
    CashRenderBackend_WGpu,
    CashRenderBackend_Vulkan,
    CashRenderBackend_Count,
};
ENUMOPS_PURE(CashRenderBackend);

bool CashRenderInit(ArrayView<const ArrayView<const u8>> app_icons);
void CashRenderDestroy();
void CashRender();
void CashImguiInit();
void CashImguiDestroy();
void CashImguiNewFrame(double delta_time);






//========================
//     Vertex Data
//========================

DATAID_TYPE(VertexID)

enum VertexFormat : u32 {
    VertexFormat_Invalid,
    VertexFormat_Float,
    VertexFormat_Float2,
    VertexFormat_Float3,
    VertexFormat_Float4,
    VertexFormat_Int,
    VertexFormat_Int2,
    VertexFormat_Int3,
    VertexFormat_Int4,
    VertexFormat_UInt,
    VertexFormat_UInt2,
    VertexFormat_UInt3,
    VertexFormat_UInt4,
    VertexFormat_Byte4,
    VertexFormat_Byte4N,
    VertexFormat_UByte4,
    VertexFormat_UByte4N,
    VertexFormat_Short2,
    VertexFormat_Short2N,
    VertexFormat_UShort2,
    VertexFormat_UShort2N,
    VertexFormat_Short4,
    VertexFormat_Short4N,
    VertexFormat_UShort4,
    VertexFormat_UShort4N,
    VertexFormat_Int10_N2,
    VertexFormat_UInt10_N2,
    VertexFormat_Half2,
    VertexFormat_Half4,
    VertexFormat_Count,
};
ENUMOPS_PURE(VertexFormat);

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
ENUMOPS_PURE(VertexSemantic);

enum VertexStep : u32 {
        VertexStep_Vertex,
        VertexStep_Instance,
        VertexStep_Count,
};
ENUMOPS_PURE(VertexStep);

struct VertexParams {
    i32 vertex_index;
    i32 buffer_index;
    i32 offset;
    VertexFormat format;
    VertexSemantic semantic;
    i32 semantic_index;
    VertexStep vertex_step;
    i32 vertex_step_rate;
};

VertexID CreateVertexLayout(ArrayView<VertexParams> vertex_layout);
bool DeleteVertexLayout(VertexID id);




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
    TextureType_Texture,        //Texture
    TextureFlag_ColorTarget,    //Texture can be rendered to
    TextureFlag_DepthStencil,   //Texture is a depth buffer otherwise its for color
    TextureType_Count,
};
ENUMOPS(TextureType);

enum TextureUpdateType : u32 {
    TextureUpdateType_Immutable,    //Texture Updated by CPU: Never
    TextureUpdateType_Dynamic,      //Texture Updated by CPU: Infrequently
    TextureUpdateType_StreamUpdate, //Texture Updated by CPU: Every Frame
    TextureUpdateType_Count,
};
ENUMOPS(TextureUpdateType);

struct TextureParams {
    Vec3I size; // .x = width, .y = height, .z = "Depth" for 3D texture / "Slices" for Array / 6 Slices for cubemap
    i32 msaa_samples = 1; //1 = No MSAA, 2 = 2x MSAA, 4 = 4x MSAA, etc
    i32 mip_count = 1;

    TextureDimension dimension = TextureDimension_2D;
    TextureFormat format = TextureFormat_RGBA8_UINT;
    TextureType type = TextureType_Texture;
    TextureUpdateType update = TextureUpdateType_Dynamic;
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

enum GpuCompareFunc : u32 {
    GpuCompareFunc_Never,
    GpuCompareFunc_Less,
    GpuCompareFunc_Equal,
    GpuCompareFunc_Less_equal,
    GpuCompareFunc_Greater,
    GpuCompareFunc_Not_equal,
    GpuCompareFunc_Greater_equal,
    GpuCompareFunc_Always,
    GpuCompareFunc_Count,
};
ENUMOPS(GpuCompareFunc);

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
    GpuCompareFunc compare_func = GpuCompareFunc_Always;
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
    template<typename T, u64 size>
    inline void Upload(const StaticArray<T, size>& a)
    {
        Upload(a.begin(), a.used, sizeof(T));
    }
};

bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, GpuBufferType type, GpuBufferFlag flags);
void DeleteBuffer(GpuBuffer** buffer);





//========================
//       GpuBinding
//========================

//struct GpuBinding
//{
//    std::string name;
//
//    void BindVertex(const GpuBuffer* buffer, const i32 slot);
//    void BindIndex(const GpuBuffer* buffer);
//    void BindView(GpuBuffer* view);
//    void BindSampler(GpuBuffer* sampler);
//    void Apply();
//};
//bool CreateGpuBinding(GpuBinding** binding, const char* name);
//void DeleteBuffer(GpuBinding** binding);





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

struct ShaderFile
{
    Path filepath;
    const char* text;
    u64 last_updated = {};
};

enum ShaderConstantType : u32 {
    ShaderConstant_Invalid,
    ShaderConstant_Float,
    ShaderConstant_Float2,
    ShaderConstant_Float3,
    ShaderConstant_Float4,
    ShaderConstant_Int,
    ShaderConstant_Int2,
    ShaderConstant_Int3,
    ShaderConstant_Int4,
    ShaderConstant_Mat4,
    ShaderConstant_Count,
};

//The memory MUST be 4 byte Aligned and/or follow STD140 normal gpu memory alignment
struct ShaderConstant {
    ShaderConstantType type;
    u32 array_count;            // 0 or 1 for scalars, >1 for arrays
    const char* name;// glsl name binding is required on GL 4.1 and WebGL2
};

struct ShaderConstantsContainer {
    u32 size = 0;
    u8 slot = 0;
    ShaderConstant constants[MAX_SHADER_UNIFORM_MEMBERS];
};

struct ShaderTexture {
    Texture* texture;
    Sampler* sampler;
    const char* name; //This must match OpenGL and WebGL shader name
};

struct ShaderParams
{
    ShaderFile vertex;
    ShaderFile pixel;
    //ShaderFile compute;
    VertexID vertex_layout = {};
    ShaderConstantsContainer constants_container[MAX_SHADER_UNIFORMS];
    ShaderTexture textures[MAX_SHADER_TEXTURES] = {};

    //Path vertex_file;
    //Path pixel_file;
    //Path compute_file;
    ////
    //const char* vertex_text = nullptr;
    //const char* pixel_text = nullptr;
    //const char* compute_text = nullptr;

    std::vector<ShaderMacro> macros;
    u32 vertex_component_count = 0;
    u32 input_stride_bytes = 0;
    std::vector<std::string> reference_file_names;
    std::vector<u64>         reference_file_times;
};

struct Shader
{
    //static const u32 vertex_component_max = 4;

    //~Shader();
    //void CheckForUpdate();

    std::string name;
    ShaderParams params;

    //bool CompileShader(std::string text, const std::string& file_name, ShaderType shader_type, std::string entry_name = "");
    //bool CompileShader(std::string text, const std::string& file_name, ShaderType shader_type);
};
struct sg_shader_desc;
bool CreateShader(Shader** shader, const char* name, const sg_shader_desc* shader_desc, VertexID vertex_layout);
//bool CreateShader(Shader** shader, const char* name, const ShaderParams& params);
void DeleteShader(Shader** shader);





//========================
//       Pipeline
//========================

enum StencilOp : u32 {
    StencilOp_Keep,
    StencilOp_Zero,
    StencilOp_Replace,
    StencilOp_IncrClamp,
    StencilOp_DecrClamp,
    StencilOp_Invert,
    StencilOp_IncrWrap,
    StencilOp_DecrWrap,
    StencilOp_Count,
};
ENUMOPS_PURE(StencilOp);

enum BlendFactor : u32 {
    BlendFactor_Zero,
    BlendFactor_One,
    BlendFactor_SrcColor,
    BlendFactor_OneMinusSrcColor,
    BlendFactor_SrcAlpha,
    BlendFactor_OneMinusSrcAlpha,
    BlendFactor_DstColor,
    BlendFactor_OneMinusDstColor,
    BlendFactor_DstAlpha,
    BlendFactor_OneMinusDstAlpha,
    BlendFactor_SrcAlphaSaturated,
    BlendFactor_BlendColor,
    BlendFactor_OneMinusBlendColor,
    BlendFactor_BlendAlpha,
    BlendFactor_OneMinusBlendAlpha,
    BlendFactor_Src1Color,
    BlendFactor_OneMinusSrc1Color,
    BlendFactor_Src1Alpha,
    BlendFactor_OneMinusSrc1Alpha,
    BlendFactor_Count,
};
ENUMOPS_PURE(BlendFactor);

enum BlendOp : u32 {
    BlendOp_Add,
    BlendOp_Subtract,
    BlendOp_ReverseSubtract,
    BlendOp_Min,
    BlendOp_Max,
    BlendOp_Count,
};
ENUMOPS_PURE(BlendOp);

enum PrimitiveType : u32 {
    PrimitiveType_Points,
    PrimitiveType_Lines,
    PrimitiveType_LineStrip,
    PrimitiveType_Triangles,
    PrimitiveType_TriangleStrip,
    PrimitveType_Count,
};
ENUMOPS_PURE(PrimitiveType);

enum RenderCullMode : u32 {
    RenderCullMode_None,
    RenderCullMode_Front,
    RenderCullMode_Back,
    RenderCullMode_Count,
};
ENUMOPS_PURE(RenderCullMode);

struct BlendState {
    bool enabled = true;
    BlendFactor src_factor_rgb = BlendFactor_SrcAlpha;
    BlendFactor dst_factor_rgb = BlendFactor_OneMinusSrcAlpha;
    BlendOp op_rgb      = BlendOp_Add;
    BlendFactor src_factor_alpha = BlendFactor_One;
    BlendFactor dst_factor_alpha = BlendFactor_OneMinusSrcAlpha;
    BlendOp op_alpha    = BlendOp_Add;
};

struct RenderTarget {
    Texture* texture;
    BlendState blend;
};

struct StencilOpParams {
    GpuCompareFunc compare = GpuCompareFunc_Always;
    StencilOp fail_op = StencilOp_Keep;
    StencilOp depth_fail_op;
    StencilOp pass_op;
};

struct StencilState {
    bool enabled = false;
    StencilOpParams front_face = { .depth_fail_op = StencilOp_Replace, .pass_op = StencilOp_Replace };
    StencilOpParams back_face  = { .depth_fail_op = StencilOp_Keep,    .pass_op = StencilOp_Keep    };
    u8 read_mask    = 0;
    u8 write_mask   = 0;
    u8 ref_value    = 0;
};

struct PipelineParams {
    Shader* shader;
    PrimitiveType primitive_type;
    RenderCullMode cull_mode;
    i32 msaa_sample_count = 1;

    bool has_index_buffer;
    bool front_ccw_winding_order = true;
    bool alpha_to_coverage_enabled = false;

    //Textures
    RenderTarget targets[MAX_SHADER_TEXTURES] = {};
    
    //Depth
    Texture* depth;
    GpuCompareFunc depth_compare_func;
    float depth_bias = 0.0f;
    float depth_bias_slope_scale = 0.0f;
    float depth_bias_clamp = 0.0f;

    //Stencil
    //example of how the stencil draw works:
    // if (ref [COMPARE_FUNCTION] buffer_value) { draw pixel }
    StencilState stencil;
};

struct Pipeline {
    std::string name;
    PipelineParams params;
};

bool CreatePipeline(Pipeline** pipeline, const char* name, const PipelineParams& params);
void DeletePipeline(Pipeline** pipeline);







//========================
//       Draw Call
//========================

struct RenderColorAction {
    bool clear_on_load = true;
    Color clear_color = { };
};
struct RenderDepthAction
{
    bool clear_on_load = true;
    float clear_value = 1.0f;
};
struct RenderStencilAction
{
    bool clear_on_load = true;
    u8 clear_value = 0;
};
struct ShaderUniformData {
    i32 slot;
    ArrayView<u8> struct_data;
};

struct Bindings {
    GpuBuffer* vertex_buffer = nullptr;
    GpuBuffer* index_buffer = nullptr;
    StaticArray<Texture*, MAX_SHADER_READ_TEXTURES> read_textures = {};
    StaticArray<Sampler*, MAX_SHADER_SAMPLERS> samplers = {};
};

DATAID_TYPE(DrawID);
struct DrawCallParams {
    Pipeline* pipeline;
    Bindings bindings = {};
    i32 vertex_index = 0;
    i32 vertex_length = 0;
    SimpleRect scissor = {};

    RenderColorAction color_actions[MAX_SHADER_TEXTURES] = {};
    RenderDepthAction depth_action;
    RenderStencilAction stencil_action;

    ShaderUniformData uniforms[MAX_SHADER_UNIFORMS];

    Texture* color_targets[MAX_COLOR_ATTACHMENTS] = {};
    Texture* depth_stencil_target;
    bool draw_to_backbuffer;
    //sg_view resolves[SG_MAX_COLOR_ATTACHMENTS];
};
struct DrawCall {
    DrawID data_id;
    std::string name;
    DrawCallParams params;
//struct CashDrawCall
//{
//    RenderType renderType;
//    Rectangle sRect = {};
//    Rectangle dRect = {};
//    Rectangle scissor = {};
//    RenderPrio prio;
//    ShaderProgram shader = ShaderProgram::Sprite;
//    uint32 prioIndex;
//    Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
//    CoordinateSpace coordSpace;
//    int32 vertexIndex;
//    int32 vertexLength;
//    TextureRenderUnion texture; // Union?
//};
//
//struct GeneralRenderParams {
//    std::string name;
//    DX11GpuBuffer* v_buffer = nullptr;
//    DX11GpuBuffer* i_buffer = nullptr;
//    DX11GpuBuffer* instance_buffer = nullptr;
//    DX11GpuBuffer* test_buffer = nullptr;
//    ShaderIndex vertex_shader = ShaderIndex_Invalid;
//    ShaderIndex pixel_shader = ShaderIndex_Invalid;
//    Rasterizer rasterizer = Rasterizer_SolidBackCull;
//    DX11Texture* texture = nullptr;
//    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    ID3D11DepthStencilState* depth_state = s_dx11.depth_state_depth;
//    ID3D11RenderTargetView* target_rtv = s_dx11.rtv_hdr;
//    bool rendering_shadows;
//};
};
//DrawCall& AllocDrawCall();
bool CreateDrawCall(const char* name, const DrawCallParams& params);







//========================
//       Renderer
//========================

struct Renderer
{
    SDL_Window* window;
    SDL_Renderer* context;
    Vec2I screen_size;
    Vec2I window_size;
    CashRenderBackend backend;

    Shader* blit2d_shader = {};
    BlendState blend_normal = {};
    BlendState blend_no_color_write = {};

    StencilState stencil_2d = {};
    Texture* hdr_target;
    Texture* plain_texture; //1px x 1px White

	VertexID vertex_2d_layout = {};
    VertexID vertex_pntc_layout = {};
    GpuBuffer* vertex_2d_verts = {};

    Sampler* common_anisotropic_sampler;
    Sampler* common_sampler;
};
extern Renderer gfx;

