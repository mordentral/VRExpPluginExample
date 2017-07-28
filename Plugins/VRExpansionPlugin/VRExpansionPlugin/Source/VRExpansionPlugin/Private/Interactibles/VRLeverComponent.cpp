// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VRLeverComponent.h"

  //=============================================================================
UVRLeverComponent::UVRLeverComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	this->bGenerateOverlapEvents = true;
	this->PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	bRepGameplayTags = false;

	MovementReplicationSetting = EGripMovementReplicationSettings::ForceClientSideMovement;
	BreakDistance = 200.0f;

	HandleData = nullptr;
	SceneIndex = 0;

	bIsPhysicsLever = true;
	ParentComponent = nullptr;
	LeverRotationAxis = EVRInteractibleLeverAxis::Axis_X;
	
	LeverLimitNegative = 0.0f;
	LeverLimitPositive = 90.0f;
	bLeverState = false;
	LeverTogglePercentage = 0.8f;
	lerpCounter = 0.0f;

	LeverReturnSpeed = 50.0f;
	InitialRelativeTransform = FTransform::Identity;
	InitialInteractorLocation = FVector::ZeroVector;
	InitialGripRot = 0.0f;
	bIsLerping = false;
	bUngripAtTargetRotation = false;
	bLeverReturnsWhenReleased = true;

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
	DOREPLIFETIME_CONDITION(UVRLeverComponent, GameplayTags, COND_Custom);
}

void UVRLeverComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE(UVRLeverComponent, GameplayTags, bRepGameplayTags);
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

	FQuat RotTransform = FQuat::Identity;

	if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
		RotTransform = FRotator(FRotator(0.0, -90.0, 0.0)).Quaternion(); // Correct for roll and DotProduct

	FQuat newInitRot = (InitialRelativeTransform.GetRotation() * RotTransform);

	FVector v1 = (CurrentRelativeTransform.GetRotation() * RotTransform).Vector();
	FVector v2 = (newInitRot).Vector();
	v1.Normalize();
	v2.Normalize();

	FVector CrossP = FVector::CrossProduct(v1, v2);

	//	angle = FMath::RadiansToDegrees(CurrentRelativeTransform.GetRotation().AngularDistance(InitialRelativeTransform.GetRotation()));
	float angle = FMath::RadiansToDegrees(FMath::Atan2(CrossP.Size(), FVector::DotProduct(v1, v2)));
	angle *= FMath::Sign(FVector::DotProduct(CrossP, newInitRot.GetRightVector()));

	CurrentLeverAngle = FMath::RoundToFloat(angle);
	
	if (bIsLerping)
	{
		float LerpedVal = FMath::FInterpConstantTo(angle, 0.0f, DeltaTime, LeverReturnSpeed);
		if (FMath::IsNearlyEqual(LerpedVal, 0.0f))
		{
			this->SetComponentTickEnabled(false);
			this->SetRelativeRotation(InitialRelativeTransform.Rotator());
			CurrentLeverAngle = 0.0f;
		}
		else
		{
			this->SetRelativeRotation((FTransform(SetAxisValue(LerpedVal, FRotator::ZeroRotator)) * InitialRelativeTransform).Rotator());
		}
	}
	else
	{
		bool bNewLeverState = CurrentLeverAngle <= -(LeverLimitNegative * LeverTogglePercentage) || CurrentLeverAngle >= (LeverLimitPositive * LeverTogglePercentage);
		//if (FMath::Abs(CurrentLeverAngle) >= LeverLimit  )
		if (bNewLeverState != bLeverState)
		{
			bLeverState = bNewLeverState;
			OnLeverStateChanged.Broadcast(bLeverState);

			if (bUngripAtTargetRotation && bLeverState && HoldingController)
			{
				HoldingController->DropObjectByInterface(this);
			}
		}
	}
}

void UVRLeverComponent::OnUnregister()
{
	DestroyConstraint();
	Super::OnUnregister();
}

void UVRLeverComponent::TickGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, FVector MControllerLocDelta, float DeltaTime) 
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
		CurrentRelativeTransform = InitialRelativeTransform;//this->GetRelativeTransform();
	}


	FVector CurInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(GrippingController->GetComponentLocation());

	if (GrippingController->HasGripAuthority(GripInformation) && (CurInteractorLocation - InitialInteractorLocation).Size() >= BreakDistance)
	{
		GrippingController->DropObjectByInterface(this);
		return;
	}

	FVector RotVector;
	if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
		RotVector = FRotator(0.0, 90.0, 0.0).UnrotateVector(CurInteractorLocation);
	else
		RotVector = CurInteractorLocation;



	float DeltaAngle = FMath::RadiansToDegrees(FMath::Atan2(RotVector.Y, RotVector.X)) - InitialGripRot;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Delta: %f, Atan: %f, Initial: %f"), DeltaAngle, FMath::RadiansToDegrees(FMath::Atan2(RotVector.Y, RotVector.X)), InitialGripRot));

	this->SetRelativeRotation(SetAxisValue(RotAtGrab + DeltaAngle, this->RelativeRotation));
	
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
	

		FVector RotVector;
		if (LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X)
			RotVector = FRotator(0.0, 90.0, 0.0).UnrotateVector(InitialInteractorLocation);
		else
			RotVector = InitialInteractorLocation;
		
		InitialGripRot = FMath::RadiansToDegrees(FMath::Atan2(RotVector.Y, RotVector.X));
		RotAtGrab = GetAxisValue(this->RelativeRotation);
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

	if(bLeverReturnsWhenReleased)
		bIsLerping = true;
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
	return false;//VRGripInterfaceSettings.bDenyGripping;
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
	return 1500.0f;
}

float UVRLeverComponent::GripDamping_Implementation()
{
	return 200.0f;
}

FBPAdvGripPhysicsSettings UVRLeverComponent::AdvancedPhysicsSettings_Implementation()
{
	return FBPAdvGripPhysicsSettings();
}

float UVRLeverComponent::GripBreakDistance_Implementation()
{
	return BreakDistance;
}

void UVRLeverComponent::ClosestSecondarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, FName OverridePrefix)
{
	bHadSlotInRange = false;
}

void UVRLeverComponent::ClosestPrimarySlotInRange_Implementation(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, FName OverridePrefix)
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
