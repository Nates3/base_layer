

#define D3D11_NO_HELPERS
#define CINTERFACE
#define COBJMACROS

#include <d3d11_1.h>
#include <d3dcompiler.h>

global_ ID3D11Device1 *g_device = 0;
global_ ID3D11DeviceContext1 *g_render_context = 0;
global_ IDXGISwapChain1 *g_swap_chain = 0;
global_ ID3D11RenderTargetView *g_render_target_view = 0;
global_ Arena *g_d3d_arena = 0;
global_ D3D11_TEXTURE2D_DESC g_depth_buffer_desc = {0};
global_ ID3D11DepthStencilView *g_depth_buffer_view = 0;
global_ ID3D11RasterizerState1 *g_raster_state_fill = 0;
global_ ID3D11RasterizerState1 *g_raster_state_wire = 0;

typedef struct Shader_Blob_Pair
{
	R_Handle shader;
	ID3DBlob *blob;
} Shader_Blob_Pair;

typedef struct Texture_Context
{
	ID3D11SamplerState* sampler_state;
	ID3D11DepthStencilState* depth_stencil_state;
	ID3D11BlendState *blend_state;
} Texture_Context;

func_ void
Gfx_Startup(OS_Handle window)
{
	g_d3d_arena = ArenaAlloc(GB(4));
	
	// nates: create a standard render context
	D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
	ID3D11Device *base_device;
	ID3D11DeviceContext *base_device_context;
	D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
										feature_levels, ArrayCount(feature_levels),
										D3D11_SDK_VERSION, &base_device, 0, &base_device_context);
	
	// nates: create a device1 render context
	ID3D11Device_QueryInterface(base_device, &IID_ID3D11Device1, (void**)(&g_device));
	ID3D11DeviceContext_QueryInterface(base_device_context, &IID_ID3D11DeviceContext1, (void**)(&g_render_context));
	
	// nates: create a directx graphics infrustructure oop stuff
	IDXGIDevice1* dxgi_device;
	ID3D11Device1_QueryInterface(g_device, &IID_IDXGIDevice1, (void**)(&dxgi_device));
	IDXGIAdapter* dxgi_adapter;
	IDXGIDevice1_GetAdapter(dxgi_device, &dxgi_adapter);
	IDXGIFactory2* dxgi_factory;
	IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (void**)(&dxgi_factory));
	
	// nates: use dxgi to create a swap chain and a frame buffer
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {
		.Width              = 0,
		.Height             = 0,
		.Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
		.Stereo             = false,
		.SampleDesc.Count   = 1,
		.SampleDesc.Quality = 0,
		.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT,
		.BufferCount        = 2,
		.Scaling            = DXGI_SCALING_STRETCH,
		.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD,
		.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED,
		.Flags              = 0,
	};
	IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown *)g_device, (HWND)(window), &swap_chain_desc, 0, 0, &g_swap_chain);
	
	// TODO(nates): how do we resize this?
	ID3D11Texture2D *frame_buffer;
	IDXGISwapChain1_GetBuffer(g_swap_chain, 0, &IID_ID3D11Texture2D, (void **)&frame_buffer);
	ID3D11Device1_CreateRenderTargetView(g_device, (ID3D11Resource *)frame_buffer, 0, &g_render_target_view);
	
	// nates: use the frame buffer to set up a depth buffer
	ID3D11Texture2D_GetDesc(frame_buffer, &g_depth_buffer_desc); // base on framebuffer properties
	g_depth_buffer_desc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	g_depth_buffer_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ID3D11Texture2D* depth_buffer;
	ID3D11Device1_CreateTexture2D(g_device, &g_depth_buffer_desc, 0, &depth_buffer);
	ID3D11Device1_CreateDepthStencilView(g_device, (ID3D11Resource *)depth_buffer, 0, &g_depth_buffer_view);
	
	// nates: make a rasterizer for filling
	{
		D3D11_RASTERIZER_DESC1 rasterizer_desc = {0};
		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.ScissorEnable = true;
		ID3D11Device1_CreateRasterizerState1(g_device, &rasterizer_desc, &g_raster_state_fill);
	}
	
	// nates: make a rasterizer for wireframe
	{
		D3D11_RASTERIZER_DESC1 rasterizer_desc = {0};
		rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;
		rasterizer_desc.CullMode = D3D11_CULL_BACK;
		rasterizer_desc.ScissorEnable = true;
		ID3D11Device1_CreateRasterizerState1(g_device, &rasterizer_desc, &g_raster_state_wire);
	}
}

