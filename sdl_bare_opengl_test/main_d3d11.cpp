#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>
//#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <comdef.h>

#include <dwmapi.h>
#include <limits>


// DirectX
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3d10.lib")
//#pragma comment (lib, "d3dx11.lib")
//#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "dxgi.lib")
//;
static bool InitD3D(HWND hWnd);
static void CleanD3D();
void RenderFrame(int mouse_x, int mouse_y);     // renders a single frame
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders

// global declarations
static IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
static ID3D11Device *dev;                     // the pointer to our Direct3D device interface
static ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
static ID3D11RenderTargetView *backbuffer;    // global declaration
static ID3D11VertexShader *pVS;    // the vertex shader
static ID3D11PixelShader *pPS;     // the pixel shader
static ID3D11Buffer *pVBuffer;    // global
static ID3D11InputLayout *pLayout;    // global
static ID3D11RasterizerState *rasterizerState;
static HANDLE frameLatencyWaitableObject;

typedef ID3D11Buffer *VBO;

static VBO rows[8];

const int num_quads = 20;
const int vertices_per_quad = 6;

static HWND get_hwnd(SDL_Window *window)
{
	SDL_SysWMinfo wmInfo = { 0 };
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	return hwnd;
}

struct D3DXCOLOR
{
	D3DXCOLOR()	{}
	D3DXCOLOR(FLOAT r, FLOAT g, FLOAT b, FLOAT a = 1.0f)
		: r(r), g(g), b(b), a(a) 
	{}
	FLOAT r, g, b, a;
};

struct VERTEX
{
	VERTEX() {}
	VERTEX(FLOAT x, FLOAT y, FLOAT z, D3DXCOLOR color)
		: X(x), Y(y), Z(z), Color(color)
	{}
	FLOAT X, Y, Z;      // position
	D3DXCOLOR Color;    // color
};

const int window_w = 800;
const int window_h = 600;

static VERTEX OurVertices[3];

int main_d3d11()
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("sdl init failed %s\n", SDL_GetError());
		//return 1;
	}

	SDL_Window *window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("window failed %s\n", SDL_GetError());
		return 1;
	}

	if (!InitD3D(get_hwnd(window)))
	{
		printf("d3d init failed\n");
		return 1;
	}





	printf("Init OK\r\n");

	int frame_counter = 0;

	Uint64 performance_frequency = SDL_GetPerformanceFrequency();
	Uint64 last_tick = SDL_GetPerformanceCounter();
	Uint64 performance_start = last_tick;

	
	bool held_down = false;

	int mouse_x = 0, mouse_y = 0;

	bool enable_dwm_flush = false;
	bool flash_background = false;
	bool wait_frame_latency_waitable_object = true;
	bool present = true;
	bool skip_next_wait = false;
	int record_next_wait = 0;
	bool wait_extra_next_time = false;

	while (1)
	{
		bool drop_a_few = false;
		bool flush = false;

		if (skip_next_wait)
		{
			skip_next_wait = false;
			record_next_wait = 5;
		}
		else if (wait_frame_latency_waitable_object && frameLatencyWaitableObject != NULL)
		{
			int bleh = 0;

			Uint64 sw = SDL_GetPerformanceCounter();
			DWORD result = WaitForSingleObjectEx(
				frameLatencyWaitableObject,
				1000, // 1 second timeout (shouldn't ever occur)
				true
			);
			if (wait_extra_next_time)
			{
				wait_extra_next_time = false;
				Uint64 sw = SDL_GetPerformanceCounter();
				DWORD result = WaitForSingleObjectEx(
					frameLatencyWaitableObject,
					1000, // 1 second timeout (shouldn't ever occur)
					true
				);
				record_next_wait = 5;
			}
			//vsyncThread(NULL);
			Uint64 after = SDL_GetPerformanceCounter();

			while (WaitForSingleObjectEx(frameLatencyWaitableObject, 0, true) == WAIT_OBJECT_0)
			{
				++bleh;
			}
			if (bleh > 0)
			{
				DWORD result = WaitForSingleObjectEx(
					frameLatencyWaitableObject,
					1000, // 1 second timeout (shouldn't ever occur)
					true
				);
			}

			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			//double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			if (record_next_wait > 0)
			{
				--record_next_wait;
				printf("WaitForSingleObjectEx call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
				//printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
			if (bleh > 0)
			{
				printf("BLEH: %d\n", bleh);
			}
		}

		mouse_x = (frame_counter * 8) % window_w;
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)
		{
			SDL_WarpMouseInWindow(window, mouse_x, window_h / 2);
		}

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				goto _quit;
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				held_down = true;
				SDL_CaptureMouse(SDL_TRUE);
			}
			else if (event.type == SDL_MOUSEBUTTONUP)
			{
				held_down = false;
				SDL_CaptureMouse(SDL_FALSE);
			}
			else if (event.type == SDL_MOUSEMOTION)
			{
				if (held_down)
				{
					//printf("motion %d %d\r\n", event.motion.x, event.motion.y);
				}
				int mouse_x_tmp = event.motion.x;
				if (mouse_x_tmp != mouse_x)
				{
					//printf("mouse weird; expected %d, actual %d\n", mouse_x, mouse_x_tmp);
				}
				else
				{
					//printf("mouse good\n");
				}
				mouse_y = event.motion.y;
			}
			else if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.scancode == SDL_SCANCODE_Q)
				{
					goto _quit;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_X)
				{
					skip_next_wait = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_E)
				{
					wait_extra_next_time = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_L)
				{
					flush = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					enable_dwm_flush = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					enable_dwm_flush = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					wait_frame_latency_waitable_object = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					wait_frame_latency_waitable_object = false;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_D)
				{
					drop_a_few = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					present = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_F6)
				{
					present = false;
				}
			}
		}

		

		
		++frame_counter;
		//printf("frame: %d\r\n", frame_counter);

		double frame_counter_float;
		Uint64 milli_frame_counter;

		{
			Uint64 tmp = last_tick;
			last_tick = SDL_GetPerformanceCounter();
			Uint64 elapsed = last_tick - tmp;
			double elapsed_ms = (double)elapsed * 1000.0 / (double)performance_frequency;
			if (elapsed_ms < 15.0 || elapsed_ms > 16.8)
			{
				printf("A %d : %.3lf\r\n", frame_counter, elapsed_ms);
			}

			Uint64 total_elapsed_ms = ((last_tick - performance_start) * 1000ULL + performance_frequency / 2) / performance_frequency;
#define MONITOR_REFRESH_RATE_HZ 60
			//frame_counter = (total_elapsed_ms * MONITOR_REFRESH_RATE_HZ + 500) / 1000;

			Uint64 total_elapsed_us = ((last_tick - performance_start) * 1000000ULL + performance_frequency / 2) / performance_frequency;
			milli_frame_counter = (total_elapsed_us * MONITOR_REFRESH_RATE_HZ + 500) / 1000;
			frame_counter_float = (double)milli_frame_counter / 1000.0;
		}


		int fc_mod4 = frame_counter % 4;
		// "Divide-remainder" by the number of colored rects you want
		int fc_mod8 = frame_counter % 8;


