// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VRLeverComponent.h"
#include "Net/UnrealNetwork.h"

  //=============================================================================
UVRLeverComponent::UVRLeverComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	this->bGenerateOverlapEvents = true;
	this->PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	bRepGameplayTags = false;
	bReplicateMovement = false;

	MovementReplicationSetting = EGripMovementReplicationSettings::ForceClientSideMovement;
	BreakDistance = 100.0f;
	Stiffness = 1500.0f;
	Damping = 200.0f;

	HandleData = nullptr;
	SceneIndex = 0;

	bIsPhysicsLever = false;
	ParentComponent = nullptr;
	LeverRotationAxis = EVRInteractibleLeverAxis::Axis_X;
	
	LeverLimitNegative = 0.0f;
	LeverLimitPositive = 90.0f;
	bLeverState = false;
	LeverTogglePercentage = 0.8f;
	lerpCounter = 0.0f;

	LastDeltaAngle = FVector2D::ZeroVector;

	LeverReturnTypeWhenReleased = EVRInteractibleLeverReturnType::ReturnToZero;
	LeverReturnSpeed = 50.0f;
	bSendLeverEventsDuringLerp = false;

	InitialRelativeTransform = FTransform::Identity;
	InitialInteractorLocation = FVector::ZeroVector;
	InitialGripRot = FVector2D::ZeroVector;
	bIsLerping = false;
	bUngripAtTargetRotation = false;
	bDenyGripping = false;


	// Set to only overlap with things so that its not ruined by touching over actors
	this->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
}

//=============================================================================
UVRLeverComponent::~UVRLeverComponent()
{
}


void UVRLeverComponent::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UVRLeverComponent, bRepGameplayTags);
	DOREPLIFETIME(UVRLeverComponent, bReplicateMovement);
	DOREPLIFETIME_CONDITION(UVRLeverComponent, GameplayTags, COND_Custom);
}

void UVRLeverComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE(UVRLeverComponent, GameplayTags, bRepGameplayTags);

	DOREPLIFETIME_ACTIVE_OVERRIDE(USceneComponent, RelativeLocation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USceneComponent, RelativeRotation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE(USceneComponent, RelativeScale3D, bReplicateMovement);
}

void UVRLeverComponent::BeginPlay()
{
	// Call the base class 
	Super::BeginPlay();

	ResetInitialLeverLocation();
}

void UVRLeverComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	// Call supers tick (though I don't think any of the base classes to this actually implement it)
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FTransform CurrentRelativeTransform;
	if (ParentComponent.IsValid())
	{
		// during grip there is no parent so we do this, might as well do it anyway for lerping as well
		CurrentRelativeTransform = this->GetComponentTransform().GetRelativeTransform(ParentComponent->GetComponentTransform());
	}
	else
	{
		CurrentRelativeTransform = this->GetRelativeTransform();
	}
	
	FVector2D Angle;

	// Get axis angles
	Angle = GetAxisValue((CurrentRelativeTransform * InitialRelativeTransform.Inverse()).Rotator().GetNormalized());

	if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY)
	{
		CurrentLeverAngle = FMath::RoundToFloat(Angle.X);
		CurrentLeverAngleXY.X = CurrentLeverAngle;
		CurrentLeverAngleXY.Y = FMath::RoundToFloat(Angle.Y);
	}
	else if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
	{
		CurrentLeverAngle = FMath::RoundToFloat(Angle.X);
		CurrentLeverAngleXY.X = CurrentLeverAngle;
	}
	else
	{
		CurrentLeverAngle = FMath::RoundToFloat(Angle.Y);
	}



	if (bIsLerping)
	{
	/*	FVector2D TargetAngle = FVector2D::ZeroVector;
		switch (LeverReturnTypeWhenReleased)
		{
		case EVRInteractibleLeverReturnType::LerpToMax:
		{
			if (CurrentLeverAngle >= 0)
				TargetAngle.X = FMath::RoundToFloat(LeverLimitPositive);
			else
				TargetAngle.X = -FMath::RoundToFloat(LeverLimitNegative);

			if (CurrentLeverAngle2 >= 0)
				TargetAngle.Y = FMath::RoundToFloat(LeverLimitPositive);
			else
				TargetAngle.Y = -FMath::RoundToFloat(LeverLimitNegative);
		}break;
		case EVRInteractibleLeverReturnType::LerpToMaxIfOverThreshold:
		{
			if ((!FMath::IsNearlyZero(LeverLimitPositive) && CurrentLeverAngle >= (LeverLimitPositive * LeverTogglePercentage)))
				TargetAngle.X = FMath::RoundToFloat(LeverLimitPositive);
			else if ((!FMath::IsNearlyZero(LeverLimitNegative) && CurrentLeverAngle <= -(LeverLimitNegative * LeverTogglePercentage)))
				TargetAngle.X = -FMath::RoundToFloat(LeverLimitNegative);
			//else - Handled by the default value
			//TargetAngle = 0.0f;

			if ((!FMath::IsNearlyZero(LeverLimitPositive) && CurrentLeverAngle2 >= (LeverLimitPositive * LeverTogglePercentage)))
				TargetAngle.Y = FMath::RoundToFloat(LeverLimitPositive);
			else if ((!FMath::IsNearlyZero(LeverLimitNegative) && CurrentLeverAngle2 <= -(LeverLimitNegative * LeverTogglePercentage)))
				TargetAngle.Y = -FMath::RoundToFloat(LeverLimitNegative);
			//else - Handled by the default value
			//TargetAngle = 0.0f;
		}break;
		case EVRInteractibleLeverReturnType::ReturnToZero:
		default:
		{}break;
		}

		FVector LerpedVal;
		LerpedVal.X = FMath::FixedTurn(Angle.X, TargetAngle.X, LeverReturnSpeed * DeltaTime);
		LerpedVal.Y = FMath::FixedTurn(Angle.Y, TargetAngle.Y, LeverReturnSpeed * DeltaTime);
		//float LerpedVal = FMath::FInterpConstantTo(angle, TargetAngle, DeltaTime, LeverReturnSpeed);
		if (FMath::IsNearlyEqual(LerpedVal, TargetAngle))
		{
			this->SetComponentTickEnabled(false);

			this->SetRelativeRotation((FTransform(SetAxisValue(TargetAngle, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
			
			if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY)
			{
				CurrentLeverAngle = TargetAngle.X;
				CurrentLeverAngleXY = TargetAngle;
			}
			else if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
			{
				CurrentLeverAngle = TargetAngle.X;
				CurrentLeverAngleXY = TargetAngle;
			}
			else
			{
				CurrentLeverAngle = TargetAngle.Y;
				CurrentLeverAngleXY = TargetAngle;
			}
		}
		else
		{
			this->SetRelativeRotation((FTransform(SetAxisValue(LerpedVal, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
		}*/
	}

	bool bNewLeverState = (!FMath::IsNearlyZero(LeverLimitNegative) && CurrentLeverAngle <= -(LeverLimitNegative * LeverTogglePercentage)) || (!FMath::IsNearlyZero(LeverLimitPositive) && CurrentLeverAngle >= (LeverLimitPositive * LeverTogglePercentage));
	//if (FMath::Abs(CurrentLeverAngle) >= LeverLimit  )
	if (bNewLeverState != bLeverState)
	{
		bLeverState = bNewLeverState;

		if(bSendLeverEventsDuringLerp || !bIsLerping)
			OnLeverStateChanged.Broadcast(bLeverState, CurrentLeverAngle >= 0.0f ? EVRInteractibleLeverEventType::LeverPositive : EVRInteractibleLeverEventType::LeverNegative, CurrentLeverAngle);

		if (!bIsLerping && bUngripAtTargetRotation && bLeverState && HoldingController)
		{
			HoldingController->DropObjectByInterface(this);
		}
	}
}

