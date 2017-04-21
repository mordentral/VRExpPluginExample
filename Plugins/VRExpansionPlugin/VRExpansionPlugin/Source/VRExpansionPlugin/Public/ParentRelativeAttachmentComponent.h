// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
#include "ParentRelativeAttachmentComponent.generated.h"


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = VRExpansionLibrary)
class VREXPANSIONPLUGIN_API UParentRelativeAttachmentComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRExpansionLibrary", meta = (ClampMin = "0", UIMin = "0"))
	float YawTolerance;

	FRotator LastRot;

	// If true will subtract the HMD's location from the position, useful for if the actors base is set to the HMD location always (simple character).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRExpansionLibrary")
	bool bOffsetByHMD;


	// If valid will use this as the tracked parent instead of the HMD / Parent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRExpansionLibrary")
	FBPVRWaistTracking_Info OptionalWaistTrackingParent;


	// Set a tracked parent
	UFUNCTION(BlueprintCallable, Category = "VRExpansionLibrary")
	void SetTrackedParent(UPrimitiveComponent * NewParentComponent, EBPVRWaistTrackingMode WaistTrackingMode, float WaistRadius)
	{
		if (!NewParentComponent)
		{
			OptionalWaistTrackingParent.Clear();
			return;
		}

		// If had a different original tracked parent
		if (OptionalWaistTrackingParent.IsValid())
		{
			// Remove the tick Prerequisite
			this->RemoveTickPrerequisiteComponent(OptionalWaistTrackingParent.TrackedDevice);
		}

		// Make other component tick first if possible
		if (NewParentComponent->PrimaryComponentTick.TickGroup == this->PrimaryComponentTick.TickGroup)
		{
			this->AddTickPrerequisiteComponent(NewParentComponent);
		}

		OptionalWaistTrackingParent.TrackedDevice = NewParentComponent;
		OptionalWaistTrackingParent.RestingRotation = NewParentComponent->RelativeRotation;
		OptionalWaistTrackingParent.TrackingMode = WaistTrackingMode;
		OptionalWaistTrackingParent.WaistRadius = WaistRadius;
	}

	// Returns local transform of the parent relative attachment
	FTransform GetWaistOrientationAndPosition(FBPVRWaistTracking_Info & WaistTrackingInfo)
	{
		if (!WaistTrackingInfo.IsValid())
			return FTransform::Identity;

		FTransform DeviceTransform = WaistTrackingInfo.TrackedDevice->GetRelativeTransform();

		// Rewind by the initial rotation when the new parent was set, this should be where the tracker rests on the person
		DeviceTransform.ConcatenateRotation(WaistTrackingInfo.RestingRotation.Quaternion().Inverse());
		DeviceTransform.SetScale3D(FVector(1,1,1));

		// Don't bother if not set
		if (WaistTrackingInfo.WaistRadius > 0.0f)
		{
			// Get rotation
			FQuat rot = DeviceTransform.GetRotation();

			switch (WaistTrackingInfo.TrackingMode)
			{
			case EBPVRWaistTrackingMode::VRWaist_Tracked_Front:
				DeviceTransform.AddToTranslation(rot.RotateVector(FVector(-WaistTrackingInfo.WaistRadius, 0, 0))); break;
			case EBPVRWaistTrackingMode::VRWaist_Tracked_Rear:
				DeviceTransform.AddToTranslation(rot.RotateVector(FVector(WaistTrackingInfo.WaistRadius, 0, 0))); break;
			case EBPVRWaistTrackingMode::VRWaist_Tracked_Left:
				DeviceTransform.AddToTranslation(rot.RotateVector(FVector(0, -WaistTrackingInfo.WaistRadius, 0))); break;
			case EBPVRWaistTrackingMode::VRWaist_Tracked_Right:
				DeviceTransform.AddToTranslation(rot.RotateVector(FVector(0, WaistTrackingInfo.WaistRadius, 0))); break;
			default:break;
			}
		}

		return DeviceTransform;
	}


	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	bool IsLocallyControlled() const
	{
		// I like epics implementation better than my own
		const AActor* MyOwner = GetOwner();
		const APawn* MyPawn = Cast<APawn>(MyOwner);
		return MyPawn ? MyPawn->IsLocallyControlled() : (MyOwner->Role == ENetRole::ROLE_Authority);
	}
};

