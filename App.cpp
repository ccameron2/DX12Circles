#include "App.h"
#include "DDSTextureLoader.h"
#include <iostream>

std::vector<std::unique_ptr<FrameResource>> FrameResources;
unique_ptr<SRVDescriptorHeap> SrvDescriptorHeap;

App::App()
{
	Initialize();

	Run();
}

// Run the app
void App::Run()
{
	// Start timer for frametime
	mTimer.Start();

	// Set initial frame resources
	mGraphics->CycleFrameResources();
	
	mCamera->WindowResized(mWindow.get());

	while (!mWindow->mQuit)
	{
		if (!mWindow->mMinimized)
		{
			StartFrame();
			float frameTime = mTimer.GetLapTime();
			Update(frameTime);
			Draw(frameTime);
			EndFrame();
		}
	}	
}

void App::Initialize()
{
	mWindow = make_unique<Window>(800,600);

	HWND hwnd = mWindow->GetHWND();
;
	mGraphics = make_unique<Graphics>(hwnd, mWindow->mWidth, mWindow->mHeight);
	
	mCamera = make_unique<Camera>(mWindow.get());

	mCamera->Update();

	circles = new Circles();
	circles->InitCircles();

	LoadModels();
	CreateMaterials();

	BuildFrameResources();

	mNumModels = mModels.size();

	mGraphics->CreateRootSignature();

	mGraphics->CreateShaders();
	mGraphics->CreatePSO();

	mGraphics->ExecuteCommands();

	mGUI = make_unique<GUI>(SrvDescriptorHeap.get(), mWindow->mSDLWindow, mGraphics->mD3DDevice.Get(),
		mGraphics->mNumFrameResources, mGraphics->mBackBufferFormat);
}

void App::StartFrame()
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0)
	{
		ProcessEvents(event);
	}
	if (!mWindow->mMinimized)
	{
		mGUI->NewFrame();
	}
}

void App::LoadModels()
{
	masterBall = new Model("Models/sphere.x", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get());
	masterBall->mMeshes[0]->mMaterial = new Material();
	masterBall->mMeshes[0]->mMaterial->DiffuseAlbedo = XMFLOAT4{ 1,0,1,0 };

	masterBall2 = new Model("Models/sphere.x", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get());
	masterBall2->mMeshes[0]->mMaterial = new Material();
	masterBall2->mMeshes[0]->mMaterial->DiffuseAlbedo = XMFLOAT4{ 1,0.8,0,0 };

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		//Model* ballModel = new Model("Models/sphere.x", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get());
		Model* ballModel = new Model("", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get(),masterBall->mMeshes[0]);
		ballModel->SetPosition(XMFLOAT3{ circles->mMovingPositions[i].x,circles->mMovingPositions[i].y,circles->mMovingPositions[i].z });
		//ballModel->SetScale(XMFLOAT3{ 0.1, 0.1, 0.1 });
		mModels.push_back(ballModel);
		mMovingModels.push_back(ballModel);
	}

	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		//Model* ballModel = new Model("Models/sphere.x", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get());
		Model* ballModel = new Model("", mGraphics->mD3DDevice.Get(), mGraphics->mCommandList.Get(), masterBall2->mMeshes[0]);
		ballModel->SetPosition(XMFLOAT3{ circles->mStillPositions[i].x,circles->mStillPositions[i].y,circles->mStillPositions[i].z });
		//ballModel->SetScale(XMFLOAT3{ 0.1, 0.1, 0.1 });
		mModels.push_back(ballModel);
		mStillModels.push_back(ballModel);
	}

	for (int i = 0; i < mModels.size(); i++)
	{
		mModels[i]->mObjConstantBufferIndex = i;
	}
}

void App::BuildFrameResources()
{
	for (int i = 0; i < mGraphics->mNumFrameResources; i++)
	{
		FrameResources.push_back(std::make_unique<FrameResource>(mGraphics->mD3DDevice.Get(), 1, mModels.size(), mMaterials.size()));
	}
}