func_ void Gfx_ResizeRenderTarget(U32 new_width, U32 new_height)
{
	Unimplemented;
	if(g_swap_chain) {
		IDXGISwapChain1_ResizeBuffers(g_swap_chain, 2, new_width, new_height, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 0);
	}
}

func_ R_Buffer 
Gfx_MakeDynamicVBuffer(U64 buffer_size)
{
	D3D11_BUFFER_DESC vertex_buffer_desc = {0};
	vertex_buffer_desc.ByteWidth = buffer_size;
	vertex_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	vertex_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
	vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ID3D11Buffer *vertex_buffer;
	ID3D11Device1_CreateBuffer(g_device, &vertex_buffer_desc, 0, &vertex_buffer);
	
	R_Buffer result = {
		.handle = (R_Handle)vertex_buffer,
		.size = buffer_size,
	};
	return(result);
}

func_ R_Buffer 
Gfx_MakeDynamicIBuffer(U64 buffer_size)
{
	D3D11_BUFFER_DESC index_buffer_desc = {0};
	index_buffer_desc.ByteWidth = buffer_size;
	index_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	index_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
	
	index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ID3D11Buffer *index_buffer;
	ID3D11Device1_CreateBuffer(g_device, &index_buffer_desc, 0, &index_buffer);
	
	R_Buffer result = {
		.handle = (R_Handle)index_buffer,
		.size = buffer_size,
	};
	return(result);
}


func_ R_Buffer
Gfx_MakeDynamicUBuffer(U64 buffer_size)
{
	D3D11_BUFFER_DESC constant_buffer_desc = {0};
	constant_buffer_desc.ByteWidth      = AlignUp(buffer_size, 16); // nates: maybe have to align size to 16 bytes
	constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	
	constant_buffer_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	ID3D11Buffer *constant_buffer;
	ID3D11Device1_CreateBuffer(g_device, &constant_buffer_desc, 0, &constant_buffer);
	R_Buffer result = {
		.handle = (R_Handle)constant_buffer,
		.size = buffer_size,
	};
	return(result);
}

func_ void 
Gfx_SetVertexBufferData(R_Buffer vbuffer, void *memory, U64 memory_size)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	ID3D11DeviceContext1_Map(g_render_context, (ID3D11Resource *)vbuffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	MemoryCopy((void *)subresource.pData, memory, memory_size);
	ID3D11DeviceContext1_Unmap(g_render_context, (ID3D11Resource *)vbuffer.handle, 0);
}

