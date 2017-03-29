// Fill out your copyright notice in the Description page of Project Settings.
#include "OpenVRExpansionPluginPrivatePCH.h"
#include "Engine/Texture2D.h"

#if WITH_EDITOR
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#endif

#include "OpenVRExpansionFunctionLibrary.h"

//General Log
DEFINE_LOG_CATEGORY(OpenVRExpansionFunctionLibraryLog);

UOpenVRExpansionFunctionLibrary::UOpenVRExpansionFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

//=============================================================================
UOpenVRExpansionFunctionLibrary::~UOpenVRExpansionFunctionLibrary()
{
#if STEAMVR_SUPPORTED_PLATFORM
	if(bInitialized)
		UnloadOpenVRModule();
#endif
}


bool UOpenVRExpansionFunctionLibrary::OpenVRHandles()
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else
	if (IsLocallyControlled() && !bInitialized)
		bInitialized = LoadOpenVRModule();
	else if (bInitialized)
		return true;
	else
		bInitialized = false;

	return bInitialized;
#endif
}

bool UOpenVRExpansionFunctionLibrary::CloseVRHandles()
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else
	if (OpenCamera.IsValid())
	{
		EBPVRCameraResultSwitch Result;
		ReleaseVRCamera(OpenCamera, Result);
	}

	if (bInitialized)
	{
		UnloadOpenVRModule();
		bInitialized = false;
		return true;
	}
	else
		return false;
#endif
}

bool UOpenVRExpansionFunctionLibrary::LoadOpenVRModule()
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else
#if PLATFORM_WINDOWS
#if PLATFORM_64BITS

	if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Failed to load OpenVR library, HMD device either not connected or not using SteamVR"));
		return false;
	}

	FString RootOpenVRPath;
	TCHAR VROverridePath[MAX_PATH];
	FPlatformMisc::GetEnvironmentVariable(TEXT("VR_OVERRIDE"), VROverridePath, MAX_PATH);

	if (FCString::Strlen(VROverridePath) > 0)
	{
		RootOpenVRPath = FString::Printf(TEXT("%s\\bin\\win64\\"), VROverridePath);
	}
	else
	{
		RootOpenVRPath = FPaths::EngineDir() / FString::Printf(TEXT("Binaries/ThirdParty/OpenVR/%s/Win64/"), OPENVR_SDK_VER);
	}

	FPlatformProcess::PushDllDirectory(*RootOpenVRPath);
	OpenVRDLLHandle = FPlatformProcess::GetDllHandle(*(RootOpenVRPath + "openvr_api.dll"));
	FPlatformProcess::PopDllDirectory(*RootOpenVRPath);
#else
	FString RootOpenVRPath = FPaths::EngineDir() / FString::Printf(TEXT("Binaries/ThirdParty/OpenVR/%s/Win32/"), OPENVR_SDK_VER);
	FPlatformProcess::PushDllDirectory(*RootOpenVRPath);
	OpenVRDLLHandle = FPlatformProcess::GetDllHandle(*(RootOpenVRPath + "openvr_api.dll"));
	FPlatformProcess::PopDllDirectory(*RootOpenVRPath);
#endif
#elif PLATFORM_MAC
	OpenVRDLLHandle = FPlatformProcess::GetDllHandle(TEXT("libopenvr_api.dylib"));
#endif	//PLATFORM_WINDOWS

	if (!OpenVRDLLHandle)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Failed to load OpenVR library."));
		return false;
	}

	//@todo steamvr: Remove GetProcAddress() workaround once we update to Steamworks 1.33 or higher
	//VRInitFn = (pVRInit)FPlatformProcess::GetDllExport(OpenVRDLLHandle, TEXT("VR_Init"));
	//VRShutdownFn = (pVRShutdown)FPlatformProcess::GetDllExport(OpenVRDLLHandle, TEXT("VR_Shutdown"));
	//VRIsHmdPresentFn = (pVRIsHmdPresent)FPlatformProcess::GetDllExport(OpenVRDLLHandle, TEXT("VR_IsHmdPresent"));
