// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Object.h"
#include "Engine/Texture.h"
#include "Engine/EngineTypes.h"
#include "RHI.h"
//#include "EngineMinimal.h"
#include "IMotionController.h"
#include "OpenXRHmd/Private/OpenXRHMD.h"
#include "OpenXRHmd/Private/OpenXRHMDPrivate.h"
#include <openxr/openxr.h>
//#include "VRBPDatatypes.h"

//Re-defined here as I can't load ISteamVRPlugin on non windows platforms
// Make sure to update if it changes

#include "HeadMountedDisplayFunctionLibrary.h"
#include "IHeadMountedDisplay.h"

#include "OpenXRExpansionFunctionLibrary.generated.h"

//General Advanced Sessions Log
DECLARE_LOG_CATEGORY_EXTERN(OpenXRExpansionFunctionLibraryLog, Log, All);

// This needs to be updated as the original gets changed, that or hope they make the original blueprint accessible.
UENUM(Blueprintable)
enum class EBPOpenXRHMDDeviceType : uint8
{
	DT_SteamVR,
	DT_ValveIndex,
	DT_Vive,
	DT_ViveCosmos,
	DT_OculusQuestHMD,
	DT_OculusHMD,
	DT_WindowsMR,
	//DT_OSVR,
	DT_Unknown
};

// This needs to be updated as the original gets changed, that or hope they make the original blueprint accessible.
UENUM(Blueprintable)
enum class EBPOpenXRControllerDeviceType : uint8
{
	DT_IndexController,
	DT_ViveController,
	DT_CosmosController,
	DT_RiftController,
	DT_RiftSController,
	DT_QuestController,
	DT_WMRController,
	DT_UnknownController
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class OPENXREXPANSIONPLUGIN_API UOpenXRExpansionFunctionLibrary : public UBlueprintFunctionLibrary
{
	//GENERATED_BODY()
	GENERATED_BODY()

public:
	UOpenXRExpansionFunctionLibrary(const FObjectInitializer& ObjectInitializer);

	~UOpenXRExpansionFunctionLibrary();
public:

	FOpenXRHMD* GetOpenXRHMD() const
	{
		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			return static_cast<FOpenXRHMD*>(GEngine->XRSystem.Get());
		}

		return nullptr;
	}

	// Checks if a specific OpenVR device is connected, index names are assumed, they may not be exact
	UFUNCTION(BlueprintCallable, Category = "VRExpansionFunctions|OpenXR", meta = (bIgnoreSelf = "true"))
	static bool GetDeviceName()
	{
		/*if (FOpenXRHMD* XRSystem = GetOpenXRHMD())
		{
			if (XRSystem->IsInitialized())
			{
				
				XrSession ourXRSession = XRSystem->GetSession();
				XrInstance ourXRInstance = XRSystem->GetInstance();
				
				TArray<int32> OutDevices;
				XRSystem->EnumerateTrackedDevices(OutDevices, EXRTrackedDeviceType::Controller);
			}
		}*/
		return false;
	}

};	