void UVRLeverComponent::OnUnregister()
{
	DestroyConstraint();
	Super::OnUnregister();
}

void UVRLeverComponent::TickGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, float DeltaTime) 
{
	// Handle manual tracking here

	FTransform CurrentRelativeTransform;
	if (ParentComponent.IsValid())
	{
		// during grip there is no parent so we do this, might as well do it anyway for lerping as well
		CurrentRelativeTransform = InitialRelativeTransform * ParentComponent->GetComponentTransform();
	}
	else
	{
		CurrentRelativeTransform = InitialRelativeTransform;
	}

	FVector CurInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(GrippingController->GetComponentLocation());


	FVector2D DeltaAngle;

	if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY)
	{	
		FRotator Rot;

		FVector nAxis;
		float nAngle = 0.0f;

		FQuat::FindBetweenVectors(InitialInteractorLocation, CurInteractorLocation).ToAxisAndAngle(nAxis, nAngle);

		nAngle = FMath::Clamp(nAngle, 0.0f, FMath::DegreesToRadians(LeverLimitPositive));// FMath::DegreesToRadians(FMath::ClampAngle(FMath::RadiansToDegrees(nAngle), 0.0f, LeverLimitPositive));
		Rot = FQuat(nAxis, nAngle).Rotator();

		this->SetRelativeRotation((FTransform(Rot) * InitialRelativeTransform).Rotator());
	}
	else
	{
		DeltaAngle = CalcAngle(LeverRotationAxis, CurInteractorLocation);


		this->SetRelativeRotation((FTransform(SetAxisValue(DeltaAngle, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
		LastDeltaAngle = DeltaAngle;
	}

	// #TODO: This drop code is incorrect, it is based off of the initial point and not the location at grip - revise it at some point
	// Also set it to after rotation
	if (GrippingController->HasGripAuthority(GripInformation) && (this->GetComponentTransform().InverseTransformPosition(GrippingController->GetComponentLocation()) - InitialInteractorDropLocation).Size() >= BreakDistance)
	{
		GrippingController->DropObjectByInterface(this);
		return;
	}
}

void UVRLeverComponent::OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) 
{
	ParentComponent = this->GetAttachParent();

	if (bIsPhysicsLever)
	{
		SetupConstraint();
	}
	else
	{
		FTransform CurrentRelativeTransform;
		if (ParentComponent.IsValid())
		{
			// during grip there is no parent so we do this, might as well do it anyway for lerping as well
			CurrentRelativeTransform = InitialRelativeTransform * ParentComponent->GetComponentTransform();
		}
		else
		{
			CurrentRelativeTransform = InitialRelativeTransform;//this->GetRelativeTransform();
		}

		InitialInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(GrippingController->GetComponentLocation());
		InitialInteractorDropLocation = this->GetComponentTransform().InverseTransformPosition(GrippingController->GetComponentLocation());

		FVector RotVector;
		if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_XY)
		{
			InitialGripRot.X = FMath::RadiansToDegrees(FMath::Atan2(InitialInteractorLocation.Y, InitialInteractorLocation.Z));
			InitialGripRot.Y = FMath::RadiansToDegrees(FMath::Atan2(InitialInteractorLocation.Z, InitialInteractorLocation.X));
		}
		else if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
			InitialGripRot.X = FMath::RadiansToDegrees(FMath::Atan2(InitialInteractorLocation.Y, InitialInteractorLocation.Z));
		else
			InitialGripRot.Y = FMath::RadiansToDegrees(FMath::Atan2(InitialInteractorLocation.Z, InitialInteractorLocation.X));

		RotAtGrab = GetAxisValue(this->GetComponentTransform().GetRelativeTransform(CurrentRelativeTransform).Rotator());
	}

	bIsLerping = false;
	this->SetComponentTickEnabled(true);

}

void UVRLeverComponent::OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation) 
{
	if(bIsPhysicsLever)
	{
		DestroyConstraint();
		FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, true);
		this->AttachToComponent(ParentComponent.Get(), AttachRules);
	}
	
	if (LeverReturnTypeWhenReleased != EVRInteractibleLeverReturnType::Stay)
	{
		bIsLerping = true;
	}
	else
		this->SetComponentTickEnabled(false);
}