//VRGetStringForHmdErrorFn = (pVRGetStringForHmdError)FPlatformProcess::GetDllExport(OpenVRDLLHandle, TEXT("VR_GetStringForHmdError"));
	VRGetGenericInterfaceFn = (pVRGetGenericInterface)FPlatformProcess::GetDllExport(OpenVRDLLHandle, TEXT("VR_GetGenericInterface"));

	if (!VRGetGenericInterfaceFn)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Failed to GetProcAddress() on openvr_api.dll"));
		UnloadOpenVRModule();
		return false;
	}

	// Grab the chaperone
	vr::EVRInitError ChaperoneErr = vr::VRInitError_None;
	//VRChaperone = (vr::IVRChaperone*)vr::VR_GetGenericInterface(vr::IVRChaperone_Version, &ChaperoneErr);
	//VRChaperone = (vr::IVRChaperone*)(*VRGetGenericInterfaceFn)(vr::IVRChaperone_Version, &ChaperoneErr);
	/*if ((VRChaperone != nullptr) && (ChaperoneErr == vr::VRInitError_None))
	{
		//ChaperoneBounds = FChaperoneBounds(VRChaperone);
	}
	else
	{
		VRChaperone = nullptr;
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Failed to initialize Chaperone system"));
	}*/

	return true;
#endif
}

void UOpenVRExpansionFunctionLibrary::UnloadOpenVRModule()
{
#if STEAMVR_SUPPORTED_PLATFORM
	if (OpenVRDLLHandle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(OpenVRDLLHandle);
		OpenVRDLLHandle = nullptr;
		//(*VRShutdownFn)();
	}
#endif
}

bool UOpenVRExpansionFunctionLibrary::HasVRCamera()
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else
	// Don't run anything if no HMD and if the HMD is not a steam type
	if (!GEngine->HMDDevice.IsValid() || (GEngine->HMDDevice->GetHMDDeviceType() != EHMDDeviceType::DT_SteamVR))
		return false;

	vr::HmdError HmdErr;
	vr::IVRTrackedCamera * VRCamera = (vr::IVRTrackedCamera*)(*VRGetGenericInterfaceFn)(vr::IVRTrackedCamera_Version, &HmdErr);

	if (!VRCamera || HmdErr != vr::HmdError::VRInitError_None)
		return false;

	bool pHasCamera;
	vr::EVRTrackedCameraError CamError = VRCamera->HasCamera(vr::k_unTrackedDeviceIndex_Hmd, &pHasCamera);

	if (CamError != vr::EVRTrackedCameraError::VRTrackedCameraError_None)
		return false;

	return pHasCamera;

#endif
}

void UOpenVRExpansionFunctionLibrary::AcquireVRCamera(FBPOpenVRCameraHandle & CameraHandle, EBPVRCameraResultSwitch & Result)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	Result = EBPVRCameraResultSwitch::OnFailed;
	return;
#else
	// Don't run anything if no HMD and if the HMD is not a steam type
	if (!GEngine->HMDDevice.IsValid() || (GEngine->HMDDevice->GetHMDDeviceType() != EHMDDeviceType::DT_SteamVR))
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	vr::HmdError HmdErr;
	vr::IVRTrackedCamera * VRCamera = (vr::IVRTrackedCamera*)(*VRGetGenericInterfaceFn)(vr::IVRTrackedCamera_Version, &HmdErr);

	if (!VRCamera || HmdErr != vr::HmdError::VRInitError_None)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	vr::EVRTrackedCameraError CamError = VRCamera->AcquireVideoStreamingService(vr::k_unTrackedDeviceIndex_Hmd, &CameraHandle.pCameraHandle);

	if (CamError != vr::EVRTrackedCameraError::VRTrackedCameraError_None)
		CameraHandle.pCameraHandle = INVALID_TRACKED_CAMERA_HANDLE;

	if (CameraHandle.pCameraHandle == INVALID_TRACKED_CAMERA_HANDLE)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	OpenCamera = CameraHandle;
	Result = EBPVRCameraResultSwitch::OnSucceeded;
	return;
#endif
}

void UOpenVRExpansionFunctionLibrary::ReleaseVRCamera(UPARAM(ref) FBPOpenVRCameraHandle & CameraHandle, EBPVRCameraResultSwitch & Result)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	Result = EBPVRCameraResultSwitch::OnFailed;
	return;
