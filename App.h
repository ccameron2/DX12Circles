#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#define SDL_MAIN_HANDLED
#define SDL_ENABLE_SYSWM_WINDOWS

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <memory>
#include "Common.h"

#include "Planet.h"
#include "Model.h"

#include <SDL.h>
#include <imgui.h>
#include "GUI.h"

#include "imgui_impl_sdl.h"
#include "imgui_impl_dx12.h"

#include "Window.h"
#include "Camera.h"

#include "Circles.h"

#include "Timer.h"
#include "Utility.h"
#include "Icosahedron.h"
#include "Graphics.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "SRVDescriptorHeap.h"
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>

#include <fstream>

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class App
{
public:
	App();

	// Flush command queue to prevent GPU crash on exit
	~App();

	void Run();

	void Initialize();
private:
	Timer mTimer;
	Circles* circles;
	void Update(float frameTime);
	void Draw(float frameTime);
	void ProcessEvents(SDL_Event& e);
	void CreateMaterials();
	
	vector<Texture*> mTextures;
	vector<Material*> mMaterials;

	unique_ptr<Graphics> mGraphics;
	unique_ptr<Window> mWindow;
	SDL_Surface mScreenSurface;

	// If diffent PSOs needed then use different lists
	vector<Model*> mModels;
	vector<Model*> mMovingModels;
	vector<Model*> mStillModels;

	unique_ptr<Camera> mCamera;
	unique_ptr<GUI> mGUI;

	Model* masterBall;
	Model* masterBall2;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	int mNumModels = 0;

	bool mWireframe = false;

	// Input layout
	ID3D12PipelineState* mCurrentPSO = nullptr;

	void UpdateSelectedModel();
	void UpdatePerObjectConstantBuffers();
	void UpdatePerFrameConstantBuffer();
	void UpdatePerMaterialConstantBuffers();

	void BuildFrameResources();

	void LoadModels();
	void DrawModels(ID3D12GraphicsCommandList* commandList);
	void StartFrame();
	void EndFrame();
};