#pragma once

#include "VRGlobalSettings.generated.h"


namespace ControllerProfileStatics
{
	static const FTransform OculusTouchStaticOffset(FRotator(-70.f, 0.f, 0.f));
}


USTRUCT(BlueprintType, Category = "VRGlobalSettings")
struct VREXPANSIONPLUGIN_API FBPVRControllerProfile
{
	GENERATED_BODY()
public:

	// Name of controller profile
	UPROPERTY(EditAnywhere, Category = "ControllerProfiles")
		FName ControllerName;

	// Offset to use with this controller
	UPROPERTY(EditAnywhere, Category = "ControllerProfiles")
		FTransform OffsetTransform;

	FBPVRControllerProfile() :
		ControllerName(NAME_None),
		OffsetTransform(FTransform::Identity)
	{}

	FBPVRControllerProfile(FName ControllerName) :
		ControllerName(ControllerName),
		OffsetTransform(FTransform::Identity)
	{}

	FBPVRControllerProfile(FName ControllerName, const FTransform & Offset) :
		ControllerName(ControllerName),
		OffsetTransform(Offset)
	{}

};

UCLASS(config = Engine, defaultconfig)
class VREXPANSIONPLUGIN_API UVRGlobalSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	// Controller profiles to store related information on a per model basis
	UPROPERTY(config, EditAnywhere, Category = "ControllerProfiles")
	TArray<FBPVRControllerProfile> ControllerProfiles;

	// Setting to use for the OneEuro smoothing low pass filter when double gripping something held with a hand
	UPROPERTY(config, EditAnywhere, Category = "Secondary Grip 1Euro Settings")
	float OneEuroMinCutoff;

	// Setting to use for the OneEuro smoothing low pass filter when double gripping something held with a hand
	UPROPERTY(config, EditAnywhere, Category = "Secondary Grip 1Euro Settings")
	float OneEuroCutoffSlope;

	// Setting to use for the OneEuro smoothing low pass filter when double gripping something held with a hand
	UPROPERTY(config, EditAnywhere, Category = "Secondary Grip 1Euro Settings")
	float OneEuroDeltaCutoff;

	// Adjust the transform of a socket for a particular controller model
	UFUNCTION(BlueprintPure, Category = "VRControllerProfiles")
	static FTransform AdjustRelativeTransformByControllerProfile(FName ControllerProfileName, const FTransform & SocketTransform)
	{
		if (ControllerProfileName == NAME_None)
			return SocketTransform;

		const UVRGlobalSettings& VRSettings = *GetDefault<UVRGlobalSettings>();

		const FBPVRControllerProfile * FoundProfile = VRSettings.ControllerProfiles.FindByPredicate([ControllerProfileName](const FBPVRControllerProfile & ArrayItem)
		{
			return ArrayItem.ControllerName == ControllerProfileName;
		});

		if (FoundProfile)
		{
			return SocketTransform * FoundProfile->OffsetTransform;
		}

		return SocketTransform;
	}

	virtual void PostInitProperties() override
	{
#if WITH_EDITOR
		if (ControllerProfiles.Num() == 0)
		{
			ControllerProfiles.Add(FBPVRControllerProfile(TEXT("Vive_Wands")));
			ControllerProfiles.Add(FBPVRControllerProfile(TEXT("Oculus_Touch"), ControllerProfileStatics::OculusTouchStaticOffset));
			this->SaveConfig(CPF_Config, *this->GetDefaultConfigFilename());
		}
#endif
		Super::PostInitProperties();
	}
};