#else
	// Don't run anything if no HMD and if the HMD is not a steam type
	if (!GEngine->HMDDevice.IsValid() || (GEngine->HMDDevice->GetHMDDeviceType() != EHMDDeviceType::DT_SteamVR))
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	if (!CameraHandle.IsValid())
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	vr::HmdError HmdErr;
	vr::IVRTrackedCamera * VRCamera = (vr::IVRTrackedCamera*)(*VRGetGenericInterfaceFn)(vr::IVRTrackedCamera_Version, &HmdErr);

	if (!VRCamera || HmdErr != vr::HmdError::VRInitError_None)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	vr::EVRTrackedCameraError CamError = VRCamera->ReleaseVideoStreamingService(CameraHandle.pCameraHandle);
	CameraHandle.pCameraHandle = INVALID_TRACKED_CAMERA_HANDLE;

	OpenCamera = CameraHandle;
	Result = EBPVRCameraResultSwitch::OnSucceeded;
	return;

#endif
}

bool UOpenVRExpansionFunctionLibrary::IsValid(UPARAM(ref) FBPOpenVRCameraHandle & CameraHandle)
{
	return (CameraHandle.IsValid());
}


UTexture2D * UOpenVRExpansionFunctionLibrary::CreateCameraTexture2D(UPARAM(ref) FBPOpenVRCameraHandle & CameraHandle, EOpenVRCameraFrameType FrameType, EBPVRCameraResultSwitch & Result)
{
#if !STEAMVR_SUPPORTED_PLATFORM 
	Result = EBPVRCameraResultSwitch::OnFailed;
	return nullptr;
#else
	// Don't run anything if no HMD and if the HMD is not a steam type
	if (!GEngine->HMDDevice.IsValid() || (GEngine->HMDDevice->GetHMDDeviceType() != EHMDDeviceType::DT_SteamVR))
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return nullptr;
	}

	if (!CameraHandle.IsValid() || !FApp::CanEverRender())
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return nullptr;
	}

	vr::HmdError HmdErr;
	vr::IVRTrackedCamera * VRCamera = (vr::IVRTrackedCamera*)(*VRGetGenericInterfaceFn)(vr::IVRTrackedCamera_Version, &HmdErr);

	if (!VRCamera || HmdErr != vr::HmdError::VRInitError_None)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return nullptr;
	}

	uint32 Width;
	uint32 Height;
	uint32 FrameBufferSize;
	vr::EVRTrackedCameraError CamError = VRCamera->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, (vr::EVRTrackedCameraFrameType)FrameType, &Width, &Height, &FrameBufferSize);

	if (Width > 0 && Height > 0)
	{

		UTexture2D * NewRenderTarget2D = UTexture2D::CreateTransient(Width, Height, EPixelFormat::PF_R8G8B8A8);//NewObject<UTexture2D>(GetWorld());
		check(NewRenderTarget2D);

		//NewRenderTarget2D->PlatformData = new FTexturePlatformData();
		//NewRenderTarget2D->PlatformData->SizeX = Width;
		//NewRenderTarget2D->PlatformData->SizeY = Height;
		//NewRenderTarget2D->PlatformData->PixelFormat = EPixelFormat::PF_R8G8B8A8;

		// Allocate first mipmap.
		//int32 NumBlocksX = Width / GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockSizeX;
		//int32 NumBlocksY = Height / GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockSizeY;
		//FTexture2DMipMap* Mip = new(NewRenderTarget2D->PlatformData->Mips) FTexture2DMipMap();
		//Mip->SizeX = Width;
		//Mip->SizeY = Height;
		//Mip->BulkData.Lock(LOCK_READ_WRITE);
		//Mip->BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockBytes);
		//Mip->BulkData.Unlock();

		//Setting some Parameters for the Texture and finally returning it
		NewRenderTarget2D->PlatformData->NumSlices = 1;
		NewRenderTarget2D->NeverStream = true;
		NewRenderTarget2D->UpdateResource();

		//NewRenderTarget2D->InitCustomFormat(Width, Height, /*EPixelFormat::PF_R8G8B8A8*/EPixelFormat::PF_B8G8R8A8, false);
		//NewRenderTarget2D->UpdateResourceImmediate(true);

		Result = EBPVRCameraResultSwitch::OnSucceeded;
		return NewRenderTarget2D;
	}

	Result = EBPVRCameraResultSwitch::OnFailed;
	return nullptr;
#endif
}

