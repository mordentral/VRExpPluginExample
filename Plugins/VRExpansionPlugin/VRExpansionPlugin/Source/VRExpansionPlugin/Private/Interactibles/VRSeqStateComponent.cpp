// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Interactibles/VRSeqStateComponent.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(VRSeqStateComponent)

#include "VRExpansionFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "GripMotionControllerComponent.h"
#include "Net/UnrealNetwork.h"

  //=============================================================================
UVRSeqStateComponent::UVRSeqStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	this->SetGenerateOverlapEvents(true);
	this->PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	bRepGameplayTags = false;

	// Defaulting these true so that they work by default in networked environments
	bReplicateMovement = true;

	MovementReplicationSetting = EGripMovementReplicationSettings::ForceClientSideMovement;
	BreakDistance = 100.0f;

	InitialRelativeTransform = FTransform::Identity;
	bDenyGripping = false;

	bUpdateInTick = false;
	bPassThrough = false;

	MinSlideDistance = FVector::ZeroVector;
	MaxSlideDistance = FVector(10.0f, 0.f, 0.f);
	CurrentSliderProgress = 0.0f;

	InitialInteractorLocation = FVector::ZeroVector;
	InitialGripLoc = FVector::ZeroVector;

	bSlideDistanceIsInParentSpace = true;
	bIsLocked = false;
	bAutoDropWhenLocked = true;

	PrimarySlotRange = 100.f;
	SecondarySlotRange = 100.f;
	GripPriority = 1;
	LastSliderProgressState = -1.0f;

	// Set to only overlap with things so that its not ruined by touching over actors
	this->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
}

//=============================================================================
UVRSeqStateComponent::~UVRSeqStateComponent()
{
}


void UVRSeqStateComponent::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVRSeqStateComponent, InitialRelativeTransform);
	//DOREPLIFETIME_CONDITION(UVRSeqStateComponent, bIsLerping, COND_InitialOnly);

	DOREPLIFETIME(UVRSeqStateComponent, bRepGameplayTags);
	DOREPLIFETIME(UVRSeqStateComponent, bReplicateMovement);
	DOREPLIFETIME_CONDITION(UVRSeqStateComponent, GameplayTags, COND_Custom);
}

void UVRSeqStateComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Replicate the levers initial transform if we are replicating movement
	//DOREPLIFETIME_ACTIVE_OVERRIDE(UVRSeqStateComponent, InitialRelativeTransform, bReplicateMovement);
	//DOREPLIFETIME_ACTIVE_OVERRIDE(UVRSeqStateComponent, SplineComponentToFollow, bReplicateMovement);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(UVRSeqStateComponent, GameplayTags, bRepGameplayTags);

	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeLocation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeRotation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeScale3D, bReplicateMovement);
}

void UVRSeqStateComponent::OnRegister()
{
	Super::OnRegister();

	// Init the slider settings
	ResetInitialSliderLocation();
}

void UVRSeqStateComponent::BeginPlay()
{
	// Call the base class 
	Super::BeginPlay();

	CalculateSliderProgress();

	bOriginalReplicatesMovement = bReplicateMovement;
}

void UVRSeqStateComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	// Call supers tick (though I don't think any of the base classes to this actually implement it)
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsHeld && bUpdateInTick && HoldingGrip.HoldingController)
	{
		FBPActorGripInformation GripInfo;
		EBPVRResultSwitch Result;
		HoldingGrip.HoldingController->GetGripByID(GripInfo, HoldingGrip.GripID, Result);

		if (Result == EBPVRResultSwitch::OnSucceeded)
		{
			bPassThrough = true;
			TickGrip_Implementation(HoldingGrip.HoldingController, GripInfo, DeltaTime);
			bPassThrough = false;
		}
		return;
	}

	// If we are locked then end the lerp, no point
	if (bIsLocked)
	{
		// Notify the end user
		this->SetComponentTickEnabled(false);
		bReplicateMovement = bOriginalReplicatesMovement;

		return;
	}
}