func_ void 
Gfx_SetIndexBufferData(R_Buffer ibuffer, void *memory, U64 memory_size)
{
	D3D11_MAPPED_SUBRESOURCE subresource;
	ID3D11DeviceContext1_Map(g_render_context, (ID3D11Resource *)ibuffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
	MemoryCopy((void *)subresource.pData, memory, memory_size);
	ID3D11DeviceContext1_Unmap(g_render_context, (ID3D11Resource *)ibuffer.handle, 0);
}

func_ void 
Gfx_SetUniformBufferData(R_Buffer ubuffer, void *memory, U64 memory_size)
{
	D3D11_MAPPED_SUBRESOURCE mapped_subresource;
	ID3D11DeviceContext1_Map(g_render_context, (ID3D11Resource *)ubuffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
	void *dest = (void *)mapped_subresource.pData;
	MemoryCopy(dest, memory, memory_size);
	ID3D11DeviceContext1_Unmap(g_render_context, (ID3D11Resource *)ubuffer.handle, 0);
}


func_ R_Shader
Gfx_MakeVertexShader(Str8 filename) 
{
	Shader_Blob_Pair *pair = PushArray(g_d3d_arena, Shader_Blob_Pair, 1);
	ArenaTemp scratch = GetScratch(0, 0);
	Str16 shader_filename16 = Str16FromStr8(scratch.arena, filename);
	ID3DBlob *error_blob = 0;
	HRESULT hr = D3DCompileFromFile((WCHAR *)shader_filename16.m, 0, 0, "vs_main", "vs_5_0", 0, 0, &pair->blob, &error_blob);
	if(FAILED(hr)) {
		InvalidPath;
	}
	
	if(error_blob) {
		void *error_message = ID3D10Blob_GetBufferPointer(error_blob);
		fprintf(stderr, error_message);
		OS_Abort();
	}
	
	ID3D11Device1_CreateVertexShader(g_device, ID3D10Blob_GetBufferPointer(pair->blob), ID3D10Blob_GetBufferSize(pair->blob), 0, (ID3D11VertexShader **)&pair->shader);
	ReleaseScratch(scratch);
	R_Shader result = (R_Shader)pair;
	return(result);
}

func_ R_Handle
Gfx_MakeFragmentShader(Str8 filename) 
{
	Shader_Blob_Pair *pair = PushArray(g_d3d_arena, Shader_Blob_Pair, 1);
	ArenaTemp scratch = GetScratch(0, 0);
	Str16 shader_filename16 = Str16FromStr8(scratch.arena, filename);
	ID3DBlob *error_blob = 0;
	HRESULT hr = D3DCompileFromFile((WCHAR *)shader_filename16.m, 0, 0, "ps_main", "ps_5_0", 0, 0, &pair->blob, 0);
	if(FAILED(hr)) {
		InvalidPath;
	}
	
	if(error_blob) {
		void *error_message = ID3D10Blob_GetBufferPointer(error_blob);
		fprintf(stderr, error_message);
		OS_Abort();
	}
	ID3D11Device1_CreatePixelShader(g_device, ID3D10Blob_GetBufferPointer(pair->blob), ID3D10Blob_GetBufferSize(pair->blob), 0, (ID3D11PixelShader **)&pair->shader);
	ReleaseScratch(scratch);
	R_Handle result = (R_Handle)pair;
	return(result);
}

func_ R_Handle
Gfx_MakeGeometryShader(Str8 filename)
{
	Shader_Blob_Pair *pair = PushArray(g_d3d_arena, Shader_Blob_Pair, 1);
	ArenaTemp scratch = GetScratch(0, 0);
	Str16 shader_filename16 = Str16FromStr8(scratch.arena, filename);
	ID3DBlob *error_blob = 0;
	HRESULT hr = D3DCompileFromFile((WCHAR *)shader_filename16.m, 0, 0, "gs_main", "gs_5_0", 0, 0, &pair->blob, 0);
	if(FAILED(hr)) {
		InvalidPath;
	}
	
	if(error_blob) {
		void *error_message = ID3D10Blob_GetBufferPointer(error_blob);
		fprintf(stderr, error_message);
		OS_Abort();
	}
	ID3D11Device1_CreateGeometryShader(g_device, ID3D10Blob_GetBufferPointer(pair->blob), ID3D10Blob_GetBufferSize(pair->blob), 0, (ID3D11GeometryShader **)&pair->shader);
	ReleaseScratch(scratch);
	R_Handle result = (R_Handle)pair;
	return(result);
}

func_ R_Layout
Gfx_MakeInputLayout_(R_Handle vertex_shader, R_LayoutElement *elements, U32 element_count)
{
	D3D11_INPUT_ELEMENT_DESC input_layout_desc[16] = {0};
	Assert(element_count <= ArrayCount(input_layout_desc));
	U32 offset = 0;
	for(U32 index = 0; index < element_count; ++index)
	{
		R_LayoutElement element = elements[index];
		switch(element) {
			case R_Layout_Element_Pos2: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_Pos2], 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 2*sizeof(F32);
			} break;
			case R_Layout_Element_Pos3: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_Pos3], 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 3*sizeof(F32);
			} break;
			case R_Layout_Element_Normal: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_Normal], 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 3*sizeof(F32);
			} break;
			case R_Layout_Element_TextureCoords: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_TextureCoords], 0, DXGI_FORMAT_R32G32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 2*sizeof(F32);
			} break;
			case R_Layout_Element_Color3: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_Color3], 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 3*sizeof(F32);
			} break;
			case R_Layout_Element_Color4: {
				input_layout_desc[index] = (D3D11_INPUT_ELEMENT_DESC){ g_layout_strings[R_Layout_Element_Color4], 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				offset += 4*sizeof(F32);
			} break;
		}
	}
	
	Shader_Blob_Pair *pair = (Shader_Blob_Pair *)vertex_shader;
	ID3D11InputLayout *input_layout = 0;
	ID3D11Device1_CreateInputLayout(g_device, input_layout_desc, element_count,
																	ID3D10Blob_GetBufferPointer(pair->blob),
																	ID3D10Blob_GetBufferSize(pair->blob), 
																	&input_layout);
	R_Layout result = {
		.handle = (R_Handle)input_layout,
		.stride = offset,
	};
	return(result);
}