void UOpenVRExpansionFunctionLibrary::GetVRCameraFrame(UPARAM(ref) FBPOpenVRCameraHandle & CameraHandle, EOpenVRCameraFrameType FrameType, EBPVRCameraResultSwitch & Result, UTexture2D * TargetRenderTarget)
{
#if !STEAMVR_SUPPORTED_PLATFORM 
	Result = EBPVRCameraResultSwitch::OnFailed;
	return;
#else
	// Don't run anything if no HMD and if the HMD is not a steam type
	if (!GEngine->HMDDevice.IsValid() || (GEngine->HMDDevice->GetHMDDeviceType() != EHMDDeviceType::DT_SteamVR))
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	if (!TargetRenderTarget || !CameraHandle.IsValid())
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	vr::HmdError HmdErr;
	vr::IVRTrackedCamera * VRCamera = (vr::IVRTrackedCamera*)(*VRGetGenericInterfaceFn)(vr::IVRTrackedCamera_Version, &HmdErr);

	if (!VRCamera || HmdErr != vr::HmdError::VRInitError_None)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	uint32 Width;
	uint32 Height;
	uint32 FrameBufferSize;
	vr::EVRTrackedCameraError CamError = VRCamera->GetCameraFrameSize(vr::k_unTrackedDeviceIndex_Hmd, (vr::EVRTrackedCameraFrameType)FrameType, &Width, &Height, &FrameBufferSize);

	if (CamError != vr::EVRTrackedCameraError::VRTrackedCameraError_None || Width <= 0 || Height <= 0)
	{
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	// Make sure formats are correct
	check(FrameBufferSize == (Width * Height * GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockSizeX));


	// Need to bring this back after moving from render target to this
	// Update the format if required, this is in case someone made a new render target NOT with my custom function
	// Enforces correct buffer size for the camera feed
	if (TargetRenderTarget->GetSizeX() != Width || TargetRenderTarget->GetSizeY() != Height || TargetRenderTarget->GetPixelFormat() != EPixelFormat::PF_R8G8B8A8)
	{
		TargetRenderTarget->PlatformData->SizeX = Width;
		TargetRenderTarget->PlatformData->SizeY = Height;
		TargetRenderTarget->PlatformData->PixelFormat = EPixelFormat::PF_R8G8B8A8;
		
		// Allocate first mipmap.
		int32 NumBlocksX = Width / GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockSizeX;
		int32 NumBlocksY = Height / GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockSizeY;

		TargetRenderTarget->PlatformData->Mips[0].SizeX = Width;
		TargetRenderTarget->PlatformData->Mips[0].SizeY = Height;
		TargetRenderTarget->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		TargetRenderTarget->PlatformData->Mips[0].BulkData.Realloc(NumBlocksX * NumBlocksY * GPixelFormats[EPixelFormat::PF_R8G8B8A8].BlockBytes);
		TargetRenderTarget->PlatformData->Mips[0].BulkData.Unlock();
	}
	
	vr::CameraVideoStreamFrameHeader_t CamHeader;
	uint8* pData = new uint8[FrameBufferSize];

	/*uint8* MipData = (uint8*)TargetRenderTarget->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	CamError = VRCamera->GetVideoStreamFrameBuffer(CameraHandle.pCameraHandle, (vr::EVRTrackedCameraFrameType)FrameType, MipData, TargetRenderTarget->PlatformData->Mips[0].BulkData.GetBulkDataSize(), &CamHeader, sizeof(vr::CameraVideoStreamFrameHeader_t));
	TargetRenderTarget->PlatformData->Mips[0].BulkData.Unlock();
	TargetRenderTarget->UpdateResource();*/

	CamError = VRCamera->GetVideoStreamFrameBuffer(CameraHandle.pCameraHandle, (vr::EVRTrackedCameraFrameType)FrameType, pData, FrameBufferSize, &CamHeader, sizeof(vr::CameraVideoStreamFrameHeader_t));

	// No frame available = still on spin / wake up
	if (CamError != vr::EVRTrackedCameraError::VRTrackedCameraError_None || CamError == vr::EVRTrackedCameraError::VRTrackedCameraError_NoFrameAvailable)
	{
		delete[] pData;
		Result = EBPVRCameraResultSwitch::OnFailed;
		return;
	}

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		UpdateDynamicTextureCode,
		UTexture2D*, pTexture, TargetRenderTarget,
		const uint8*, pData, pData,
		{
			FUpdateTextureRegion2D region;
			region.SrcX = 0;
			region.SrcY = 0;
			region.DestX = 0;
			region.DestY = 0;
			region.Width = pTexture->GetSizeX();// TEX_WIDTH;
			region.Height = pTexture->GetSizeY();//TEX_HEIGHT;

			FTexture2DResource* resource = (FTexture2DResource*)pTexture->Resource;
			RHIUpdateTexture2D(resource->GetTexture2DRHI(), 0, region, region.Width * GPixelFormats[pTexture->GetPixelFormat()].BlockBytes/*TEX_PIXEL_SIZE_IN_BYTES*/, pData);
			delete[] pData;
		});

	// Letting the enqueued command free the memory
	Result = EBPVRCameraResultSwitch::OnSucceeded;
	return;
#endif
}

