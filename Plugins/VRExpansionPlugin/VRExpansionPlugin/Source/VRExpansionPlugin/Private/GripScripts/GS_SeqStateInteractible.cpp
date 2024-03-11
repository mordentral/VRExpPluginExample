// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Interactibles/VRSeqStateComponent.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(VRSeqStateComponent)

#include "VRExpansionFunctionLibrary.h"
#include "Components/SplineComponent.h"
#include "GripMotionControllerComponent.h"
#include "Net/UnrealNetwork.h"

  //=============================================================================
UGS_SeqStateInteractible::UGS_SeqStateInteractible(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/*this->SetGenerateOverlapEvents(true);
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
	this->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);*/
}

//=============================================================================
UGS_SeqStateInteractible::~UGS_SeqStateInteractible()
{
}

/*
void UGS_SeqStateInteractible::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGS_SeqStateInteractible, InitialRelativeTransform);
	//DOREPLIFETIME_CONDITION(UGS_SeqStateInteractible, bIsLerping, COND_InitialOnly);

	DOREPLIFETIME(UGS_SeqStateInteractible, bRepGameplayTags);
	DOREPLIFETIME(UGS_SeqStateInteractible, bReplicateMovement);
	DOREPLIFETIME_CONDITION(UGS_SeqStateInteractible, GameplayTags, COND_Custom);
}

void UGS_SeqStateInteractible::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Replicate the levers initial transform if we are replicating movement
	//DOREPLIFETIME_ACTIVE_OVERRIDE(UGS_SeqStateInteractible, InitialRelativeTransform, bReplicateMovement);
	//DOREPLIFETIME_ACTIVE_OVERRIDE(UGS_SeqStateInteractible, SplineComponentToFollow, bReplicateMovement);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(UGS_SeqStateInteractible, GameplayTags, bRepGameplayTags);

	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeLocation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeRotation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(USceneComponent, RelativeScale3D, bReplicateMovement);
}

void UGS_SeqStateInteractible::OnRegister()
{
	Super::OnRegister();

	// Init the slider settings
	ResetInitialSliderLocation();
}

void UGS_SeqStateInteractible::BeginPlay()
{
	// Call the base class 
	Super::BeginPlay();

	CalculateSliderProgress();

	bOriginalReplicatesMovement = bReplicateMovement;
}

void UGS_SeqStateInteractible::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
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

void UGS_SeqStateInteractible::TickGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, float DeltaTime) 
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

bool UGS_SeqStateInteractible::CheckAutoDrop(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation)
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

void UGS_SeqStateInteractible::CheckSliderProgress()
{
 // Implement progress here

}

void UGS_SeqStateInteractible::OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) 
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

void UGS_SeqStateInteractible::OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) 
{
	//this->SetComponentTickEnabled(false);
	// #TODO: Handle letting go and how lerping works, specifically with the snap points it may be an issue
	this->SetComponentTickEnabled(false);
	bReplicateMovement = bOriginalReplicatesMovement;


	//OnDropped.Broadcast(ReleasingController, GripInformation, bWasSocketed);
}

void UGS_SeqStateInteractible::SetIsLocked(bool bNewLockedState)
{
	bIsLocked = bNewLockedState;
}

void UGS_SeqStateInteractible::SetGripPriority(int NewGripPriority)
{
	GripPriority = NewGripPriority;
}

void UGS_SeqStateInteractible::OnChildGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) {}
void UGS_SeqStateInteractible::OnChildGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) {}
void UGS_SeqStateInteractible::OnSecondaryGrip_Implementation(UGripMotionControllerComponent * GripOwningController, USceneComponent * SecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UGS_SeqStateInteractible::OnSecondaryGripRelease_Implementation(UGripMotionControllerComponent * GripOwningController, USceneComponent * ReleasingSecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UGS_SeqStateInteractible::OnUsed_Implementation() {}
void UGS_SeqStateInteractible::OnEndUsed_Implementation() {}
void UGS_SeqStateInteractible::OnSecondaryUsed_Implementation() {}
void UGS_SeqStateInteractible::OnEndSecondaryUsed_Implementation() {}
void UGS_SeqStateInteractible::OnInput_Implementation(FKey Key, EInputEvent KeyEvent) {}
bool UGS_SeqStateInteractible::RequestsSocketing_Implementation(USceneComponent *& ParentToSocketTo, FName & OptionalSocketName, FTransform_NetQuantize & RelativeTransform) { return false; }

bool UGS_SeqStateInteractible::DenyGripping_Implementation(UGripMotionControllerComponent * GripInitiator)
{
	return bDenyGripping;
}

EGripInterfaceTeleportBehavior UGS_SeqStateInteractible::TeleportBehavior_Implementation()
{
	return EGripInterfaceTeleportBehavior::DropOnTeleport;
}

bool UGS_SeqStateInteractible::SimulateOnDrop_Implementation()
{
	return false;
}

EGripCollisionType UGS_SeqStateInteractible::GetPrimaryGripType_Implementation(bool bIsSlot)
{
	return EGripCollisionType::CustomGrip;
}

ESecondaryGripType UGS_SeqStateInteractible::SecondaryGripType_Implementation()
{
	return ESecondaryGripType::SG_None;
}


EGripMovementReplicationSettings UGS_SeqStateInteractible::GripMovementReplicationType_Implementation()
{
	return MovementReplicationSetting;
}

EGripLateUpdateSettings UGS_SeqStateInteractible::GripLateUpdateSetting_Implementation()
{
	return EGripLateUpdateSettings::LateUpdatesAlwaysOff;
}

void UGS_SeqStateInteractible::GetGripStiffnessAndDamping_Implementation(float &GripStiffnessOut, float &GripDampingOut)
{
	GripStiffnessOut = 0.0f;
	GripDampingOut = 0.0f;
}

FBPAdvGripSettings UGS_SeqStateInteractible::AdvancedGripSettings_Implementation()
{
	return FBPAdvGripSettings(GripPriority);
}

float UGS_SeqStateInteractible::GripBreakDistance_Implementation()
{
	return BreakDistance;
}

void UGS_SeqStateInteractible::ClosestGripSlotInRange_Implementation(FVector WorldLocation, bool bSecondarySlot, bool & bHadSlotInRange, FTransform & SlotWorldTransform, FName & SlotName, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	if (OverridePrefix.IsNone())
		bSecondarySlot ? OverridePrefix = "VRGripS" : OverridePrefix = "VRGripP";

	UVRExpansionFunctionLibrary::GetGripSlotInRangeByTypeName_Component(OverridePrefix, this, WorldLocation, bSecondarySlot ? SecondarySlotRange : PrimarySlotRange, bHadSlotInRange, SlotWorldTransform, SlotName, CallingController);
}

bool UGS_SeqStateInteractible::AllowsMultipleGrips_Implementation()
{
	return false;
}

void UGS_SeqStateInteractible::IsHeld_Implementation(TArray<FBPGripPair> & CurHoldingControllers, bool & bCurIsHeld)
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

void UGS_SeqStateInteractible::Native_NotifyThrowGripDelegates(UGripMotionControllerComponent* Controller, bool bGripped, const FBPActorGripInformation& GripInformation, bool bWasSocketed)
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

void UGS_SeqStateInteractible::SetHeld_Implementation(UGripMotionControllerComponent * NewHoldingController, uint8 GripID, bool bNewIsHeld)
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

bool UGS_SeqStateInteractible::GetGripScripts_Implementation(TArray<UVRGripScriptBase*> & ArrayReference)
{
	return false;
}

FVector UGS_SeqStateInteractible::ClampSlideVector(FVector ValueToClamp)
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


float UGS_SeqStateInteractible::GetCurrentSliderProgress(FVector CurLocation, bool bUseKeyInstead, float CurKey)
{
	// Should need the clamp normally, but if someone is manually setting locations it could go out of bounds
	float Progress = 0.f;
	

	Progress = FMath::Clamp(FVector::Dist(-MinSlideDistance.GetAbs(), CurLocation) / FVector::Dist(-MinSlideDistance.GetAbs(), MaxSlideDistance.GetAbs()), 0.0f, 1.0f);
	
	return Progress;
}


void UGS_SeqStateInteractible::ResetInitialSliderLocation()
{
	// Get our initial relative transform to our parent (or not if un-parented).
	InitialRelativeTransform = this->GetRelativeTransform();
	CurrentSliderProgress = GetCurrentSliderProgress(FVector(0, 0, 0));
}

void UGS_SeqStateInteractible::SetSliderProgress(float NewSliderProgress)
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

float UGS_SeqStateInteractible::CalculateSliderProgress()
{

	FTransform ParentTransform = UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(this);
	FTransform CurrentRelativeTransform = InitialRelativeTransform * ParentTransform;
	FVector CalculatedLocation = CurrentRelativeTransform.InverseTransformPosition(this->GetComponentLocation());

	CurrentSliderProgress = GetCurrentSliderProgress(bSlideDistanceIsInParentSpace ? CalculatedLocation * InitialRelativeTransform.GetScale3D() : CalculatedLocation);

	return CurrentSliderProgress;
}*/