func_ R_Handle 
Gfx_MakeFullViewport(void)
{
	D3D11_VIEWPORT *viewport = PushArray(g_d3d_arena, D3D11_VIEWPORT, 1);
	viewport->TopLeftX = 0;
	viewport->TopLeftY = 0;
	viewport->Width = g_depth_buffer_desc.Width;
	viewport->Height = g_depth_buffer_desc.Height;
	viewport->MinDepth = 0.f;
	viewport->MaxDepth = 1.f;
	R_Handle result = (R_Handle)viewport;
	return(result);
}

func_ R_Handle
Gfx_MakeViewport(R2 rect)
{
	D3D11_VIEWPORT *viewport = PushArray(g_d3d_arena, D3D11_VIEWPORT, 1);
	viewport->TopLeftX = rect.x0;
	viewport->TopLeftY = rect.y0;
	viewport->Width = rect.x1 - rect.x0;
	viewport->Height = rect.y1 - rect.y0;
	viewport->MinDepth = 0.f;
	viewport->MaxDepth = 1.f;
	R_Handle result = (R_Handle)viewport;
	return(result);
}

func_ R_Handle
Gfx_MakeTexture(void *data, U32 width, U32 height)
{
	// IMPORTANT TODO(nates): Sometimes the texture looks wrong
	
	D3D11_TEXTURE2D_DESC texture_desc = {0};
	texture_desc.Width              = width;  // in data.h
	texture_desc.Height             = height; // in data.h
	// TODO(nates): How do we abstract mip maps
	texture_desc.MipLevels          = 1;
	texture_desc.ArraySize          = 1;
	// TODO(nates): Support rgb format
	texture_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	texture_desc.SampleDesc.Count   = 1;
	// TODO(nates): Dynamic textures
	texture_desc.Usage              = D3D11_USAGE_IMMUTABLE;
	texture_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA texture_data = {0};
	texture_data.pSysMem            = data;
	//TODO(nates): support rgb format
	texture_data.SysMemPitch        = width*4;
	ID3D11Texture2D* texture;
	ID3D11Device1_CreateTexture2D(g_device, &texture_desc, &texture_data, &texture);
	ID3D11ShaderResourceView* texture_view;
	ID3D11Device1_CreateShaderResourceView(g_device, (ID3D11Resource *)texture, 0, &texture_view);
	
	R_Handle result = (R_Handle)texture_view;
	return(result);
}

func_ R_Handle
Gfx_MakeTextureContext(B32 blending)
{
	D3D11_SAMPLER_DESC sampler_desc = {0};
	sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ID3D11SamplerState* sampler_state;
	ID3D11Device1_CreateSamplerState(g_device, &sampler_desc, &sampler_state);
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	D3D11_DEPTH_STENCIL_DESC depth_stencil_desc = {0};
	depth_stencil_desc.DepthEnable    = TRUE;
	depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depth_stencil_desc.DepthFunc      = D3D11_COMPARISON_LESS;
	ID3D11DepthStencilState* depth_stencil_state;
	ID3D11Device1_CreateDepthStencilState(g_device, &depth_stencil_desc, &depth_stencil_state);
	
	ID3D11BlendState *blend_state = 0;
	D3D11_BLEND_DESC blend_desc = {0};
	blend_desc.RenderTarget[0] = (D3D11_RENDER_TARGET_BLEND_DESC){
		.BlendEnable = blending,
		.SrcBlend = D3D11_BLEND_SRC_ALPHA,
		.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
		.BlendOp = D3D11_BLEND_OP_ADD,
		.SrcBlendAlpha = D3D11_BLEND_ZERO,
		.DestBlendAlpha = D3D11_BLEND_ZERO,
		.BlendOpAlpha = D3D11_BLEND_OP_ADD,
		.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	};
	ID3D11Device1_CreateBlendState(g_device, &blend_desc, &blend_state);
	
	Texture_Context *tex_ctx = PushArray(g_d3d_arena, Texture_Context, 1);
	tex_ctx->sampler_state = sampler_state;
	tex_ctx->depth_stencil_state = depth_stencil_state;
	tex_ctx->blend_state = blend_state;
	
	R_Handle result = (R_Handle)tex_ctx;
	return(result);
}