void App::Update(float frameTime)
{
	mGUI->UpdateModelData(mModels[mGUI->mSelectedModel]);

	mGUI->Update(mNumModels);

	mCamera->Update(frameTime,mGUI->mCameraOrbit,mGUI->mInvertY);

	if (mWindow->mScrollValue != 0)
	{
		mCamera->UpdateSpeed(mWindow->mScrollValue);
		mWindow->mScrollValue = 0;
	}
	if (mWindow->mForward)	mCamera->MoveForward();
	if (mWindow->mBackward) mCamera->MoveBackward();
	if (mWindow->mLeft)	mCamera->MoveLeft();
	if (mWindow->mRight) mCamera->MoveRight();
	if (mWindow->mUp)	mCamera->MoveUp();
	if (mWindow->mDown)	mCamera->MoveDown();


	circles->UpdateCircles(/*frameTime*/);
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mMovingModels[i]->SetPosition
		(
			XMFLOAT3{ circles->mMovingPositions[i].x, circles->mMovingPositions[i].y, circles->mMovingPositions[i].z }
		);
		mMovingModels[i]->mNumDirtyFrames += FrameResources.size();
	}
	for (int i = 0; i < NUM_CIRCLES / 2; i++)
	{
		mStillModels[i]->SetPosition
		(
			XMFLOAT3{ circles->mStillPositions[i].x, circles->mStillPositions[i].y, circles->mStillPositions[i].z }
		);
		mMovingModels[i]->mNumDirtyFrames += FrameResources.size();
	}

	//if (circles->mCollisions.size() > 0)
	//{
	//	circles->OutputFrame();
	//	std::cout << "Frame finished in: " << frameTime << std::endl;
	//}

	UpdateSelectedModel();
	UpdatePerObjectConstantBuffers();
	UpdatePerFrameConstantBuffer();
	UpdatePerMaterialConstantBuffers();
}

void App::UpdateSelectedModel()
{
	// If the world matrix has changed
	if (mGUI->mWMatrixChanged)
	{
		mModels[mGUI->mSelectedModel]->SetPosition(mGUI->mInPosition, false);
		mModels[mGUI->mSelectedModel]->SetRotation(mGUI->mInRotation, false);
		mModels[mGUI->mSelectedModel]->SetScale(mGUI->mInScale, true);
		mModels[mGUI->mSelectedModel]->mNumDirtyFrames += mGraphics->mNumFrameResources;

		mGUI->mWMatrixChanged = false;
	}
}

void App::UpdatePerObjectConstantBuffers()
{
	// Get reference to the current per object constant buffer
	auto currentObjectConstantBuffer = mGraphics->mCurrentFrameResource->mPerObjectConstantBuffer.get();
	
	for (auto& model : mModels)
	{
		// If their values have been changed
		if (model->mNumDirtyFrames > 0)
		{
			// Get the world matrix of the item
			XMMATRIX worldMatrix = XMLoadFloat4x4(&model->mWorldMatrix);
			bool parallax = model->mParallax;
			
			// Transpose data
			PerObjectConstants objectConstants;
			XMStoreFloat4x4(&objectConstants.WorldMatrix, XMMatrixTranspose(worldMatrix));
			objectConstants.parallax = parallax;

			// Copy the structure into the current buffer at the item's index
			currentObjectConstantBuffer->Copy(model->mObjConstantBufferIndex, objectConstants);

			model->mNumDirtyFrames--;
		}
	}
}

void App::UpdatePerFrameConstantBuffer()
{
	// Make a per frame constants structure
	PerFrameConstants perFrameConstantBuffer;

	// Create view, proj, and view proj matrices
	XMMATRIX view = XMLoadFloat4x4(&mCamera->mViewMatrix);
	XMMATRIX proj = XMLoadFloat4x4(&mCamera->mProjectionMatrix);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	// Transpose them into the structure
	XMStoreFloat4x4(&perFrameConstantBuffer.ViewMatrix, XMMatrixTranspose(view));
	XMStoreFloat4x4(&perFrameConstantBuffer.ProjMatrix, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&perFrameConstantBuffer.ViewProjMatrix, XMMatrixTranspose(viewProj));

	// Set the rest of the structure's values
	perFrameConstantBuffer.EyePosW = mCamera->mPos;
	perFrameConstantBuffer.RenderTargetSize = XMFLOAT2((float)mWindow->mWidth, (float)mWindow->mHeight);
	perFrameConstantBuffer.NearZ = 1.0f;
	perFrameConstantBuffer.FarZ = 1000.0f;
	perFrameConstantBuffer.TotalTime = mTimer.GetTime();
	perFrameConstantBuffer.DeltaTime = mTimer.GetLapTime();
	perFrameConstantBuffer.AmbientLight = { 0.01f, 0.01f, 0.01f, 1.0f };

	XMVECTOR lightDir = -SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMStoreFloat3(&perFrameConstantBuffer.Lights[0].Direction, lightDir);

	perFrameConstantBuffer.Lights[0].Colour = { 0.5f,0.5f,0.5f };
	perFrameConstantBuffer.Lights[0].Position = { 4.0f, 4.0f, 0.0f };
	perFrameConstantBuffer.Lights[0].Direction = { mGUI->mLightDir[0], mGUI->mLightDir[1], mGUI->mLightDir[2] };
	perFrameConstantBuffer.Lights[0].Strength = { 2,2,2 };

	// Copy the structure into the per frame constant buffer
	auto currentFrameCB = mGraphics->mCurrentFrameResource->mPerFrameConstantBuffer.get();
	currentFrameCB->Copy(0, perFrameConstantBuffer);
}