#if 1

		// set the render target as the back buffer
		devcon->OMSetRenderTargets(1, &backbuffer, NULL);

		// set the shader objects
		devcon->VSSetShader(pVS, 0, 0);
		devcon->PSSetShader(pPS, 0, 0);


		if (!flash_background || frame_counter & 1)
		{
			float color[4] = { 60.0f / 255.0f, 50.0f / 255.0f, 75.0f / 255.0f, 0.0f };
			devcon->ClearRenderTargetView(backbuffer, color);
		}
		else
		{
			float color[4] = { 0, 0, 0, 0 };
			devcon->ClearRenderTargetView(backbuffer, color);
		}


		// Flashing rectangle row
		{
			int row_index = frame_counter % ARRAYSIZE(rows);

			// select which vertex buffer to display
			UINT stride = sizeof(VERTEX);
			UINT offset = 0;
			devcon->IASetVertexBuffers(0, 1, &rows[row_index], &stride, &offset);

			// select which primtive type we are using
			devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// draw the vertex buffer to the back buffer
			devcon->Draw(num_quads * vertices_per_quad, 0);
		}

		


		// mouse "cursor"
		OurVertices[0] = VERTEX((float)mouse_x, (float)mouse_y, 0.0f, D3DXCOLOR(1, 1, 0, 1));
		OurVertices[1] = VERTEX((float)mouse_x, (float)mouse_y + 24.0f, 0.0f, D3DXCOLOR(1, 1, 0, 1));
		OurVertices[2] = VERTEX((float)mouse_x + 24.0f, (float)mouse_y, 0.0f, D3DXCOLOR(1, 1, 0, 1));

		// copy the vertices into the buffer
		D3D11_MAPPED_SUBRESOURCE ms;
		devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
		memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
		devcon->Unmap(pVBuffer, NULL);                                      // unmap the buffer

		{
			// select which vertex buffer to display
			UINT stride = sizeof(VERTEX);
			UINT offset = 0;
			devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

			// select which primtive type we are using
			devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// draw the vertex buffer to the back buffer
			devcon->Draw(3, 0);
		}

		if (present)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			swapchain->Present(1, 0);
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			//double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			//if (frame_counter < 60)
			{
				//printf("swap call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
			}
			if (frame_counter < 60)
			{
				//printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
		}
		if (enable_dwm_flush)
		{
			Uint64 sw = SDL_GetPerformanceCounter();
			DwmFlush();
			//vsyncThread(NULL);
			Uint64 after = SDL_GetPerformanceCounter();
			double elapsed_ms = (double)(after - sw) * 1000.0 / (double)performance_frequency;
			//double delay_to_vsync = (double)(after - last_vsync) * 1000.0 / (double)performance_frequency;
			//if (frame_counter < 6000)
			{
				//printf("DwmFlush call: %d, %.3lf\r\n", frame_counter, elapsed_ms);
				//printf("delay to vsync: %d, %.4lf\r\n", frame_counter, delay_to_vsync);
			}
		}

		if (drop_a_few)
		{
			Sleep(100);
		}

		if (flush)
		{
			devcon->Flush();
		}
