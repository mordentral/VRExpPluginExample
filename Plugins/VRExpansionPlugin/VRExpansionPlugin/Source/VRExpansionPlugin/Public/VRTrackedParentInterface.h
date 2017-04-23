// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "VRBPDatatypes.h"
#include "UObject/Interface.h"

#include "VRTrackedParentInterface.generated.h"


UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class VREXPANSIONPLUGIN_API UVRTrackedParentInterface: public UInterface
{
	GENERATED_UINTERFACE_BODY()
};


class VREXPANSIONPLUGIN_API IVRTrackedParentInterface
{
	GENERATED_IINTERFACE_BODY()
 
public:

	// Set a tracked parent
	UFUNCTION(BlueprintCallable, Category = "VRTrackedParentInterface")
	virtual void SetTrackedParent(UPrimitiveComponent * NewParentComponent, float WaistRadius, EBPVRWaistTrackingMode WaistTrackingMode)
	{}

	static void Default_SetTrackedParent_Impl(UPrimitiveComponent * NewParentComponent, float WaistRadius, EBPVRWaistTrackingMode WaistTrackingMode, FBPVRWaistTracking_Info & OptionalWaistTrackingParent, USceneComponent * Self)
	{
		if (!NewParentComponent || !Self)
		{
			OptionalWaistTrackingParent.Clear();
			return;
		}

		// If had a different original tracked parent
		if (OptionalWaistTrackingParent.IsValid())
		{
			// Remove the tick Prerequisite
			Self->RemoveTickPrerequisiteComponent(OptionalWaistTrackingParent.TrackedDevice);
		}

		// Make other component tick first if possible
		if (NewParentComponent->PrimaryComponentTick.TickGroup == Self->PrimaryComponentTick.TickGroup)
		{
			// Make sure the other component isn't depending on this one
			NewParentComponent->RemoveTickPrerequisiteComponent(Self);

			// Add a tick pre-res for ourselves so that we tick after our tracked parent.
			Self->AddTickPrerequisiteComponent(NewParentComponent);
		}

		OptionalWaistTrackingParent.TrackedDevice = NewParentComponent;
		OptionalWaistTrackingParent.RestingRotation = NewParentComponent->RelativeRotation;
		OptionalWaistTrackingParent.RestingRotation.Yaw = 0.0f;
		
		OptionalWaistTrackingParent.TrackingMode = WaistTrackingMode;
		OptionalWaistTrackingParent.WaistRadius = WaistRadius;
	}

	// Returns local transform of the parent relative attachment
	static FTransform Default_GetWaistOrientationAndPosition(FBPVRWaistTracking_Info & WaistTrackingInfo)
	{
		if (!WaistTrackingInfo.IsValid())
			return FTransform::Identity;

		FTransform DeviceTransform = WaistTrackingInfo.TrackedDevice->GetRelativeTransform();

		// Rewind by the initial rotation when the new parent was set, this should be where the tracker rests on the person
		DeviceTransform.ConcatenateRotation(WaistTrackingInfo.RestingRotation.Quaternion().Inverse());
		DeviceTransform.SetScale3D(FVector(1, 1, 1));

		// Don't bother if not set
		if (WaistTrackingInfo.WaistRadius > 0.0f)
		{
			DeviceTransform.AddToTranslation(DeviceTransform.GetRotation().RotateVector(FVector(-WaistTrackingInfo.WaistRadius, 0, 0)));
		}
		

		// This changes the forward vector to be correct
		// I could pre do it by changed the yaw in resting mode to these values
		switch (WaistTrackingInfo.TrackingMode)
		{
		case EBPVRWaistTrackingMode::VRWaist_Tracked_Front: DeviceTransform.ConcatenateRotation(FRotator(0, 0, 0).Quaternion()); break;
		case EBPVRWaistTrackingMode::VRWaist_Tracked_Rear: DeviceTransform.ConcatenateRotation(FRotator(0, -180, 0).Quaternion()); break;
		case EBPVRWaistTrackingMode::VRWaist_Tracked_Left: DeviceTransform.ConcatenateRotation(FRotator(0, 90, 0).Quaternion()); break;
		case EBPVRWaistTrackingMode::VRWaist_Tracked_Right:	DeviceTransform.ConcatenateRotation(FRotator(0, -90, 0).Quaternion()); break;
		}
		

		return DeviceTransform;
	}
};