bool UOpenVRExpansionFunctionLibrary::GetVRControllerPropertyString(EVRControllerProperty_String PropertyToRetrieve, int32 DeviceID, FString & StringValue)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else

	if (!bInitialized)
		return false;

	if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
		return false;

	vr::HmdError HmdErr;
	vr::IVRSystem * VRSystem = (vr::IVRSystem*)(*VRGetGenericInterfaceFn)(vr::IVRSystem_Version, &HmdErr);
	//vr::IVRSystem * VRSystem = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);

	if (!VRSystem)
		return false;

	vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_Success;

	char charvalue[vr::k_unMaxPropertyStringSize];
	uint32_t buffersize = 255;
	uint32_t ret = VRSystem->GetStringTrackedDeviceProperty(DeviceID, (vr::ETrackedDeviceProperty) (((int32)PropertyToRetrieve) + 3000), charvalue, buffersize, &pError);

	if (pError != vr::TrackedPropertyError::TrackedProp_Success)
		return false;

	StringValue = FString(ANSI_TO_TCHAR(charvalue));
	return true;

#endif
}

bool UOpenVRExpansionFunctionLibrary::GetVRDevicePropertyString(EVRDeviceProperty_String PropertyToRetrieve, int32 DeviceID, FString & StringValue)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else

	if (!bInitialized)
		return false;

	if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
		return false;

	vr::HmdError HmdErr;
	vr::IVRSystem * VRSystem = (vr::IVRSystem*)(*VRGetGenericInterfaceFn)(vr::IVRSystem_Version, &HmdErr);
	//vr::IVRSystem * VRSystem = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);

	if (!VRSystem)
		return false;

	vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_Success;

	char charvalue[vr::k_unMaxPropertyStringSize];
	uint32_t buffersize = 255;
	uint32_t ret = VRSystem->GetStringTrackedDeviceProperty(DeviceID, (vr::ETrackedDeviceProperty) (((int32)PropertyToRetrieve) + 1000), charvalue, buffersize, &pError);

	if (pError != vr::TrackedPropertyError::TrackedProp_Success)
		return false;

	StringValue = FString(ANSI_TO_TCHAR(charvalue));
	return true;

#endif
}

bool UOpenVRExpansionFunctionLibrary::GetVRDevicePropertyBool(EVRDeviceProperty_Bool PropertyToRetrieve, int32 DeviceID, bool & BoolValue)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else

	if (!bInitialized)
		return false;

	if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
		return false;

	vr::HmdError HmdErr;
	vr::IVRSystem * VRSystem = (vr::IVRSystem*)(*VRGetGenericInterfaceFn)(vr::IVRSystem_Version, &HmdErr);
	//vr::IVRSystem * VRSystem = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);

	if (!VRSystem)
		return false;

	vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_Success;

	bool ret = VRSystem->GetBoolTrackedDeviceProperty(DeviceID, (vr::ETrackedDeviceProperty) (((int32)PropertyToRetrieve) + 1000), &pError);

	if (pError != vr::TrackedPropertyError::TrackedProp_Success)
		return false;

	BoolValue = ret;
	return true;

#endif
}