#endif
	}

_quit:

	CleanD3D();
	SDL_DestroyWindow(window);
	SDL_Quit();

	getc(stdin);

	return 0;
}

// this function initializes and prepares Direct3D for use
bool InitD3D(HWND hWnd)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 2;                                   // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
	scd.BufferDesc.Width = window_w;                   // set the back buffer width
	scd.BufferDesc.Height = window_h;                 // set the back buffer height
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
	scd.OutputWindow = hWnd;                               // the window to be used
	scd.SampleDesc.Count = 1;                              // how many multisamples
	scd.Windowed = TRUE;                                   // windowed/full-screen mode
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
	// If the project is in a debug build, enable the debug layer.
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// Define the ordering of feature levels that Direct3D attempts to create.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_1
	};

	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags,
		featureLevels,
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,
		&dev,
		nullptr,
		&devcon
	);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}

	IDXGIDevice1 *dev1;
	hr = dev->QueryInterface(__uuidof(IDXGIDevice1), (void **)&dev1);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}
	UINT retrieved;
	dev1->GetMaximumFrameLatency(&retrieved);
	printf("retrieved: %d\n", retrieved);
	hr = dev1->SetMaximumFrameLatency(5);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}
	dev1->Release();

	IDXGIFactory *factory;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}

	hr = factory->CreateSwapChain(
		dev,
		&scd,
		&swapchain
	);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}
	factory->Release();

	IDXGISwapChain2 *sc2;
	hr = swapchain->QueryInterface(__uuidof(IDXGISwapChain2), (void **)&sc2);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}

	hr = sc2->SetMaximumFrameLatency(1);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}

	frameLatencyWaitableObject = sc2->GetFrameLatencyWaitableObject();
	if (frameLatencyWaitableObject == NULL)
	{
		MessageBox(NULL, L"frame latency waitable object is null", NULL, MB_OK);
		return false;
	}
	sc2->Release();


	// get the address of the back buffer
	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// use the back buffer address to create the render target
	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	pBackBuffer->Release();


	// Set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = window_w;
	viewport.Height = window_h;

	devcon->RSSetViewports(1, &viewport);

	D3D11_RASTERIZER_DESC rd;
	memset(&rd, 0, sizeof(rd));
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	rd.DepthClipEnable = TRUE;
	
	hr = dev->CreateRasterizerState(&rd, &rasterizerState);
	if (FAILED(hr))
	{
		_com_error err(hr);
		MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
		return false;
	}

	devcon->RSSetState(rasterizerState);

	InitGraphics();
	InitPipeline();

	return true;
}

// this is the function that cleans up Direct3D and COM
void CleanD3D()
{
	for (int i = 0; i < ARRAYSIZE(rows); ++i)
	{
		rows[i]->Release();
	}
	pVBuffer->Release();
	pLayout->Release();
	rasterizerState->Release();
	// close and release all existing COM objects
	pVS->Release();
	pPS->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
}

void RenderFrame(int mouse_x, int mouse_y)
{
	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);

	// set the shader objects
	devcon->VSSetShader(pVS, 0, 0);
	devcon->PSSetShader(pPS, 0, 0);

	// clear the back buffer to a deep blue
	D3DXCOLOR background = D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f);
	devcon->ClearRenderTargetView(backbuffer, (const float*)&background);





	

	// select which vertex buffer to display
	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

	// select which primtive type we are using
	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// draw the vertex buffer to the back buffer
	devcon->Draw(3, 0);

	// switch the back buffer and the front buffer
	swapchain->Present(1, 0);
}