void UVRSeqStateComponent::TickGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, float DeltaTime) 
{

	// Skip this tick if its not triggered from the pass through
	if (bUpdateInTick && !bPassThrough)
		return;

	// If the sliders progress is locked then just exit early
	if (bIsLocked)
	{
		if (bAutoDropWhenLocked)
		{
			// Check if we should auto drop
			CheckAutoDrop(GrippingController, GripInformation);
		}

		return;
	}

	// Handle manual tracking here
	FTransform ParentTransform = UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(this);
	FTransform CurrentRelativeTransform = InitialRelativeTransform * ParentTransform;
	FVector CurInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(GrippingController->GetPivotLocation());

	FVector CalculatedLocation = InitialGripLoc + (CurInteractorLocation - InitialInteractorLocation);
	
	float SplineProgress = CurrentSliderProgress;
	FVector ClampedLocation = ClampSlideVector(CalculatedLocation);
	this->SetRelativeLocation(InitialRelativeTransform.TransformPosition(ClampedLocation));
	CurrentSliderProgress = GetCurrentSliderProgress(bSlideDistanceIsInParentSpace ? ClampedLocation * InitialRelativeTransform.GetScale3D() : ClampedLocation);

	CheckSliderProgress();

	// Check if we should auto drop
	CheckAutoDrop(GrippingController, GripInformation);
}

bool UVRSeqStateComponent::CheckAutoDrop(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation)
{
	// Converted to a relative value now so it should be correct
	if (BreakDistance > 0.f && GrippingController->HasGripAuthority(GripInformation) && FVector::DistSquared(InitialDropLocation, this->GetComponentTransform().InverseTransformPosition(GrippingController->GetPivotLocation())) >= FMath::Square(BreakDistance))
	{
		if (GrippingController->OnGripOutOfRange.IsBound())
		{
			uint8 GripID = GripInformation.GripID;
			GrippingController->OnGripOutOfRange.Broadcast(GripInformation, GripInformation.GripDistance);
		}
		else
		{
			GrippingController->DropObjectByInterface(this, HoldingGrip.GripID);
		}
		return true;
	}

	return false;
}

void UVRSeqStateComponent::CheckSliderProgress()
{
 // Implement progress here

}

void UVRSeqStateComponent::OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) 
{
	FTransform CurrentRelativeTransform = InitialRelativeTransform * UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(this);

	// This lets me use the correct original location over the network without changes
	FTransform ReversedRelativeTransform = FTransform(GripInformation.RelativeTransform.ToInverseMatrixWithScale());
	FTransform RelativeToGripTransform = ReversedRelativeTransform * this->GetComponentTransform();

	InitialInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(RelativeToGripTransform.GetTranslation());
	InitialGripLoc = InitialRelativeTransform.InverseTransformPosition(this->GetRelativeLocation());
	InitialDropLocation = ReversedRelativeTransform.GetTranslation();

	if (GripInformation.GripMovementReplicationSetting != EGripMovementReplicationSettings::ForceServerSideMovement)
	{
		bReplicateMovement = false;
	}

	if (bUpdateInTick)
		SetComponentTickEnabled(true);

	//OnGripped.Broadcast(GrippingController, GripInformation);

}

void UVRSeqStateComponent::OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) 
{
	//this->SetComponentTickEnabled(false);
	// #TODO: Handle letting go and how lerping works, specifically with the snap points it may be an issue
	this->SetComponentTickEnabled(false);
	bReplicateMovement = bOriginalReplicatesMovement;


	//OnDropped.Broadcast(ReleasingController, GripInformation, bWasSocketed);
}

void UVRSeqStateComponent::SetIsLocked(bool bNewLockedState)
{
	bIsLocked = bNewLockedState;
}

void UVRSeqStateComponent::SetGripPriority(int NewGripPriority)
{
	GripPriority = NewGripPriority;
}