bool UOpenVRExpansionFunctionLibrary::GetVRDevicePropertyFloat(EVRDeviceProperty_Float PropertyToRetrieve, int32 DeviceID, float & FloatValue)
{
#if !STEAMVR_SUPPORTED_PLATFORM
	return false;
#else

	if (!bInitialized)
		return false;

	if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
		return false;

	vr::HmdError HmdErr;
	vr::IVRSystem * VRSystem = (vr::IVRSystem*)(*VRGetGenericInterfaceFn)(vr::IVRSystem_Version, &HmdErr);
	//vr::IVRSystem * VRSystem = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);

	if (!VRSystem)
		return false;

	vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_Success;

	float ret = VRSystem->GetFloatTrackedDeviceProperty(DeviceID, (vr::ETrackedDeviceProperty) (((int32)PropertyToRetrieve) + 1000), &pError);

	if (pError != vr::TrackedPropertyError::TrackedProp_Success)
		return false;

	FloatValue = ret;
	return true;

#endif
}

UTexture2D * UOpenVRExpansionFunctionLibrary::GetVRDeviceModelAndTexture(UObject* WorldContextObject, EBPSteamVRTrackedDeviceType DeviceType, TArray<UProceduralMeshComponent *> ProceduralMeshComponentsToFill, bool bCreateCollision, EAsyncBlueprintResultSwitch &Result/*, TArray<uint8> & OutRawTexture, bool bReturnRawTexture*/)
{

#if !STEAMVR_SUPPORTED_PLATFORM
	Result = EAsyncBlueprintResultSwitch::OnFailure;
	return NULL;
	UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Not SteamVR Supported Platform!!"));
#else

	if (!bInitialized)
	{
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return NULL;
	}

	/*if (!(GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR)))
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Get HMD Device!!"));
		bSucceeded = false;
		return nullptr;
	}*/

/*	FSteamVRHMD* SteamVRHMD = (FSteamVRHMD*)(GEngine->HMDDevice.Get());
	if (!SteamVRHMD || !SteamVRHMD->IsStereoEnabled())
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Get HMD Device!!"));
		bSucceeded = false;
		return nullptr;
	}*/

	vr::HmdError HmdErr;
	vr::IVRSystem * VRSystem = (vr::IVRSystem*)(*VRGetGenericInterfaceFn)(vr::IVRSystem_Version, &HmdErr);
	//vr::IVRSystem * VRSystem = (vr::IVRSystem*)vr::VR_GetGenericInterface(vr::IVRSystem_Version, &HmdErr);

	if (!VRSystem)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("VRSystem InterfaceErrorCode %i"), (int32)HmdErr);
	}

	vr::IVRRenderModels * VRRenderModels = (vr::IVRRenderModels*)(*VRGetGenericInterfaceFn)(vr::IVRRenderModels_Version, &HmdErr);
	//vr::IVRRenderModels * VRRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &HmdErr);

	if (!VRRenderModels)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Render Models InterfaceErrorCode %i"), (int32)HmdErr);
	}


	if (!VRSystem || !VRRenderModels)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Get Interfaces!!"));
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return nullptr;
	}

	TArray<int32> TrackedIDs;

	USteamVRFunctionLibrary::GetValidTrackedDeviceIds((ESteamVRTrackedDeviceType)DeviceType, TrackedIDs);
	if (TrackedIDs.Num() == 0)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Get Tracked Devices!!"));
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return nullptr;
	}

	int32 DeviceID = TrackedIDs[0];

	vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_Success;

	char RenderModelName[vr::k_unMaxPropertyStringSize];
	uint32_t buffersize = 255;
	uint32_t ret = VRSystem->GetStringTrackedDeviceProperty(DeviceID, vr::ETrackedDeviceProperty::Prop_RenderModelName_String, RenderModelName, buffersize, &pError);

	if (pError != vr::TrackedPropertyError::TrackedProp_Success)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Get Render Model Name String!!"));
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return nullptr;
	}

	//uint32_t numComponents = VRRenderModels->GetComponentCount("vr_controller_vive_1_5");
	//UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("NumComponents: %i"), (int32)numComponents);
	// if numComponents > 0 load each, otherwise load the main one only

	vr::RenderModel_t *RenderModel = NULL;

	//VRRenderModels->LoadRenderModel()
	vr::EVRRenderModelError ModelErrorCode = VRRenderModels->LoadRenderModel_Async(RenderModelName, &RenderModel);
	
	if (ModelErrorCode != vr::EVRRenderModelError::VRRenderModelError_None)
	{
		if (ModelErrorCode != vr::EVRRenderModelError::VRRenderModelError_Loading)
		{
			UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Load Model!!"));
			Result = EAsyncBlueprintResultSwitch::OnFailure;
		}
		else
			Result = EAsyncBlueprintResultSwitch::AsyncLoading;

		return nullptr;
	}

	if (!RenderModel)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Load Model!!"));
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return nullptr;
	}

	vr::TextureID_t texID = RenderModel->diffuseTextureId;
	vr::RenderModel_TextureMap_t * texture = NULL;

	UTexture2D* OutTexture = nullptr;
	vr::EVRRenderModelError TextureErrorCode = VRRenderModels->LoadTexture_Async(texID, &texture);

	if (TextureErrorCode != vr::EVRRenderModelError::VRRenderModelError_None)
	{
		if (TextureErrorCode != vr::EVRRenderModelError::VRRenderModelError_Loading)
		{
			UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Load Texture!!"));
			Result = EAsyncBlueprintResultSwitch::OnFailure;
		}
		else
			Result = EAsyncBlueprintResultSwitch::AsyncLoading;

		return nullptr;
	}

	if (!texture)
	{
		UE_LOG(OpenVRExpansionFunctionLibraryLog, Warning, TEXT("Couldn't Load Texture!!"));
		Result = EAsyncBlueprintResultSwitch::OnFailure;
		return nullptr;
	}

	if (ProceduralMeshComponentsToFill.Num() > 0)
	{
		TArray<FVector> vertices;
		TArray<int32> triangles;
		TArray<FVector> normals;
		TArray<FVector2D> UV0;
		TArray<FColor> vertexColors;
		TArray<FProcMeshTangent> tangents;
		
		vr::HmdVector3_t vPosition;
		vr::HmdVector3_t vNormal;
		for (uint32_t i = 0; i < RenderModel->unVertexCount; ++i)
		{
			vPosition = RenderModel->rVertexData[i].vPosition;
			vertices.Add(FVector(vPosition.v[2], vPosition.v[1], vPosition.v[0]));

			vNormal = RenderModel->rVertexData[i].vNormal;
			normals.Add(FVector(vNormal.v[2], vNormal.v[1], vNormal.v[0]));

			UV0.Add(FVector2D(RenderModel->rVertexData[i].rfTextureCoord[0], RenderModel->rVertexData[i].rfTextureCoord[1]));
		}

		for (uint32_t i = 0; i < RenderModel->unTriangleCount * 3; i += 3)
		{
			triangles.Add(RenderModel->rIndexData[i]);
			triangles.Add(RenderModel->rIndexData[i + 1]);
			triangles.Add(RenderModel->rIndexData[i + 2]);
		}

		float scale = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(WorldContextObject);
		for (int i = 0; i < ProceduralMeshComponentsToFill.Num(); ++i)
		{
			ProceduralMeshComponentsToFill[i]->ClearAllMeshSections();
			ProceduralMeshComponentsToFill[i]->CreateMeshSection(1, vertices, triangles, normals, UV0, vertexColors, tangents, bCreateCollision);
			ProceduralMeshComponentsToFill[i]->SetMeshSectionVisible(1, true);
			ProceduralMeshComponentsToFill[i]->SetWorldScale3D(FVector(scale, scale, scale));
		}
	}

	uint32 Width = texture->unWidth;
	uint32 Height = texture->unHeight;

	OutTexture = UTexture2D::CreateTransient(Width, Height, PF_R8G8B8A8);

	uint8* MipData = (uint8*)OutTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, (void*)texture->rubTextureMapData, Height * Width * 4);
	OutTexture->PlatformData->Mips[0].BulkData.Unlock();

	//Setting some Parameters for the Texture and finally returning it
	OutTexture->PlatformData->NumSlices = 1;
	OutTexture->NeverStream = true;
	OutTexture->UpdateResource();

	/*if (bReturnRawTexture)
	{
		OutRawTexture.AddUninitialized(Height * Width * 4);
		FMemory::Memcpy(OutRawTexture.GetData(), (void*)texture->rubTextureMapData, Height * Width * 4);
	}*/

	Result = EAsyncBlueprintResultSwitch::OnSuccess;
	VRRenderModels->FreeTexture(texture);

	VRRenderModels->FreeRenderModel(RenderModel);
	return OutTexture;
#endif
}

