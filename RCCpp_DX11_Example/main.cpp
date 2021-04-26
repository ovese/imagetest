// Dear ImGui: standalone example application for DirectX 11
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
using namespace std;



//***************************adding this at top of source file main.cpp for image rendering in direct11***************************
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//******************heADERS FOR SHAPE FILE MANIPULATION
#include "shapefil.h"
//#include <../noncopyable.hpp>
//#include <../geometry.hpp>
//#include <../geometries.hpp>
//#include <../shape_creator.hpp>
//#include <../shp_create_object.hpp>
//#include <../shp_create_object_multi.hpp>
//#include <../dbf_write_attribute.hpp>
//#include <boost/geometry/io/wkt/wkt.hpp>

//#include <boost/geometry.hpp>
//#include <boost/geometry/geometries/geometries.hpp>
//#include <boost/geometry/geometries/points_xy.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shape_creator.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object_multi.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/dbf_write_attribute.hpp>

//#include <boost/core/noncopyable.hpp>
//#include "boost\config.hpp"
//#include <boost/geometry.hpp>
//#include <boost/geometry/geometries/geometries.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shape_creator.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object_multi.hpp>
//#include <boost/geometry/extensions/gis/io/shapelib/dbf_write_attribute.hpp>
//#include <boost/geometry/io/wkt/wkt.hpp>

//***********************second example***********

//#include <boost\geometry\geometry.hpp>
//#include "boost/geometry/geometries/geometries.hpp"
//#include "boost/geometry/geometries/point_xy.hpp"

//using namespace boost::geometry;

//***************** END OF HEADERS FOR SHAPE FILE MANIPULATION***************

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Simple helper function to load an image into a DX11 texture with common settings
bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    // Load from disk into a raw RGBA buffer
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

//***********************ending the addition for directx11 image rendering

//++++++++++++++++Addition for Shape file reader+++++++++++++++++++++++++
template <typename T, typename F>
 void read_shapefile(const std::string& filename, std::vector<T>& polygons, F functor)
 {
     try
     {
         SHPHandle handle = SHPOpen(filename.c_str(), "rb");
         if (handle <= 0)
         {
             throw std::string("File " + filename + " not found");
         }
 
         int nShapeType, nEntities;
         double adfMinBound[4], adfMaxBound[4];
         SHPGetInfo(handle, &nEntities, &nShapeType, adfMinBound, adfMaxBound );
 
         for (int i = 0; i < nEntities; i++)
         {
             SHPObject* psShape = SHPReadObject(handle, i );
 
             // Read only polygons, and only those without holes
             if (psShape->nSHPType == SHPT_POLYGON && psShape->nParts == 1)
             {
                 T polygon;
                 functor(psShape, polygon);
                 polygons.push_back(polygon);
             }
             SHPDestroyObject( psShape );
         }
         SHPClose(handle);
     }
     catch(const std::string& s)
     {
         throw s;
     }
     catch(...)
     {
         throw std::string("Other exception");
     }
 }
 
 
 template <typename T>
 void convert(SHPObject* psShape, T& polygon)
 {
     double* x = psShape->padfX;
     double* y = psShape->padfY;
     for (int v = 0; v < psShape->nVertices; v++)
     {
         typename point_type<T>::type point;
         assign_values(point, x[v], y[v]);
         append(polygon, point);
     }
 }
 //-------------------------ending addition for shapefile reader----------------------------

//***************************adding this at top of source file main.cpp for shape file rendering ***************************



// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear-ImGui-DirectX11-Example modifiee par Ese"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

	//************Initializing and loading texture*********************
	int my_image_width = 0;
	int my_image_height = 0;
	int width = 0;
	int height = 0;
	ID3D11ShaderResourceView* my_texture = NULL;
	bool ret = LoadTextureFromFile("../eseweb.jpg", &my_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

	ID3D11ShaderResourceView* my_texture_one = NULL;
	bool ret1 = LoadTextureFromFile("../MyImage01.jpg", &my_texture_one, &my_image_width, &my_image_height);
	IM_ASSERT(ret1);

	ID3D11ShaderResourceView* my_texture_view=NULL;
	bool ret2 = LoadTextureFromFile("../gdalicon.png", &my_texture_view, &width, &height);
	IM_ASSERT(ret2);

	//ID3D11ShaderResourceView* my_texture_two = NULL;
	//bool ret2 = LoadTextureFromFile("../strassen.shp", &my_texture_two, &my_image_width, &my_image_height);
	////IM_ASSERT(ret2);

	//************Ending Initialization and loading of texture*********************

	//+++++++++++++++++Initializing the shape file reader
	//std::string filename = "c:/data/spatial/shape/world_free/world.shp";
	//std::string filename = "../strassen.shp";
 //
 //    typedef model::polygon<model::d2::point_xy<double> > polygon_2d;
 //    std::vector<polygon_2d> polygons;
 //
 //    try
 //    {
 //        read_shapefile(filename, polygons, convert<polygon_2d>);
 //    }
 //    catch(const std::string& s)
 //    {
 //        std::cout << s << std::endl;
 //        return 1;
 //    }
 //----------------- ending shaoe file reader initialization---------------I have commented it out because it has not worked as yet due to header file--------
// ----------------.hpp conflict in my editor--------------------

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
	bool show_shape_selector_window=false;
	bool show_image_view_window=false;
	bool chbx1_sel=false;
	bool chbx2_sel=false;
	bool chbx3_sel=false;
	bool shp_sel=false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Bienvennue le Monde Ikore-CIRAD!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("Enfin Monsieur Raphael...Ese est la.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
			ImGui::Checkbox("Image View Window", &show_image_view_window);// I added the next two lines and windows
			ImGui::Checkbox("Shape selector Window", &show_shape_selector_window); 

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Devoir...Hello from another window!");

            if (ImGui::Button("Close Me"))
                show_another_window = false;

			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

		// ese is adding his own window here

		// 4. Show another simple window.
        if (show_image_view_window)
        {
            ImGui::Begin("Ese's Image View Window", &show_image_view_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			//I am adding code here
			//ImGui::Begin("DirectX11 Texture Test");//this was the copied begin function, I used mine above
			
			ImGui::Checkbox("Select Image 1: ",&chbx1_sel);
			ImGui::Checkbox("Select Image 2: ",&chbx2_sel);
			ImGui::Checkbox("Select Image 3: ",&chbx3_sel);

			ImGui::Text("pointer = %p", my_texture);
			ImGui::Text("size = %d x %d", my_image_width, my_image_height);
			ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));

			if(chbx2_sel){
				ImGui::Text("pointer = %p", my_texture_one);
				ImGui::Text("size = %d x %d", my_image_width, my_image_height);
				ImGui::Image((void*)my_texture_one, ImVec2(my_image_width, my_image_height));
			}

			if(chbx3_sel){
				ImGui::Text("pointer = %p", my_texture_view);
				ImGui::Text("size = %d x %d", my_image_width, my_image_height);
				ImGui::Image((void*)my_texture_view, ImVec2(my_image_width, my_image_height));
			}
			//ImGui::InputText(
			/*int state_n=0;
			switch (state_n){
				case 1:
					chbx1_sel=true;
					chbx2_sel=false;
					ImGui::Text("pointer = %p", my_texture);
					ImGui::Text("size = %d x %d", my_image_width, my_image_height);
					ImGui::Image((void*)my_texture, ImVec2(my_image_width, my_image_height));
					break;
				case 2:
					chbx1_sel=false;
					chbx2_sel=true;
					ImGui::Text("pointer = %p", my_texture_one);
					ImGui::Text("size = %d x %d", my_image_width, my_image_height);
					ImGui::Image((void*)my_texture_one, ImVec2(my_image_width, my_image_height));
					break;
				default:
					chbx1_sel=false;
					chbx2_sel=false;
			}*/
			
			

            if (ImGui::Button("Close Me"))
                show_image_view_window = false;

			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

		// ese is adding his own window here

		// 5. Show another simple window.
        if (show_shape_selector_window)
        {
            ImGui::Begin("Ese's Shape selector Window", &show_shape_selector_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
			//I am adding code here
			
			ImGui::Text("*******Shape selector*******");			
			ImGui::Checkbox("Shape one", &shp_sel);

			//++++++++++++++++++++++++++++++++++++++++++
			ImGui::Text("pointer = %p", my_texture_view);
			ImGui::Text("size = %d x %d", width, height);
			ImGui::Image((void*)my_texture_view, ImVec2(my_image_width, my_image_height));

			// The example DirectX11 back-end uses ID3D11ShaderResourceView* to identify textures.				
			//static ID3D11Device* g_pd3dDevice =NULL;
			
			
			//ID3D11Texture2D* my_texture= NULL;

			//ImGuiIO& io = ImGui::GetIO();
			//unsigned char* pixels;
			////int width, height;
			//io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			//D3D11_TEXTURE2D_DESC desc;
			//ZeroMemory(&desc, sizeof(desc));
			//desc.Width = width;
			//desc.Height = height;
			//desc.MipLevels = 1;
			//desc.ArraySize = 1;
			//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//desc.SampleDesc.Count = 1;
			//desc.Usage = D3D11_USAGE_DEFAULT;
			//desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			//desc.CPUAccessFlags = 0;

			//D3D11_SUBRESOURCE_DATA subResource;
			//subResource.pSysMem = pixels;
			//subResource.SysMemPitch = desc.Width * 4;
			//subResource.SysMemSlicePitch = 0;
			//g_pd3dDevice->CreateTexture2D(&desc, &subResource, &my_texture);


			//D3D11_SHADER_RESOURCE_VIEW_DESC my_shader_resource_view_desc;
			//ZeroMemory(&my_shader_resource_view_desc, sizeof(my_shader_resource_view_desc));
			//my_shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//my_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			//my_shader_resource_view_desc.Texture2D.MipLevels = desc.MipLevels;
			//my_shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
			//g_pd3dDevice->CreateShaderResourceView(my_texture, &my_shader_resource_view_desc, &my_texture_view);
			//ImGui::Image((void*)my_texture_view, ImVec2(512,512));
			//my_texture->Release();
			//-----------------------------------------


            if (ImGui::Button("Close Me"))
                show_shape_selector_window = false;

			//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
		

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
