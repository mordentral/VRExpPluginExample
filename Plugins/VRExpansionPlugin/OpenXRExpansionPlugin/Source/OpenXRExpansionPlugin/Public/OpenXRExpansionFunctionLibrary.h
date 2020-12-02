// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "OpenXRCore.h"
#include "OpenXRHMD.h"
#include "OpenXRExpansionFunctionLibrary.Generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class OPENXREXPANSIONPLUGIN_API UOpenXRExpansionFunctionLibrary : public UBlueprintFunctionLibrary
{
	//GENERATED_BODY()
	GENERATED_BODY()

public:
	UOpenXRExpansionFunctionLibrary(const FObjectInitializer& ObjectInitializer);

	~UOpenXRExpansionFunctionLibrary();
public:

	static FOpenXRHMD* GetOpenXRHMD()
	{
		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			return static_cast<FOpenXRHMD*>(GEngine->XRSystem.Get());
		}

		return nullptr;
	}


	// Get a list of all currently tracked devices and their types, index in the array is their device index
	UFUNCTION(BlueprintCallable, Category = "VRExpansionFunctions|OpenXR", meta = (bIgnoreSelf = "true"))
		static bool GetXRTrackingSystemProperties(int32 & VendorID, FString & SystemName)
	{
		if (FOpenXRHMD* pOpenXRHMD = GetOpenXRHMD())
		{
			XrInstance XRInstance = pOpenXRHMD->GetInstance();
			XrSystemId XRSysID = pOpenXRHMD->GetSystem();

			if (XRSysID && XRInstance)
			{
				XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
				systemProperties.next = nullptr;
				//XR_ENSURE(xrGetSystemProperties(XRInstance, XRSysID, &systemProperties));

				if (xrGetSystemProperties(XRInstance, XRSysID, &systemProperties) == XR_SUCCESS)
				{
					

					/*XrInputSourceLocalizedNameGetInfo InputSourceInfo{ XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO };
					InputSourceInfo.next = nullptr;
					InputSourceInfo.whichComponents = XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT | XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT | XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

					XrPath myPath;
					XrResult PathResult = xrStringToPath(XRInstance, "/user/hand/left", &myPath);
					InputSourceInfo.sourcePath = myPath;
					XrSession XRSesh = pOpenXRHMD->GetSession();
					char buffer[XR_MAX_SYSTEM_NAME_SIZE];
					uint32_t usedBufferCount = 0;
					if (xrGetInputSourceLocalizedName(XRSesh, &InputSourceInfo, XR_MAX_SYSTEM_NAME_SIZE, &usedBufferCount, (char*)&buffer) == XR_SUCCESS)
					{
						int gg = 0;
					}*/

					SystemName = FString(ANSI_TO_TCHAR(systemProperties.systemName));// , XR_MAX_SYSTEM_NAME_SIZE);
					VendorID = systemProperties.vendorId;
					return true;
				}
			}
		}

		SystemName.Empty();
		return false;
	}
};	