func_ void
Gfx_BindViewport(R_Handle viewport)
{
	ID3D11DeviceContext_RSSetViewports(g_render_context, 1, (D3D11_VIEWPORT *)viewport);
}

func_ void
Gfx_SetRenderTarget(void)
{
	ID3D11DeviceContext_OMSetRenderTargets(g_render_context, 1, &g_render_target_view, g_depth_buffer_view);
}

func_ void 
Gfx_BindInputLayout(R_Layout layout)
{
	ID3D11DeviceContext_IASetInputLayout(g_render_context, (ID3D11InputLayout *)layout.handle);
}

func_ void
Gfx_BindVertexBuffer(R_Buffer vbuffer, U32 stride, U32 offset)
{
	ID3D11Buffer **buffer = (ID3D11Buffer **)&vbuffer.handle;
	ID3D11DeviceContext1_IASetVertexBuffers(g_render_context, 0, 1, buffer, &stride, &offset);
}

func_ void 
Gfx_BindIndexBuffer(R_Buffer ibuffer)
{
	ID3D11DeviceContext1_IASetIndexBuffer(g_render_context, (ID3D11Buffer *)ibuffer.handle, DXGI_FORMAT_R32_UINT, 0);
}

func_ void 
Gfx_BindUniformBuffer(R_Buffer ubuffer, R_Shader_kind kind)
{
	ID3D11Buffer **buffer = (ID3D11Buffer **)&ubuffer.handle;
	switch(kind) {
		default: { InvalidPath; } break;
		case R_Shader_Kind_Vertex: {
			ID3D11DeviceContext1_VSSetConstantBuffers(g_render_context, 0, 1, buffer);
		} break;
		case R_Shader_Kind_Fragment: {
			ID3D11DeviceContext1_PSSetConstantBuffers(g_render_context, 0, 1, buffer);
		} break;
		case R_Shader_Kind_Geometry: {
			ID3D11DeviceContext1_GSSetConstantBuffers(g_render_context, 0, 1, buffer);
		} break;
	}
}

func_ void
Gfx_BindVertexShader(R_Handle vertex_shader)
{
	Shader_Blob_Pair *pair = (Shader_Blob_Pair *)vertex_shader;
	ID3D11DeviceContext_VSSetShader(g_render_context, (ID3D11VertexShader *)pair->shader, 0, 0);
}

func_ void
Gfx_BindFragmentShader(R_Handle pixel_shader)
{
	Shader_Blob_Pair *pair = (Shader_Blob_Pair *)pixel_shader;
	ID3D11DeviceContext_PSSetShader(g_render_context, (ID3D11PixelShader *)pair->shader, 0, 0);
}

func_ void
Gfx_FillRenderTarget(V4 background_color)
{
	ID3D11DeviceContext1_ClearRenderTargetView(g_render_context, g_render_target_view, (F32 *)&background_color);
}

func_ void
Gfx_ClearDepthBuffer(void)
{
	ID3D11DeviceContext1_ClearDepthStencilView(g_render_context, g_depth_buffer_view, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

func_ void
Gfx_SetRenderMode(R_Mode mode)
{
	D3D_PRIMITIVE_TOPOLOGY topology = 0;
	switch(mode) {
		case R_Mode_Null: {
			topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
		case R_Mode_PointList: {
			topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		} break;
		case R_Mode_LineList: {
			topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		} break;
		case R_Mode_TriangleList: {
			topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		} break;
	};
	
	ID3D11DeviceContext_IASetPrimitiveTopology(g_render_context, topology);
}

func_ void
Gfx_DrawIndices(U32 index_count)
{
	// TODO(nates): We probably want to be able to pass the index offset and the vertex offset
	ID3D11DeviceContext1_DrawIndexed(g_render_context, index_count, 0, 0);
}

func_ void
Gfx_DrawVertices(U32 vertex_count)
{
	// TODO(nates): We probably want to be able to pass the start vertex location
	ID3D11DeviceContext1_Draw(g_render_context, vertex_count, 0);
}


func_ void
Gfx_SwapBuffers(void)
{
	IDXGISwapChain1_Present(g_swap_chain, 1, 0);
}