// this is the function that creates the shape to render
void InitGraphics()
{
	// create a triangle using the VERTEX struct
	
	D3DXCOLOR colors[4] =
	{
		D3DXCOLOR(1.0f, 0.0f, 0.0f),
		D3DXCOLOR(0.0f, 1.0f, 0.0f),
		D3DXCOLOR(0.0f, 1.0f, 1.0f),
		D3DXCOLOR(1.0f, 1.0f, 0.0f),
	};

	// Flashing quads VBO
	for (int row = 0; row < ARRAYSIZE(rows); ++row)
	{
		const int quad_size = 10.0f;
		const int dquad_size = 2.0f * quad_size;
		D3DXCOLOR row_color = colors[row % ARRAYSIZE(colors)];
		VERTEX vertices[num_quads * vertices_per_quad];
		for (int i = 0; i < num_quads; ++i)
		{
			vertices[i * vertices_per_quad + 0] = VERTEX(quad_size + row * dquad_size, quad_size + i * dquad_size, 0, row_color);
			vertices[i * vertices_per_quad + 1] = VERTEX(quad_size + row * dquad_size, dquad_size + i * dquad_size, 0, row_color);
			vertices[i * vertices_per_quad + 2] = VERTEX(dquad_size + row * dquad_size, quad_size + i * dquad_size, 0, row_color);
			vertices[i * vertices_per_quad + 3] = VERTEX(dquad_size + row * dquad_size, quad_size + i * dquad_size, 0, row_color);
			vertices[i * vertices_per_quad + 4] = VERTEX(quad_size + row * dquad_size, dquad_size + i * dquad_size, 0, row_color);
			vertices[i * vertices_per_quad + 5] = VERTEX(dquad_size + row * dquad_size, dquad_size + i * dquad_size, 0, row_color);
			//vertices[i * 4 + 3] = VERTEX(40.0f + row * 40.0f, 20.0f + i * 40.0f, 0.0f, row_color);
		}

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
		bd.ByteWidth = sizeof(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = dev->CreateBuffer(&bd, NULL, &rows[row]);
		if (FAILED(hr))
		{
			_com_error err(hr);
			MessageBox(NULL, err.ErrorMessage(), NULL, MB_OK);
			return;
		}


		D3D11_MAPPED_SUBRESOURCE ms;
		devcon->Map(rows[row], NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);
		memcpy(ms.pData, vertices, sizeof(vertices));
		devcon->Unmap(rows[row], NULL);
	}

	// Mouse "cursor" VBO
	{
		// create the vertex buffer
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
		bd.ByteWidth = sizeof(VERTEX) * 3;             // size is the VERTEX struct * 3
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

		dev->CreateBuffer(&bd, NULL, &pVBuffer);       // create the buffer
	}
}


// this function loads and prepares the shaders
void InitPipeline()
{
	// load and compile the two shaders
	ID3D10Blob *VS, *PS;

	const char *shader_source = "struct VOut                                                                                  "
		"{                                                                                            "
		"    float4 position : SV_POSITION;                                                           "
		"    float4 color : COLOR;                                                                    "
		"};                                                                                           "
		"                                                                                             "
		"VOut VShader(float4 position : POSITION, float4 color : COLOR)                               "
		"{                                                                                            "
		"    VOut output;                                                                             "
		"                                                                                             "
		//"    output.position = float4((position.x / 960.0f) - 1, -(position.y / 600.0f) + 1, position.z, position.w);                                                              "
		"    output.position = float4((position.x / 400.0f) - 1, -(position.y / 300.0f) + 1, position.z, position.w);                                                              "
		"    output.color = color;                                                                    "
		"                                                                                             "
		"    return output;                                                                           "
		"}                                                                                            "
		"                                                                                             "
		"                                                                                             "
		"float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET              "
		"{                                                                                            "
		"    return color;                                                                            "
		"}                                                                                            ";

	D3D10CompileShader(shader_source, strlen(shader_source), NULL, NULL, NULL, "VShader", "vs_4_0", 0, &VS, NULL);
	D3D10CompileShader(shader_source, strlen(shader_source), NULL, NULL, NULL, "PShader", "ps_4_0", 0, &PS, NULL);

	//D3DX11CompileFromFile(L"shaders.shader", 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, 0, 0);
	//D3DX11CompileFromFile(L"shaders.shader", 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, 0, 0);

	// encapsulate both shaders into shader objects
	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

	// create the input layout object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	dev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
	devcon->IASetInputLayout(pLayout);
}
