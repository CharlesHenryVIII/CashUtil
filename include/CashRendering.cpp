#include "CashRendering.h"
#include "CashDebug.h"
#include "CashSystem.h"
#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "ImGui/backends/imgui_impl_sdl3.h"
#include "ImGui/backends/imgui_impl_sdlrenderer3.h"
#include "sokol/sokol_gfx.h"

Renderer gfx;

//typedef std::array<sg_vertex_attr_state, SG_MAX_VERTEX_ATTRIBUTES> SokolVertexLayout;
//typedef sg_vertex_attr_state SokolVertexLayout[SG_MAX_VERTEX_ATTRIBUTES];
//using SokolVertexLayout = sg_vertex_attr_state[SG_MAX_VERTEX_ATTRIBUTES];
using SokolVertexLayout = std::array<sg_vertex_attr_state, SG_MAX_VERTEX_ATTRIBUTES>;

struct GfxDevice {
    SokolVertexLayout vertex_layouts[VertexType_Count]= {};
    sg_buffer fullscreen_verts = {};
};
static GfxDevice s_gfx;


//void SysGetRenderEnvironment(sg_environment* env);

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
    SysRenderInitDesc sys_desc;
    bool result = SysRenderInit(&sys_desc);
    sg_desc desc = { };
    //desc.environment.defaults.color_format = SG_PIXELFORMAT_RGBA8;
    //desc.environment.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    SysGetRenderEnvironment(&desc.environment);
    sg_allocator alloc;
    sg_logger logger;
    //TODO(CSH): override allocator with custom frame temp memory?
    desc.allocator;     // optional memory allocation hooks.  Default is malloc and free
    desc.logger = { SgLogFunc, nullptr };
    sg_setup(&desc);



    static_assert(VertexType_Count == 2); //NOTE(CSH): If this is wrong then you will need to update the vertex layout descriptions here:
    s_gfx.vertex_layouts[VertexType_2D][0] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, position),  .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][1] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, uv),        .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][2] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, color),     .format = SG_VERTEXFORMAT_UINT };

#error NOT DONE
    s_gfx.vertex_layouts[VertexType_2D][0] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, position),  .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][1] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, uv),        .format = SG_VERTEXFORMAT_FLOAT2 };
    s_gfx.vertex_layouts[VertexType_2D][2] = { .buffer_index = 0, .offset = offsetof(Vertex_2D, color),     .format = SG_VERTEXFORMAT_UINT };

        //Vertex_PNTC
        s_gfx.vert_layouts[VertexType_PNTC]
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float, true)
            .end();
        bgfx::createVertexLayout(s_gfx.vert_layouts[VertexType_PNTC]);
    {
        Vertex_2D verts[] = {
            { { -1.0f, +1.0f }, {}, ToColorI(White) }, // Top Left
            { { -1.0f, -3.0f }, {}, ToColorI(White) }, // Bot Left
            { { +3.0f, +1.0f }, {}, ToColorI(White) } // Top Right
        };

        sg_buffer_desc d = {};
        d.size = sizeof(verts);
        d.data = SG_RANGE(verts);
        d.usage.vertex_buffer = true;
        d.usage.immutable = true;
        d.label = "Fullscreen Triangle VB";
        s_gfx.fullscreen_verts = sg_make_buffer(d);
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


}
void RenderDestroySokol()
{
    sg_shutdown();
    SysRenderDestroy();
}