void App::UpdatePerMaterialConstantBuffers()
{
	auto currMaterialCB = mGraphics->mCurrentFrameResource->mPerMaterialConstantBuffer.get();

	for (auto& mat : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed. If the cbuffer data changes, it needs to be updated for each FrameResource.
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			PerMaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			matConstants.Metallic = mat->Metalness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->Copy(mat->CBIndex, matConstants);

			// Next FrameResource needs to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void App::Draw(float frameTime)
{
	auto commandAllocator = mGraphics->mCurrentFrameResource->mCommandAllocator.Get();
	auto commandList = mGraphics->mCommandList;

	mGraphics->ResetCommandAllocator(commandAllocator);

	mGraphics->ResetCommandList(commandAllocator, mCurrentPSO);

	mGraphics->SetViewportAndScissorRects();

	// Clear the back buffer and depth buffer.
	mGraphics->ClearBackBuffer();
	mGraphics->ClearDepthBuffer();

	// Select MSAA texture as render target
	mGraphics->SetMSAARenderTarget();

	mGraphics->SetDescriptorHeap(SrvDescriptorHeap->mHeap.Get());

	commandList->SetGraphicsRootSignature(mGraphics->mRootSignature.Get());

	auto srvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(SrvDescriptorHeap->mHeap->GetGPUDescriptorHandleForHeapStart());
	commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	auto perFrameBuffer = mGraphics->mCurrentFrameResource->mPerFrameConstantBuffer->GetBuffer();
	commandList->SetGraphicsRootConstantBufferView(2, perFrameBuffer->GetGPUVirtualAddress());

	DrawModels(commandList.Get());

	mGraphics->ResolveMSAAToBackBuffer();

	mGUI->Render(mGraphics->mCommandList.Get(), mGraphics->CurrentBackBuffer(), mGraphics->CurrentBackBufferView(), mGraphics->mDSVHeap.Get(), mGraphics->mDsvDescriptorSize);

	mGraphics->CloseAndExecuteCommandList();

	mGraphics->SwapBackBuffers(mGUI->mVSync);
}

void App::DrawModels(ID3D12GraphicsCommandList* commandList)
{

	if (mWireframe) { commandList->SetPipelineState(mGraphics->mWireframePSO.Get()); }
	else { commandList->SetPipelineState(mGraphics->mSolidPSO.Get()); }

	for(int i = 0; i < mMovingModels.size(); i++)
	{		
		mMovingModels[i]->Draw(commandList);
	}

	for(int i = 0; i < mStillModels.size(); i++)
	{
		mStillModels[i]->Draw(commandList);
	}
}

void App::EndFrame()
{
	// Advance fence value
	mGraphics->mCurrentFrameResource->Fence = ++mGraphics->mCurrentFence;

	// Tell command queue to set new fence point, will only be set when the GPU gets to new fence value.
	mGraphics->mCommandQueue->Signal(mGraphics->mFence.Get(), mGraphics->mCurrentFence);

	mGraphics->CycleFrameResources();
}

void App::ProcessEvents(SDL_Event& event)
{
	if(mGUI->ProcessEvents(event)) return;
	
	mWindow->ProcessEvents(event);

	if (mWindow->mMinimized) mTimer.Stop();
	else mTimer.Start();

	if (mWindow->mResized)
	{
		mGraphics->Resize(mWindow->mWidth, mWindow->mHeight);
		mCamera->WindowResized(mWindow.get());
		mWindow->mResized = false;
	}

	if(mWindow->mMiddleMouse)
	{
		mWireframe = !mWireframe;
	}

	if (mWindow->mMouseMoved)
	{
		mCamera->MouseMoved(event,mWindow.get());
		mWindow->mMouseMoved = false;
	}

}

void App::CreateMaterials()
{
	masterBall->mMeshes[0]->mMaterial->CBIndex = 0;
	mMaterials.push_back(masterBall->mMeshes[0]->mMaterial);
	masterBall2->mMeshes[0]->mMaterial->CBIndex = 1;
	mMaterials.push_back(masterBall2->mMeshes[0]->mMaterial);
	//for (auto& model : mModels)
	//{
	//	for (auto& mesh : model->mMeshes)
	//	{/*
	//		bool alreadyThere = false;
	//		for (auto& mat : mMaterials)
	//		{
	//			if (mat->Name == mesh->mMaterial->Name)
	//			{
	//				alreadyThere = true;
	//			}
	//		}
	//		if (!alreadyThere)
	//		{*/
	//			mesh->mMaterial->CBIndex = index;
	//			mMaterials.push_back(mesh->mMaterial);
	//			index++;
	//		//}
	//	}		
	//}
}

App::~App()
{
	// Empty the command queue
	if (mGraphics->mD3DDevice != nullptr) { mGraphics->EmptyCommandQueue(); }

	for (auto& model : mModels)
	{
		delete model;
	}

	for (auto& texture : mTextures)
	{
		delete texture;
	}

	circles->ClearMemory();
}