void UVRSeqStateComponent::OnChildGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) {}
void UVRSeqStateComponent::OnChildGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) {}
void UVRSeqStateComponent::OnSecondaryGrip_Implementation(UGripMotionControllerComponent * GripOwningController, USceneComponent * SecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UVRSeqStateComponent::OnSecondaryGripRelease_Implementation(UGripMotionControllerComponent * GripOwningController, USceneComponent * ReleasingSecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UVRSeqStateComponent::OnUsed_Implementation() {}
void UVRSeqStateComponent::OnEndUsed_Implementation() {}
void UVRSeqStateComponent::OnSecondaryUsed_Implementation() {}
void UVRSeqStateComponent::OnEndSecondaryUsed_Implementation() {}
void UVRSeqStateComponent::OnInput_Implementation(FKey Key, EInputEvent KeyEvent) {}
bool UVRSeqStateComponent::RequestsSocketing_Implementation(USceneComponent *& ParentToSocketTo, FName & OptionalSocketName, FTransform_NetQuantize & RelativeTransform) { return false; }

bool UVRSeqStateComponent::DenyGripping_Implementation(UGripMotionControllerComponent * GripInitiator)
{
	return bDenyGripping;
}

EGripInterfaceTeleportBehavior UVRSeqStateComponent::TeleportBehavior_Implementation()
{
	return EGripInterfaceTeleportBehavior::DropOnTeleport;
}

bool UVRSeqStateComponent::SimulateOnDrop_Implementation()
{
	return false;
}

/*EGripCollisionType UVRSeqStateComponent::SlotGripType_Implementation()
{
	return EGripCollisionType::CustomGrip;
}

EGripCollisionType UVRSeqStateComponent::FreeGripType_Implementation()
{
	return EGripCollisionType::CustomGrip;
}*/

EGripCollisionType UVRSeqStateComponent::GetPrimaryGripType_Implementation(bool bIsSlot)
{
	return EGripCollisionType::CustomGrip;
}

ESecondaryGripType UVRSeqStateComponent::SecondaryGripType_Implementation()
{
	return ESecondaryGripType::SG_None;
}


EGripMovementReplicationSettings UVRSeqStateComponent::GripMovementReplicationType_Implementation()
{
	return MovementReplicationSetting;
}

EGripLateUpdateSettings UVRSeqStateComponent::GripLateUpdateSetting_Implementation()
{
	return EGripLateUpdateSettings::LateUpdatesAlwaysOff;
}

/*float UVRSeqStateComponent::GripStiffness_Implementation()
{
	return 0.0f;
}

float UVRSeqStateComponent::GripDamping_Implementation()
{
	return 0.0f;
}*/
void UVRSeqStateComponent::GetGripStiffnessAndDamping_Implementation(float &GripStiffnessOut, float &GripDampingOut)
{
	GripStiffnessOut = 0.0f;
	GripDampingOut = 0.0f;
}

FBPAdvGripSettings UVRSeqStateComponent::AdvancedGripSettings_Implementation()
{
	return FBPAdvGripSettings(GripPriority);
}

float UVRSeqStateComponent::GripBreakDistance_Implementation()
{
	return BreakDistance;
}

/*void UVRSeqStateComponent::ClosestSecondarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	bHadSlotInRange = false;
}

void UVRSeqStateComponent::ClosestPrimarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	bHadSlotInRange = false;
}*/

void UVRSeqStateComponent::ClosestGripSlotInRange_Implementation(FVector WorldLocation, bool bSecondarySlot, bool & bHadSlotInRange, FTransform & SlotWorldTransform, FName & SlotName, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	if (OverridePrefix.IsNone())
		bSecondarySlot ? OverridePrefix = "VRGripS" : OverridePrefix = "VRGripP";

	UVRExpansionFunctionLibrary::GetGripSlotInRangeByTypeName_Component(OverridePrefix, this, WorldLocation, bSecondarySlot ? SecondarySlotRange : PrimarySlotRange, bHadSlotInRange, SlotWorldTransform, SlotName, CallingController);
}

bool UVRSeqStateComponent::AllowsMultipleGrips_Implementation()
{
	return false;
}

void UVRSeqStateComponent::IsHeld_Implementation(TArray<FBPGripPair> & CurHoldingControllers, bool & bCurIsHeld)
{
	CurHoldingControllers.Empty();
	if (HoldingGrip.IsValid())
	{
		CurHoldingControllers.Add(HoldingGrip);
		bCurIsHeld = bIsHeld;
	}
	else
	{
		bCurIsHeld = false;
	}
}

void UVRSeqStateComponent::Native_NotifyThrowGripDelegates(UGripMotionControllerComponent* Controller, bool bGripped, const FBPActorGripInformation& GripInformation, bool bWasSocketed)
{
	if (bGripped)
	{
		OnGripped.Broadcast(Controller, GripInformation);
	}
	else
	{
		OnDropped.Broadcast(Controller, GripInformation, bWasSocketed);
	}
}

void UVRSeqStateComponent::SetHeld_Implementation(UGripMotionControllerComponent * NewHoldingController, uint8 GripID, bool bNewIsHeld)
{
	if (bNewIsHeld)
	{
		HoldingGrip = FBPGripPair(NewHoldingController, GripID);
		if (MovementReplicationSetting != EGripMovementReplicationSettings::ForceServerSideMovement)
		{
			if (!bIsHeld)
				bOriginalReplicatesMovement = bReplicateMovement;
			bReplicateMovement = false;
		}
	}
	else
	{
		HoldingGrip.Clear();
		if (MovementReplicationSetting != EGripMovementReplicationSettings::ForceServerSideMovement)
		{
			bReplicateMovement = bOriginalReplicatesMovement;
		}
	}

	bIsHeld = bNewIsHeld;
}

/*FBPInteractionSettings UVRSeqStateComponent::GetInteractionSettings_Implementation()
{
	return FBPInteractionSettings();
}*/

bool UVRSeqStateComponent::GetGripScripts_Implementation(TArray<UVRGripScriptBase*> & ArrayReference)
{
	return false;
}

FVector UVRSeqStateComponent::ClampSlideVector(FVector ValueToClamp)
{
	FVector fScaleFactor = FVector(1.0f);

	if (bSlideDistanceIsInParentSpace)
		fScaleFactor = fScaleFactor / InitialRelativeTransform.GetScale3D();

	FVector MinScale = MinSlideDistance.GetAbs() * fScaleFactor;

	FVector Dist = (MinSlideDistance.GetAbs() + MaxSlideDistance.GetAbs()) * fScaleFactor;
	FVector Progress = (ValueToClamp - (-MinScale)) / Dist;

	Progress.X = FMath::Clamp(Progress.X, 0.f, 1.f);
	Progress.Y = FMath::Clamp(Progress.Y, 0.f, 1.f);
	Progress.Z = FMath::Clamp(Progress.Z, 0.f, 1.f);


	return (Progress * Dist) - (MinScale);
}


float UVRSeqStateComponent::GetCurrentSliderProgress(FVector CurLocation, bool bUseKeyInstead, float CurKey)
{
	// Should need the clamp normally, but if someone is manually setting locations it could go out of bounds
	float Progress = 0.f;
	

	Progress = FMath::Clamp(FVector::Dist(-MinSlideDistance.GetAbs(), CurLocation) / FVector::Dist(-MinSlideDistance.GetAbs(), MaxSlideDistance.GetAbs()), 0.0f, 1.0f);
	
	return Progress;
}


void UVRSeqStateComponent::ResetInitialSliderLocation()
{
	// Get our initial relative transform to our parent (or not if un-parented).
	InitialRelativeTransform = this->GetRelativeTransform();
	CurrentSliderProgress = GetCurrentSliderProgress(FVector(0, 0, 0));
}

void UVRSeqStateComponent::SetSliderProgress(float NewSliderProgress)
{
	NewSliderProgress = FMath::Clamp(NewSliderProgress, 0.0f, 1.0f);

	// Doing it min+max because the clamp value subtracts the min value
	FVector CalculatedLocation = FMath::Lerp(-MinSlideDistance.GetAbs(), MaxSlideDistance.GetAbs(), NewSliderProgress);

	if (bSlideDistanceIsInParentSpace)
		CalculatedLocation *= FVector(1.0f) / InitialRelativeTransform.GetScale3D();

	FVector ClampedLocation = ClampSlideVector(CalculatedLocation);

	//if (bSlideDistanceIsInParentSpace)
	//	this->SetRelativeLocation(InitialRelativeTransform.TransformPositionNoScale(ClampedLocation));
	//else
	this->SetRelativeLocation(InitialRelativeTransform.TransformPosition(ClampedLocation));

	CurrentSliderProgress = NewSliderProgress;
}

float UVRSeqStateComponent::CalculateSliderProgress()
{

	FTransform ParentTransform = UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(this);
	FTransform CurrentRelativeTransform = InitialRelativeTransform * ParentTransform;
	FVector CalculatedLocation = CurrentRelativeTransform.InverseTransformPosition(this->GetComponentLocation());

	CurrentSliderProgress = GetCurrentSliderProgress(bSlideDistanceIsInParentSpace ? CalculatedLocation * InitialRelativeTransform.GetScale3D() : CalculatedLocation);

	return CurrentSliderProgress;
}