void UVRLeverComponent::OnChildGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) {}
void UVRLeverComponent::OnChildGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation) {}
void UVRLeverComponent::OnSecondaryGrip_Implementation(USceneComponent * SecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UVRLeverComponent::OnSecondaryGripRelease_Implementation(USceneComponent * ReleasingSecondaryGripComponent, const FBPActorGripInformation & GripInformation) {}
void UVRLeverComponent::OnUsed_Implementation() {}
void UVRLeverComponent::OnEndUsed_Implementation() {}
void UVRLeverComponent::OnSecondaryUsed_Implementation() {}
void UVRLeverComponent::OnEndSecondaryUsed_Implementation() {}

bool UVRLeverComponent::DenyGripping_Implementation()
{
	return bDenyGripping;
}

EGripInterfaceTeleportBehavior UVRLeverComponent::TeleportBehavior_Implementation()
{
	return EGripInterfaceTeleportBehavior::DropOnTeleport;
}

bool UVRLeverComponent::SimulateOnDrop_Implementation()
{
	return false;
}

EGripCollisionType UVRLeverComponent::SlotGripType_Implementation()
{
	if (bIsPhysicsLever)
		return EGripCollisionType::ManipulationGrip;
	else
		return EGripCollisionType::CustomGrip;
}

EGripCollisionType UVRLeverComponent::FreeGripType_Implementation()
{
	if (bIsPhysicsLever)
		return EGripCollisionType::ManipulationGrip;
	else
		return EGripCollisionType::CustomGrip;
}

ESecondaryGripType UVRLeverComponent::SecondaryGripType_Implementation()
{
	return ESecondaryGripType::SG_None;
}


EGripMovementReplicationSettings UVRLeverComponent::GripMovementReplicationType_Implementation()
{
	return MovementReplicationSetting;
}

EGripLateUpdateSettings UVRLeverComponent::GripLateUpdateSetting_Implementation()
{
	return EGripLateUpdateSettings::LateUpdatesAlwaysOff;
}

float UVRLeverComponent::GripStiffness_Implementation()
{
	return Stiffness;
}

float UVRLeverComponent::GripDamping_Implementation()
{
	return Damping;
}

FBPAdvGripPhysicsSettings UVRLeverComponent::AdvancedPhysicsSettings_Implementation()
{
	return FBPAdvGripPhysicsSettings();
}

float UVRLeverComponent::GripBreakDistance_Implementation()
{
	return BreakDistance;
}

void UVRLeverComponent::ClosestSecondarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	bHadSlotInRange = false;
}

void UVRLeverComponent::ClosestPrimarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController, FName OverridePrefix)
{
	bHadSlotInRange = false;
}

bool UVRLeverComponent::IsInteractible_Implementation()
{
	return false;
}

void UVRLeverComponent::IsHeld_Implementation(UGripMotionControllerComponent *& CurHoldingController, bool & bCurIsHeld)
{
	CurHoldingController = HoldingController;
	bCurIsHeld = bIsHeld;
}

void UVRLeverComponent::SetHeld_Implementation(UGripMotionControllerComponent * NewHoldingController, bool bNewIsHeld)
{
	bIsHeld = bNewIsHeld;

	if (bIsHeld)
		HoldingController = NewHoldingController;
	else
		HoldingController = nullptr;
}

FBPInteractionSettings UVRLeverComponent::GetInteractionSettings_Implementation()
{
	return FBPInteractionSettings();
}