bool RenderInitBgfx()
{
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

#ifdef CASH_SOKOL_RENDER
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





//========================
//       GpuBuffer
//========================

    //s_gfx.fullscreen_verts = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);

struct GfxGpuBuffer : public GpuBuffer
{
    union Handle {
        bgfx::DynamicIndexBufferHandle  dynamic_index_buffer;
        bgfx::DynamicVertexBufferHandle dynamic_vertex_buffer;
        //bgfx::FrameBufferHandle         frame_buffer;
        bgfx::IndexBufferHandle         index_buffer;
        bgfx::IndirectBufferHandle      indirect_buffer;
        //bgfx::OcclusionQueryHandle      occlusion_query;
        //bgfx::ProgramHandle             program;
        //bgfx::ShaderHandle              shader;
        //bgfx::TextureHandle             texture;
        //bgfx::UniformHandle             uniform;
        bgfx::VertexBufferHandle        vertex_buffer;
        bgfx::VertexLayoutHandle        vertex_layout;
    }handle;



    //D3D11_USAGE m_usage = D3D11_USAGE_DYNAMIC;
    ID3D11Buffer* buffer = nullptr;
    ID3D11ShaderResourceView* structure_resource_view = nullptr;
    ID3D11UnorderedAccessView* unordered_access_view = nullptr;
    //D3D11_BIND_FLAG m_target = {};
};

void GpuBuffer::Upload(const void* data, const size_t in_count, const u32 in_element_size, const bool is_byte_format)
{
    GfxGpuBuffer* buf = reinterpret_cast<GfxGpuBuffer*>(this);
    count = in_count;
    element_size = in_element_size;
    SafeRelease(buf->buffer);
    VALIDATE(element_size);
    VALIDATE(buf->type != GpuBufferType_Invalid);
    VALIDATE(in_count);
    UINT total_bytes = UINT(element_size * in_count);
    //ASSERT(total_bytes / 16 == 0);
    UINT buffer_type = 0;
    UINT cpu_access_flags = 0;
    UINT struct_byte_stride = 0;
    UINT memory_pitch = 0;
    UINT misc_flags = 0;
    UINT uav_flags = 0;
    D3D11_USAGE usage_flag = buf->is_dymamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    bool create_srv = false;
    bool create_uav = false;




    switch (buf->type)
    {
    case GpuBufferType_Vertex:
        buf->handle.vertex_buffer           = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
        buf->handle.dynamic_vertex_buffer   = bgfx::createDynamicVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
    case GpuBufferType_Index:
        buf->handle.index_buffer            = bgfx::createIndexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
        buf->handle.dynamic_index_buffer    = bgfx::createDynamicIndexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
    case GpuBufferType_Constant:
    //NOTE(CSH): NO CONSTANT BUFFER OF GENERIC TYPE!!??
        buf->handle.uniform = bgfx::createUniform(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
    case GpuBufferType_Structure:
        buf->handle.vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
        buf->handle.vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
        buf->handle.vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
        buf->handle.vertex_buffer = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), s_gfx.vert_layouts[VertexType_2D], BGFX_BUFFER_NONE);
    case GpuBufferType_RWStructure:
    case GpuBufferType_AppendStructure:
    case GpuBufferType_IndirectArgs:
    default: FAIL; return false;
    }






    switch (buf->type)
    {
    case GpuBufferType_Vertex:
        buffer_type = D3D11_BIND_VERTEX_BUFFER;
        break;
    case GpuBufferType_Index:
        buffer_type = D3D11_BIND_INDEX_BUFFER;
        break;
    case GpuBufferType_Constant:
        buffer_type = D3D11_BIND_CONSTANT_BUFFER;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        break;
    case GpuBufferType_Structure:
        create_srv = true;
        format = DXGI_FORMAT_UNKNOWN;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_type = D3D11_BIND_SHADER_RESOURCE;
        ASSERT(!buf->is_dymamic);
        struct_byte_stride = (UINT)element_size;
        break;
    case GpuBufferType_RWStructure:
        create_srv = true;
        create_uav = true;
        format = DXGI_FORMAT_UNKNOWN;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_type = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        ASSERT(!buf->is_dymamic);
        struct_byte_stride = (UINT)element_size;
        break;
    case GpuBufferType_AppendStructure:
        create_srv = true;
        create_uav = true;
        format = DXGI_FORMAT_UNKNOWN;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_type = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        ASSERT(!buf->is_dymamic);
        struct_byte_stride = (UINT)element_size;
        uav_flags = D3D11_BUFFER_UAV_FLAG_APPEND;
        break;
    case GpuBufferType_IndirectArgs:
        create_uav = true;
        buffer_type = D3D11_BIND_UNORDERED_ACCESS;
        misc_flags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS | D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        uav_flags |= D3D11_BUFFER_UAV_FLAG_RAW;
        format = DXGI_FORMAT_R32_TYPELESS;
        break;
    default:
        FAIL;
    }

    if (!buf->buffer)
    {
        {
            D3D11_BUFFER_DESC desc;
            desc.ByteWidth = total_bytes;
            desc.Usage = usage_flag;
            desc.BindFlags = buffer_type;
            desc.CPUAccessFlags = buf->is_dymamic ? D3D11_CPU_ACCESS_WRITE | cpu_access_flags : cpu_access_flags;
            desc.MiscFlags = misc_flags;
            desc.StructureByteStride = struct_byte_stride;

            D3D11_SUBRESOURCE_DATA dx11_data;
            dx11_data.pSysMem = data;
            dx11_data.SysMemPitch = memory_pitch;
            dx11_data.SysMemSlicePitch = 0;

			HR(s_dx11.device->CreateBuffer(
				&desc,                                        //[in]            const D3D11_BUFFER_DESC * pDesc,
				(data == nullptr) ? nullptr : &dx11_data,     //[in, optional]  const D3D11_SUBRESOURCE_DATA * pInitialData,
				&buf->buffer                                  //[out, optional] ID3D11Buffer * *ppBuffer
			));
            SetResourceName(buf->buffer, buf->name);
        }
        DEBUG_LOG("Created and Uploaded data to gpu buffer: element: %i size: %i", element_size, in_count);

        if (create_srv)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
            desc.Format = format;
            desc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)in_count;
            HR(s_dx11.device->CreateShaderResourceView(
                buf->buffer,                    //[in]            ID3D11Resource * pResource,
                &desc,                          //[in, optional]  const D3D11_SHADER_RESOURCE_VIEW_DESC * pDesc,
                &buf->structure_resource_view   //[out, optional] ID3D11ShaderResourceView * *ppSRView
            ));
        }

        if (create_uav)
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC));
            desc.Format = format;
            desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)in_count;
            desc.Buffer.Flags = uav_flags;
            HR(s_dx11.device->CreateUnorderedAccessView(
                buf->buffer,
                &desc,
                &buf->unordered_access_view
            ));
        }

        return;
    }

    if (buf->is_dymamic)
    {
        //map/unmap/memcopy
        D3D11_MAPPED_SUBRESOURCE resource;
        ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
        HR(s_dx11.device_context->Map(
            buf->buffer,          //[in]            ID3D11Resource * pResource,
            0,                      //[in]            UINT                     Subresource,
            D3D11_MAP_WRITE_DISCARD,//[in]            D3D11_MAP                MapType,
            0,                      //[in]            UINT                     MapFlags,
            &resource               //[out, optional] D3D11_MAPPED_SUBRESOURCE * pMappedResource
        ));
        memmove(resource.pData, data, element_size * in_count);
        s_dx11.device_context->Unmap(buf->buffer, 0);
        DEBUG_LOG("Uploaded dynamic_buffer data to gpu buffer: element: %i size: %i", element_size, in_count);
    }
    else
    {
        s_dx11.device_context->UpdateSubresource(
            buf->buffer,  //[in]           ID3D11Resource * pDstResource,
            0,              //[in]           UINT            DstSubresource,
            NULL,           //[in, optional] const D3D11_BOX * pDstBox,
            data,           //[in]           const void* pSrcData,
            total_bytes,    //[in]           UINT            SrcRowPitch,
            0               //[in]           UINT            SrcDepthPitch
        );
        DEBUG_LOG("Uploaded default_buffer data to gpu buffer: element: %i size: %i", element_size, in_count);
    }
}

