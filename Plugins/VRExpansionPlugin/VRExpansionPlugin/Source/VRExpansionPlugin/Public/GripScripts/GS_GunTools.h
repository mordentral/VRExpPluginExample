// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRGripScriptBase.h"
#include "GripScripts/GS_Default.h"
#include "GS_GunTools.generated.h"

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
	
	// Offset to apply to the pivot (good for centering pivot into the palm ect).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Pivot")
		FVector PivotOffset;

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

	// Relative transform on the gripped object to keep to the shoulder mount
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		FTransform_NetQuantize ShoulderMountRelativeTransform;

	// Overrides the relative transform and uses this socket location instead
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|ShoulderMount")
		FName ShoulderMountSocketOverride;

	// If this gun has recoil
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil")
		bool bHasRecoil;

	// Maximum recoil addition
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector MaxRecoilTranslation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector MaxRecoilRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GunSettings|Recoil", meta = (editcondition = "bHasRecoil"))
		FVector MaxRecoilScale;

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
};
