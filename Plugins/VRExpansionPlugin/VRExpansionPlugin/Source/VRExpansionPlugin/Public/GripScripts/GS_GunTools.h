// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRGripScriptBase.h"
#include "GripScripts/GS_Default.h"
#include "GS_GunTools.generated.h"

USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct VREXPANSIONPLUGIN_API FGunTools_AdvSecondarySettings
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings")
		bool bUseAdvancedSecondarySettings;

	// Used to smooth filter the secondary influence
	FBPEuroLowPassFilter SmoothingOneEuro;

	// Scaler used for handling the smoothing amount, 0.0f is full smoothing, 1.0f is smoothing off
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings", meta = (editcondition = "bUseAdvancedSecondarySettings", ClampMin = "0.00", UIMin = "0.00", ClampMax = "1.00", UIMax = "1.00"))
		float SecondaryGripScaler;

	// If true we will constantly be lerping with the grip scaler instead of only sometimes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings", meta = (editcondition = "bUseAdvancedSecondarySettings"))
		bool bUseConstantGripScaler;

	// Whether to scale the secondary hand influence off of distance from grip point
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings", meta = (editcondition = "bUseAdvancedSecondarySettings"))
		bool bUseSecondaryGripDistanceInfluence;

	// If true, will use the GripInfluenceDeadZone as a constant value instead of calculating the distance and lerping, lets you define a static influence amount.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecondaryGripSettings", meta = (editcondition = "bUseSecondaryGripDistanceInfluence"))
	//	bool bUseGripInfluenceDeadZoneAsConstant;

	// Distance from grip point in local space where there is 100% influence
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings", meta = (editcondition = "bUseSecondaryGripDistanceInfluence", ClampMin = "0.00", UIMin = "0.00", ClampMax = "256.00", UIMax = "256.00"))
		float GripInfluenceDeadZone;

	// Distance from grip point in local space before all influence is lost on the secondary grip (1.0f - 0.0f influence over this range)
	// this comes into effect outside of the deadzone
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AdvSecondarySettings", meta = (editcondition = "bUseSecondaryGripDistanceInfluence", ClampMin = "1.00", UIMin = "1.00", ClampMax = "256.00", UIMax = "256.00"))
		float GripInfluenceDistanceToZero;

	FGunTools_AdvSecondarySettings()
	{
		bUseAdvancedSecondarySettings = false;
		SecondaryGripScaler = 1.0f;
		bUseSecondaryGripDistanceInfluence = false;
		//bUseGripInfluenceDeadZoneAsConstant(false),
		GripInfluenceDeadZone = 50.0f;
		GripInfluenceDistanceToZero = 100.0f;
		bUseConstantGripScaler = false;
	}
};


// This class is currently under development, DO NOT USE
UCLASS(NotBlueprintable, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UGS_GunTools : public UGS_Default
{
	GENERATED_BODY()
public:

	// TODO: shoulder model based on forward of hands from anchor point.
	// Range from anchor to attach too in X/Y/Z axis, larger Y for shoulder span.
	// Consider the anchor point below the head generally?


	UGS_GunTools(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings")
	FGunTools_AdvSecondarySettings AdvSecondarySettings;


	// Offset to apply to the pivot (good for centering pivot into the palm ect).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Pivot")
		FVector_NetQuantize100 PivotOffset;

	UFUNCTION(BlueprintCallable, Category = "GunTools|ShoulderMount")
		void SetShoulderMountComponent(USceneComponent * NewShoulderComponent)
	{
		ShoulderMountComponent = NewShoulderComponent;
	}

	UFUNCTION(BlueprintCallable, Category = "GunTools|ShoulderMount")
		void SetShoulderMounting(bool bAllowShoulderMounting)
	{
		bUseShoulderMounting = bAllowShoulderMounting;
	}

	// Overrides the pivot location to be at this component instead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		bool bUseShoulderMounting;

	FTransform MountWorldTransform;
	bool bIsMounted;

	// Overrides the default behavior of using the HMD location for the mount and uses this component instead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		TWeakObjectPtr<USceneComponent> ShoulderMountComponent;

	// Should we auto snap to the shoulder mount by a set distance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		bool bUseDistanceBasedShoulderSnapping;

	// The distance before snapping to the shoulder / unsnapping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
	 float ShoulderSnapDistance;

	// An offset to apply to the HMD location to be considered the neck / mount pivot 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		FVector_NetQuantize100 ShoulderSnapOffset;

	// If this gun has recoil
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil")
		bool bHasRecoil;

	// Maximum recoil addition
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector_NetQuantize100 MaxRecoilTranslation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector_NetQuantize100 MaxRecoilRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector_NetQuantize100 MaxRecoilScale;

	// Recoil decay rate, how fast it decays back to baseline
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		float DecayRate;

	// Recoil lerp rate, how long it takes to lerp to the target recoil amount (0.0f would be instant)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		float LerpRate;

	// Stores the current amount of recoil
	FTransform BackEndRecoilStorage;

	// Stores the target amount of recoil
	FTransform BackEndRecoilTarget;

	bool bHasActiveRecoil;
	
	UFUNCTION(BlueprintCallable, Category = "GunTools|Recoil")
		void AddRecoilInstance(const FTransform & RecoilAddition);

	UFUNCTION(BlueprintCallable, Category = "GunTools|Recoil")
		void ResetRecoil();

	virtual bool GetWorldTransform_Implementation(UGripMotionControllerComponent * GrippingController, float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface, bool bIsForTeleport) override;

	inline void GunTools_ApplySmoothingAndLerp(FBPActorGripInformation & Grip, FVector &frontLoc, FVector & frontLocOrig, float DeltaTime);
};