void GpuBuffer::Bind(u32 slot, GpuBufferBindLocation binding)
{
    DX11GpuBuffer* buf = reinterpret_cast<DX11GpuBuffer*>(this);
    switch (type)
    {
    case GpuBufferType_Constant:
    {
        switch (binding)
        {
        case GpuBufferBindLocation_Vertex:
            s_dx11.device_context->VSSetConstantBuffers(slot, 1, &buf->buffer);
            break;
        case GpuBufferBindLocation_Pixel:
            s_dx11.device_context->PSSetConstantBuffers(slot, 1, &buf->buffer);
            break;
        case GpuBufferBindLocation_All:
            s_dx11.device_context->VSSetConstantBuffers(slot, 1, &buf->buffer);
            s_dx11.device_context->PSSetConstantBuffers(slot, 1, &buf->buffer);
            break;
        default:
            FAIL;
            break;
        }
        break;
    }
    case GpuBufferType_Structure:
    {
        switch (binding)
        {
        case GpuBufferBindLocation_Vertex:
            s_dx11.device_context->VSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        case GpuBufferBindLocation_Pixel:
            s_dx11.device_context->PSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        case GpuBufferBindLocation_All:
            s_dx11.device_context->VSSetShaderResources(slot, 1, &buf->structure_resource_view);
            s_dx11.device_context->PSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        case GpuBufferBindLocation_Compute:
			s_dx11.device_context->CSSetShaderResources(slot, 1, &buf->structure_resource_view);
			s_dx11.device_context->CSSetUnorderedAccessViews(slot, 1, &buf->unordered_access_view, nullptr);
            break;
        default:
            FAIL;
            break;
        }
        break;
    }
    default:
        FAIL;
    }
}

bool CreateGpuBuffer(GpuBuffer** buffer, const std::wstring& name, bool is_dynamic, GpuBufferType type)
{
    ASSERT(buffer);
    ASSERT(*buffer == nullptr);
    GfxGpuBuffer* buf = new GfxGpuBuffer;

    buf->is_dymamic = is_dynamic;
    buf->type = type;
    buf->name = name;
    (*buffer) = reinterpret_cast<GpuBuffer*>(buf);
    return true;
}

void DeleteBuffer(GpuBuffer** buffer)
{
    VALIDATE(buffer);
    DX11GpuBuffer* buf = reinterpret_cast<DX11GpuBuffer*>(*buffer);
    SafeRelease(buf->buffer);
    delete buf;
    DEBUG_LOG("GPU Buffer deleted %i, %i\n", m_target, m_handle);
}