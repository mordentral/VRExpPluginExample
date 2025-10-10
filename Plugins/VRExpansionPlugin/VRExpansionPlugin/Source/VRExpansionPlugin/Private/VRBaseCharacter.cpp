// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VRBaseCharacter.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(VRBaseCharacter)

#include "VRPlayerController.h"
#include "NavigationSystem.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "ParentRelativeAttachmentComponent.h"
#include "GripMotionControllerComponent.h"
#include "IMotionController.h"
#include "VRRootComponent.h"
#include "VRPathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "XRMotionControllerBase.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "Misc/EngineNetworkCustomVersion.h"


#include "Serializers/SerializerHelpers.h"
#include "Iris/Serialization/NetSerializerDelegates.h"
#include "Iris/Serialization/NetSerializers.h"
#include "Iris/Serialization/PackedVectorNetSerializers.h"
#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
#include "Serializers/FTransformNetQuantizeNetSerializer.h"
//#include "Iris/ReplicationState/ReplicationStateDescriptorBuilder.h"

//#include "Runtime/Engine/Private/EnginePrivate.h"

#if WITH_PUSH_MODEL
#include "Net/Core/PushModel/PushModel.h"
#endif

DEFINE_LOG_CATEGORY(LogBaseVRCharacter);

FName AVRBaseCharacter::LeftMotionControllerComponentName(TEXT("Left Grip Motion Controller"));
FName AVRBaseCharacter::RightMotionControllerComponentName(TEXT("Right Grip Motion Controller"));
FName AVRBaseCharacter::ReplicatedCameraComponentName(TEXT("VR Replicated Camera"));
FName AVRBaseCharacter::ParentRelativeAttachmentComponentName(TEXT("Parent Relative Attachment"));
FName AVRBaseCharacter::SmoothingSceneParentComponentName(TEXT("NetSmoother"));
FName AVRBaseCharacter::VRProxyComponentName(TEXT("VRProxy"));


FRepMovementVRCharacter::FRepMovementVRCharacter()
: Super()
{
	bJustTeleported = false;
	bJustTeleportedGrips = false;
	bPausedTracking = false;
	PausedTrackingLoc = FVector::ZeroVector;
	PausedTrackingRot = 0.f;
	Owner = nullptr;
}

AVRBaseCharacter::AVRBaseCharacter(const FObjectInitializer& ObjectInitializer)
 : Super(ObjectInitializer/*.DoNotCreateDefaultSubobject(ACharacter::MeshComponentName)*/.SetDefaultSubobjectClass<UVRBaseCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))

{

	FRepMovement& MovementRep = GetReplicatedMovement_Mutable();
	
	// Remove the movement jitter with slow speeds
	MovementRep.LocationQuantizationLevel = EVectorQuantization::RoundTwoDecimals;

	if (UCapsuleComponent * cap = GetCapsuleComponent())
	{
		cap->SetCapsuleSize(16.0f, 96.0f);
		cap->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		cap->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	}

	NetSmoother = CreateOptionalDefaultSubobject<USceneComponent>(AVRBaseCharacter::SmoothingSceneParentComponentName);
	if (NetSmoother)
	{
		NetSmoother->SetupAttachment(RootComponent);

		if (!bRetainRoomscale)
		{
			if (UVRRootComponent* MyRoot = Cast<UVRRootComponent>(RootComponent))
			{
				NetSmoother->SetRelativeLocation(MyRoot->GetTargetHeightOffset());
				//VRProxyComponent->SetRelativeLocation(MyRoot->GetTargetHeightOffset());
			}
		}
	}

	VRProxyComponent = CreateOptionalDefaultSubobject<USceneComponent>(AVRBaseCharacter::VRProxyComponentName);
	if (NetSmoother && VRProxyComponent)
	{
		VRProxyComponent->SetupAttachment(NetSmoother);
	}

	VRReplicatedCamera = CreateOptionalDefaultSubobject<UReplicatedVRCameraComponent>(AVRBaseCharacter::ReplicatedCameraComponentName);
	if (VRReplicatedCamera)
	{
		//VRReplicatedCamera->bOffsetByHMD = false;
		VRReplicatedCamera->SetupAttachment(VRProxyComponent ? VRProxyComponent : NetSmoother ? NetSmoother : RootComponent);
		VRReplicatedCamera->OverrideSendTransform = &AVRBaseCharacter::Server_SendTransformCamera;
	}

	VRMovementReference = NULL;
	if (GetMovementComponent())
	{
		VRMovementReference = Cast<UVRBaseCharacterMovementComponent>(GetMovementComponent());
		//AddTickPrerequisiteComponent(this->GetCharacterMovement());
	}

	ParentRelativeAttachment = CreateOptionalDefaultSubobject<UParentRelativeAttachmentComponent>(AVRBaseCharacter::ParentRelativeAttachmentComponentName);
	if (ParentRelativeAttachment && VRReplicatedCamera)
	{
		// Moved this to be root relative as the camera late updates were killing how it worked
		ParentRelativeAttachment->SetupAttachment(VRProxyComponent ? VRProxyComponent : NetSmoother ? NetSmoother : RootComponent);
		//ParentRelativeAttachment->bOffsetByHMD = false;
		ParentRelativeAttachment->AddTickPrerequisiteComponent(VRReplicatedCamera);

		if (USkeletalMeshComponent * SKMesh = GetMesh())
		{
			SKMesh->SetupAttachment(ParentRelativeAttachment);
		}
	}

	LeftMotionController = CreateOptionalDefaultSubobject<UGripMotionControllerComponent>(AVRBaseCharacter::LeftMotionControllerComponentName);
	if (IsValid(LeftMotionController))
	{
		LeftMotionController->SetupAttachment(VRProxyComponent ? VRProxyComponent : NetSmoother ? NetSmoother : RootComponent);
		//LeftMotionController->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
		LeftMotionController->SetTrackingMotionSource(IMotionController::LeftHandSourceId);
		//LeftMotionController->Hand = EControllerHand::Left;
		//LeftMotionController->bOffsetByHMD = false;
		//LeftMotionController->bUpdateInCharacterMovement = true;
		// Keep the controllers ticking after movement
		LeftMotionController->AddTickPrerequisiteComponent(GetCharacterMovement());
		LeftMotionController->OverrideSendTransform = &AVRBaseCharacter::Server_SendTransformLeftController;
	}

	RightMotionController = CreateOptionalDefaultSubobject<UGripMotionControllerComponent>(AVRBaseCharacter::RightMotionControllerComponentName);
	if (IsValid(RightMotionController))
	{
		RightMotionController->SetupAttachment(VRProxyComponent ? VRProxyComponent : NetSmoother ? NetSmoother : RootComponent);
		//RightMotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
		RightMotionController->SetTrackingMotionSource(IMotionController::RightHandSourceId);
		//RightMotionController->Hand = EControllerHand::Right;
		//RightMotionController->bOffsetByHMD = false;
		//RightMotionController->bUpdateInCharacterMovement = true;
		// Keep the controllers ticking after movement
		RightMotionController->AddTickPrerequisiteComponent(GetCharacterMovement());
		RightMotionController->OverrideSendTransform = &AVRBaseCharacter::Server_SendTransformRightController;
	}

	OffsetComponentToWorld = FTransform(FQuat(0.0f, 0.0f, 0.0f, 1.0f), FVector::ZeroVector, FVector(1.0f));


	// Setting a minimum of every frame for replication consideration (UT uses this value for characters and projectiles).
	// Otherwise we will get some massive slow downs if the replication is allowed to hit the 2 per second minimum default
	SetMinNetUpdateFrequency(100.0f);

	// This is for smooth turning, we have more of a use for this than FPS characters do
	// Due to roll/pitch almost never being off 0 for VR the cost is just one byte so i'm fine defaulting it here
	// End users can reset to byte components if they ever want too.
	MovementRep.RotationQuantizationLevel = ERotatorQuantization::ShortComponents;

	VRReplicateCapsuleHeight = false;

	bUseExperimentalUnseatModeFix = true;

	ReplicatedMovementVR.Owner = this;
	bFlagTeleported = false;
	bTrackingPaused = false;
	PausedTrackingLoc = FVector::ZeroVector;
	PausedTrackingRot = 0.f;
}

 void AVRBaseCharacter::PossessedBy(AController* NewController)
 {
	 Super::PossessedBy(NewController);
	 OwningVRPlayerController = Cast<AVRPlayerController>(Controller);
 }

void AVRBaseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	OwningVRPlayerController = Cast<AVRPlayerController>(Controller);
}

void AVRBaseCharacter::OnRep_PlayerState()
{
	OnPlayerStateReplicated_Bind.Broadcast(GetPlayerState());
	Super::OnRep_PlayerState();
}

void AVRBaseCharacter::PostInitializeComponents()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_Character_PostInitComponents);

	Super::PostInitializeComponents();

	if (IsValidChecked(this))
	{
		if (NetSmoother)
		{
			CacheInitialMeshOffset(NetSmoother->GetRelativeLocation(), NetSmoother->GetRelativeRotation());
		}

		if (USkeletalMeshComponent * myMesh = GetMesh())
		{
			// force animation tick after movement component updates
			if (myMesh->PrimaryComponentTick.bCanEverTick && GetMovementComponent())
			{
				myMesh->PrimaryComponentTick.AddPrerequisite(GetMovementComponent(), GetMovementComponent()->PrimaryComponentTick);
			}
		}

		if (GetCharacterMovement() && GetCapsuleComponent())
		{
			GetCharacterMovement()->UpdateNavAgent(*GetCapsuleComponent());
		}

		if (Controller == nullptr && GetNetMode() != NM_Client)
		{
			if (GetCharacterMovement() && GetCharacterMovement()->bRunPhysicsWithNoController)
			{				
				GetCharacterMovement()->SetDefaultMovementMode();
			}
		}
	}
}

/*void AVRBaseCharacter::CacheInitialMeshOffset(FVector MeshRelativeLocation, FRotator MeshRelativeRotation)
{
	BaseTranslationOffset = MeshRelativeLocation;
	BaseRotationOffset = MeshRelativeRotation.Quaternion();

#if ENABLE_NAN_DIAGNOSTIC
	if (BaseRotationOffset.ContainsNaN())
	{
		logOrEnsureNanError(TEXT("ACharacter::PostInitializeComponents detected NaN in BaseRotationOffset! (%s)"), *BaseRotationOffset.ToString());
	}
	
	if (GetMesh())
	{
		const FRotator LocalRotation = GetMesh()->GetRelativeRotation();
		if (LocalRotation.ContainsNaN())
		{
			logOrEnsureNanError(TEXT("ACharacter::PostInitializeComponents detected NaN in Mesh->RelativeRotation! (%s)"), *LocalRotation.ToString());
		}
	}
#endif
}*/

void AVRBaseCharacter::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// For std properties
	FDoRepLifetimeParams PushModelParams{ COND_None, REPNOTIFY_OnChanged, /*bIsPushBased=*/true };

	DOREPLIFETIME_WITH_PARAMS_FAST(AVRBaseCharacter, SeatInformation, PushModelParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AVRBaseCharacter, VRReplicateCapsuleHeight, PushModelParams);

	// For properties with special conditions
	FDoRepLifetimeParams PushModelParamsWithCondition{ COND_SimulatedOnly, REPNOTIFY_OnChanged, /*bIsPushBased=*/true };

	DOREPLIFETIME_WITH_PARAMS_FAST(AVRBaseCharacter, ReplicatedCapsuleHeight, PushModelParamsWithCondition);
	
	DISABLE_REPLICATED_PRIVATE_PROPERTY(AActor, ReplicatedMovement);

	// For properties with special conditions
	FDoRepLifetimeParams PushModelParamsReplicatedMovement{ COND_SimulatedOrPhysics, REPNOTIFY_Always, /*bIsPushBased=*/true };

	DOREPLIFETIME_WITH_PARAMS_FAST(AVRBaseCharacter, ReplicatedMovementVR, PushModelParamsReplicatedMovement);
}

void AVRBaseCharacter::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AVRBaseCharacter, ReplicatedCapsuleHeight, VRReplicateCapsuleHeight);
	DOREPLIFETIME_ACTIVE_OVERRIDE_FAST(AVRBaseCharacter, ReplicatedMovementVR, IsReplicatingMovement());
}

/*USkeletalMeshComponent* AVRBaseCharacter::GetIKMesh_Implementation() const
{
	return GetMesh();
//	return nullptr;
}*/

bool AVRBaseCharacter::Server_SetSeatedMode_Validate(USceneComponent * SeatParent, bool bSetSeatedMode, FTransform_NetQuantize TargetTransform, FTransform_NetQuantize InitialRelCameraTransform, float AllowedRadius, float AllowedRadiusThreshold, bool bZeroToHead, EVRConjoinedMovementModes PostSeatedMovementMode)
{
	return true;
}

void AVRBaseCharacter::Server_SetSeatedMode_Implementation(USceneComponent * SeatParent, bool bSetSeatedMode, FTransform_NetQuantize TargetTransform, FTransform_NetQuantize InitialRelCameraTransform, float AllowedRadius, float AllowedRadiusThreshold, bool bZeroToHead, EVRConjoinedMovementModes PostSeatedMovementMode)
{
	SetSeatedMode(SeatParent, bSetSeatedMode, TargetTransform, InitialRelCameraTransform, AllowedRadius, AllowedRadiusThreshold, bZeroToHead, PostSeatedMovementMode);
}

void AVRBaseCharacter::Server_ReZeroSeating_Implementation(FTransform_NetQuantize NewTargetTransform, FTransform_NetQuantize NewInitialRelCameraTransform, bool bZeroToHead)
{
	SeatInformation.StoredTargetTransform = NewTargetTransform;
	SeatInformation.InitialRelCameraTransform = NewInitialRelCameraTransform;

	// Purify the yaw of the initial rotation
	SeatInformation.InitialRelCameraTransform.SetRotation(UVRExpansionFunctionLibrary::GetHMDPureYaw_I(NewInitialRelCameraTransform.Rotator()).Quaternion());

	// #TODO: Need to handle non 1 scaled values here eventually
	if (bZeroToHead)
	{
		FVector newLocation = SeatInformation.InitialRelCameraTransform.GetTranslation();
		SeatInformation.StoredTargetTransform.AddToTranslation(FVector(0, 0, -newLocation.Z));
	}

#if WITH_PUSH_MODEL
	MARK_PROPERTY_DIRTY_FROM_NAME(AVRBaseCharacter, SeatInformation, this);
#endif

	OnRep_SeatedCharInfo();
}

bool AVRBaseCharacter::Server_ReZeroSeating_Validate(FTransform_NetQuantize NewTargetTransform, FTransform_NetQuantize NewInitialRelCameraTransform, bool bZeroToHead)
{
	return true;
}

void AVRBaseCharacter::Server_SeatedSnapTurn_Implementation(float Yaw)
{
	if(VRMovementReference && SeatInformation.bSitting)
	{
		FVRMoveActionContainer MoveActionTmp;
		MoveActionTmp.MoveAction = EVRMoveAction::VRMOVEACTION_SnapTurn;
		MoveActionTmp.MoveActionRot.Yaw = Yaw;
		MoveActionTmp.VelRetentionSetting = EVRMoveActionVelocityRetention::VRMOVEACTION_Velocity_None;
		VRMovementReference->MoveActionArray.MoveActions.Add(MoveActionTmp);
		VRMovementReference->CheckForMoveAction();
		VRMovementReference->MoveActionArray.Clear();
	}
}

bool AVRBaseCharacter::Server_SeatedSnapTurn_Validate(float Yaw)
{
	return true;
}

void AVRBaseCharacter::OnCustomMoveActionPerformed_Implementation(EVRMoveAction MoveActionType, FVector MoveActionVector, FRotator MoveActionRotator, uint8 MoveActionFlags)
{

}

void AVRBaseCharacter::OnBeginWallPushback_Implementation(FHitResult HitResultOfImpact, bool bHadLocomotionInput, FVector HmdInput)
{

}

void AVRBaseCharacter::OnEndWallPushback_Implementation()
{

}

void AVRBaseCharacter::OnClimbingSteppedUp_Implementation()
{

}

void AVRBaseCharacter::Server_SendTransformCamera_Implementation(FBPVRComponentPosRep NewTransform)
{
	if(VRReplicatedCamera)
		VRReplicatedCamera->Server_SendCameraTransform_Implementation(NewTransform);
}

bool AVRBaseCharacter::Server_SendTransformCamera_Validate(FBPVRComponentPosRep NewTransform)
{
	return true;
	// Optionally check to make sure that player is inside of their bounds and deny it if they aren't?
}

void AVRBaseCharacter::Server_SendTransformLeftController_Implementation(FBPVRComponentPosRep NewTransform)
{
	if (IsValid(LeftMotionController))
		LeftMotionController->Server_SendControllerTransform_Implementation(NewTransform);
}

bool AVRBaseCharacter::Server_SendTransformLeftController_Validate(FBPVRComponentPosRep NewTransform)
{
	return true;
	// Optionally check to make sure that player is inside of their bounds and deny it if they aren't?
}

void AVRBaseCharacter::Server_SendTransformRightController_Implementation(FBPVRComponentPosRep NewTransform)
{
	if(IsValid(RightMotionController))
		RightMotionController->Server_SendControllerTransform_Implementation(NewTransform);
}

bool AVRBaseCharacter::Server_SendTransformRightController_Validate(FBPVRComponentPosRep NewTransform)
{
	return true;
	// Optionally check to make sure that player is inside of their bounds and deny it if they aren't?
}
FVector AVRBaseCharacter::GetTeleportLocation(FVector OriginalLocation)
{	
	return OriginalLocation;
}


void AVRBaseCharacter::NotifyOfTeleport(bool bRegisterAsTeleport)
{
	if (bRegisterAsTeleport)
	{
		if (GetNetMode() < ENetMode::NM_Client)
			bFlagTeleported = true;

		if (VRMovementReference)
		{
			VRMovementReference->bNotifyTeleported = true;
		}
	}

	if (GetNetMode() < ENetMode::NM_Client)
	{
		if (bRegisterAsTeleport)
		{
			bFlagTeleported = true;
		}
		else
		{
			bFlagTeleportedGrips = true;
		}
	}

	if (IsValid(LeftMotionController))
		LeftMotionController->bIsPostTeleport = true;

	if (IsValid(RightMotionController))
		RightMotionController->bIsPostTeleport = true;
}

void AVRBaseCharacter::OnRep_ReplicatedMovement()
{
	FRepMovement& ReppedMovement = GetReplicatedMovement_Mutable();

	ReppedMovement.AngularVelocity = ReplicatedMovementVR.AngularVelocity;
	ReppedMovement.bRepPhysics = ReplicatedMovementVR.bRepPhysics;
	ReppedMovement.bSimulatedPhysicSleep = ReplicatedMovementVR.bSimulatedPhysicSleep;
	ReppedMovement.LinearVelocity = ReplicatedMovementVR.LinearVelocity;
	ReppedMovement.Location = ReplicatedMovementVR.Location;
	ReppedMovement.Rotation = ReplicatedMovementVR.Rotation;

	ReppedMovement.ServerFrame = ReplicatedMovementVR.ServerFrame;
	ReppedMovement.ServerPhysicsHandle = ReplicatedMovementVR.ServerPhysicsHandle;
	ReppedMovement.bRepAcceleration = ReplicatedMovementVR.bRepAcceleration;
	ReppedMovement.Acceleration = ReplicatedMovementVR.Acceleration;

	Super::OnRep_ReplicatedMovement();

	if (!IsLocallyControlled())
	{
		if (ReplicatedMovementVR.bJustTeleported)
		{
			// Server should never get this value so it shouldn't be double throwing for them
			NotifyOfTeleport();
		}
		else if (ReplicatedMovementVR.bJustTeleportedGrips)
		{
			NotifyOfTeleport(false);
		}

		bTrackingPaused = ReplicatedMovementVR.bPausedTracking;
		if (bTrackingPaused)
		{
			PausedTrackingLoc = ReplicatedMovementVR.PausedTrackingLoc;
			PausedTrackingRot = ReplicatedMovementVR.PausedTrackingRot;
		}
	}
}

void AVRBaseCharacter::GatherCurrentMovement()
{
	Super::GatherCurrentMovement();

	FRepMovement ReppedMovement = this->GetReplicatedMovement();

	ReplicatedMovementVR.AngularVelocity = ReppedMovement.AngularVelocity;
	ReplicatedMovementVR.bRepPhysics = ReppedMovement.bRepPhysics;
	ReplicatedMovementVR.bSimulatedPhysicSleep = ReppedMovement.bSimulatedPhysicSleep;
	ReplicatedMovementVR.LinearVelocity = ReppedMovement.LinearVelocity;
	ReplicatedMovementVR.Location = ReppedMovement.Location;
	ReplicatedMovementVR.Rotation = ReppedMovement.Rotation;
	ReplicatedMovementVR.ServerFrame = ReppedMovement.ServerFrame;
	ReplicatedMovementVR.ServerPhysicsHandle = ReppedMovement.ServerPhysicsHandle;
	ReplicatedMovementVR.bRepAcceleration = ReppedMovement.bRepAcceleration;
	ReplicatedMovementVR.Acceleration = ReppedMovement.Acceleration;

	ReplicatedMovementVR.bJustTeleported = bFlagTeleported;
	ReplicatedMovementVR.bJustTeleportedGrips = bFlagTeleportedGrips;

	bFlagTeleported = false;
	bFlagTeleportedGrips = false;
	ReplicatedMovementVR.bPausedTracking = bTrackingPaused;
	ReplicatedMovementVR.PausedTrackingLoc = PausedTrackingLoc;
	ReplicatedMovementVR.PausedTrackingRot = PausedTrackingRot;

#if WITH_PUSH_MODEL
	MARK_PROPERTY_DIRTY_FROM_NAME(AVRBaseCharacter, ReplicatedMovementVR, this);
#endif

}


void AVRBaseCharacter::OnRep_SeatedCharInfo()
{
	// Handle setting up the player here

	if (UPrimitiveComponent * root = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		if (SeatInformation.bSitting /*&& !SeatInformation.bWasSeated*/) // Removing WasSeated check because we may be switching seats
		{
			if (SeatInformation.bWasSeated)
			{
				if (SeatInformation.SeatParent != this->GetRootComponent()->GetAttachParent())
				{
					InitSeatedModeTransition();
				}
				else // Is just a reposition
				{
					//if (this->Role != ROLE_SimulatedProxy)
					ZeroToSeatInformation();
				}

			}
			else
			{
				if (this->GetLocalRole() == ROLE_SimulatedProxy)
				{
					/*if (UVRBaseCharacterMovementComponent * charMovement = Cast<UVRBaseCharacterMovementComponent>(GetMovementComponent()))
					{
						charMovement->SetMovementMode(MOVE_Custom, (uint8)EVRCustomMovementMode::VRMOVE_Seated);
					}*/
				}
				else
				{
					if (VRMovementReference)
					{
						VRMovementReference->SetMovementMode(MOVE_Custom, (uint8)EVRCustomMovementMode::VRMOVE_Seated);
					}
				}
			}
		}
		else if (!SeatInformation.bSitting && SeatInformation.bWasSeated)
		{
			if (this->GetLocalRole() == ROLE_SimulatedProxy)
			{

				/*if (UVRBaseCharacterMovementComponent * charMovement = Cast<UVRBaseCharacterMovementComponent>(GetMovementComponent()))
				{
					charMovement->ApplyReplicatedMovementMode(SeatInformation.PostSeatedMovementMode);
					//charMovement->SetComponentTickEnabled(true);
				}*/

			}
			else
			{
				if (VRMovementReference)
				{
					VRMovementReference->ApplyReplicatedMovementMode(SeatInformation.PostSeatedMovementMode);
				}
			}
		}
	}
}

void AVRBaseCharacter::InitSeatedModeTransition()
{
	if (UPrimitiveComponent * root = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		if (SeatInformation.bSitting /*&& !SeatInformation.bWasSeated*/) // Removing WasSeated check because we may be switching seats
		{

			if (SeatInformation.SeatParent /*&& !root->IsAttachedTo(SeatInformation.SeatParent)*/)
			{
				FAttachmentTransformRules TransformRule = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
				TransformRule.bWeldSimulatedBodies = true;
				AttachToComponent(SeatInformation.SeatParent, TransformRule);
			}

			if (this->GetLocalRole() == ROLE_SimulatedProxy)
			{
				if (VRMovementReference)
				{
					//charMovement->DisableMovement();
					//charMovement->SetComponentTickEnabled(false);
					//charMovement->SetMovementMode(MOVE_Custom, (uint8)EVRCustomMovementMode::VRMOVE_Seated);
				}

				root->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				// Set it before it is set below
				if (!SeatInformation.bWasSeated)
					SeatInformation.bOriginalControlRotation = bUseControllerRotationYaw;

				SeatInformation.bWasSeated = true;
				bUseControllerRotationYaw = false; // This forces rotation in world space, something that we don't want
				ZeroToSeatInformation();
				OnSeatedModeChanged(SeatInformation.bSitting, SeatInformation.bWasSeated);
			}
			else
			{
				if (VRMovementReference)
				{
					//charMovement->DisableMovement();
					//charMovement->SetComponentTickEnabled(false);
					//charMovement->SetMovementMode(MOVE_Custom, (uint8)EVRCustomMovementMode::VRMOVE_Seated);
					//charMovement->bIgnoreClientMovementErrorChecksAndCorrection = true;

					if (this->GetLocalRole() == ROLE_AutonomousProxy)
					{
						FNetworkPredictionData_Client_Character* ClientData = VRMovementReference->GetPredictionData_Client_Character();
						check(ClientData);

						if (ClientData->SavedMoves.Num())
						{
							// Ack our most recent move, we don't want to start sending old moves after un seating.
							ClientData->AckMove(ClientData->SavedMoves.Num() - 1, *VRMovementReference);
						}
					}

				}

				root->SetCollisionEnabled(ECollisionEnabled::NoCollision);

				// Set it before it is set below
				if (!SeatInformation.bWasSeated)
				{
					SeatInformation.bOriginalControlRotation = bUseControllerRotationYaw;
				}

				SeatInformation.bWasSeated = true;
				bUseControllerRotationYaw = false; // This forces rotation in world space, something that we don't want

				ZeroToSeatInformation();
				OnSeatedModeChanged(SeatInformation.bSitting, SeatInformation.bWasSeated);
			}
		}
		else if (!SeatInformation.bSitting && SeatInformation.bWasSeated)
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

			if (this->GetLocalRole() == ROLE_SimulatedProxy)
			{
				root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);


				bUseControllerRotationYaw = SeatInformation.bOriginalControlRotation;

				SetActorLocationAndRotationVR(SeatInformation.StoredTargetTransform.GetTranslation(), SeatInformation.StoredTargetTransform.Rotator(), true, true, true);

				if (IsValid(LeftMotionController))
				{
					LeftMotionController->PostTeleportMoveGrippedObjects();
				}

				if (IsValid(RightMotionController))
				{
					RightMotionController->PostTeleportMoveGrippedObjects();
				}

				/*if (UVRBaseCharacterMovementComponent * charMovement = Cast<UVRBaseCharacterMovementComponent>(GetMovementComponent()))
				{
					charMovement->ApplyReplicatedMovementMode(SeatInformation.PostSeatedMovementMode);
					//charMovement->SetComponentTickEnabled(true);
				}*/

				OnSeatedModeChanged(SeatInformation.bSitting, SeatInformation.bWasSeated);
			}
			else
			{
				if (VRMovementReference)
				{
					//charMovement->ApplyReplicatedMovementMode(SeatInformation.PostSeatedMovementMode);
					//charMovement->bIgnoreClientMovementErrorChecksAndCorrection = false;
					//charMovement->SetComponentTickEnabled(true);

					if (this->GetLocalRole() == ROLE_Authority)
					{				
						if (bUseExperimentalUnseatModeFix)
						{
							VRMovementReference->bJustUnseated = true;
							FNetworkPredictionData_Server_Character * ServerData = VRMovementReference->GetPredictionData_Server_Character();
							check(ServerData);
							ServerData->CurrentClientTimeStamp = 0.0f;
							ServerData->PendingAdjustment = FClientAdjustment();
							//ServerData->CurrentClientTimeStamp = 0.f;
							//ServerData->ServerAccumulatedClientTimeStamp = 0.0f;
							//ServerData->LastUpdateTime = 0.f;
							ServerData->ServerTimeStampLastServerMove = 0.f;
							ServerData->bForceClientUpdate = false;
							ServerData->TimeDiscrepancy = 0.f;
							ServerData->bResolvingTimeDiscrepancy = false;
							ServerData->TimeDiscrepancyResolutionMoveDeltaOverride = 0.f;
							ServerData->TimeDiscrepancyAccumulatedClientDeltasSinceLastServerTick = 0.f;
						}
						//charMovement->ForceReplicationUpdate();
						//FNetworkPredictionData_Server_Character * ServerData = charMovement->GetPredictionData_Server_Character();
						//check(ServerData);

						// Reset client timestamp check so that there isn't a delay on ending seated mode before we accept movement packets
						//ServerData->CurrentClientTimeStamp = 1.f;
					}
				}

				bUseControllerRotationYaw = SeatInformation.bOriginalControlRotation;

				// Re-purposing them for the new location and rotations
				SetActorLocationAndRotationVR(SeatInformation.StoredTargetTransform.GetTranslation(), SeatInformation.StoredTargetTransform.Rotator(), true, true, true);
				LeftMotionController->PostTeleportMoveGrippedObjects();
				RightMotionController->PostTeleportMoveGrippedObjects();

				// Enable collision now
				root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

				OnSeatedModeChanged(SeatInformation.bSitting, SeatInformation.bWasSeated);
				SeatInformation.ClearTempVals();
			}
		}
	}
}

void AVRBaseCharacter::TickSeatInformation(float DeltaTime)
{
	if (!VRReplicatedCamera)
		return;
	
	float LastThresholdScaler = SeatInformation.CurrentThresholdScaler;
	bool bLastOverThreshold = SeatInformation.bIsOverThreshold;

	FVector NewLoc = VRReplicatedCamera->ReplicatedCameraTransform.Position;//GetRelativeLocation();
	FVector OrigLocation = SeatInformation.InitialRelCameraTransform.GetTranslation();

	if (!SeatInformation.bZeroToHead)
	{
		NewLoc.Z = 0.0f;
		OrigLocation.Z = 0.0f;
	}

	if (FMath::IsNearlyZero(SeatInformation.AllowedRadius))
	{
		// Nothing to process here, seated mode isn't sticking to a set radius
		if (SeatInformation.bIsOverThreshold)
		{
			SeatInformation.bIsOverThreshold = false;
			bLastOverThreshold = false;
		}
		return;
	}
	
	float AbsDistance = FMath::Abs(FVector::Dist(OrigLocation, NewLoc));

	//FTransform newTrans = SeatInformation.StoredTargetTransform * SeatInformation.SeatParent->GetComponentTransform();

	// If over the allowed distance
	if (AbsDistance > SeatInformation.AllowedRadius)
	{
		// Force them back into range
		FVector diff = NewLoc - OrigLocation;
		diff.Normalize();

		if (bRetainRoomscale)
		{
			diff = (-diff * (AbsDistance - SeatInformation.AllowedRadius));
			SetSeatRelativeLocationAndRotationVR(diff);
		}
		else
		{
			diff = (diff * (SeatInformation.AllowedRadius));
			diff.Z = 0.0f;
			SetSeatRelativeLocationAndRotationVR(diff);
		}

		SeatInformation.bWasOverLimit = true;
	}
	else if (SeatInformation.bWasOverLimit) // Make sure we are in the zero point otherwise
	{
		if (bRetainRoomscale)
		{
			SetSeatRelativeLocationAndRotationVR(FVector::ZeroVector);
		}
		else
		{
			FVector diff = NewLoc - OrigLocation;
			diff.Z = 0.0f;
			SetSeatRelativeLocationAndRotationVR(diff);
		}

		SeatInformation.bWasOverLimit = false;
	}
	else
	{
		if (!bRetainRoomscale)
		{
			FVector diff = NewLoc - OrigLocation;
			diff.Z = 0.0f;
			SetSeatRelativeLocationAndRotationVR(diff);
		}
	}

	if (AbsDistance > SeatInformation.AllowedRadius - SeatInformation.AllowedRadiusThreshold)
		SeatInformation.bIsOverThreshold = true;
	else
		SeatInformation.bIsOverThreshold = false;

	SeatInformation.CurrentThresholdScaler = FMath::Clamp((AbsDistance - (SeatInformation.AllowedRadius - SeatInformation.AllowedRadiusThreshold)) / SeatInformation.AllowedRadiusThreshold, 0.0f, 1.0f);

	if (bLastOverThreshold != SeatInformation.bIsOverThreshold || !FMath::IsNearlyEqual(LastThresholdScaler, SeatInformation.CurrentThresholdScaler))
	{
		OnSeatThreshholdChanged(!SeatInformation.bIsOverThreshold, SeatInformation.CurrentThresholdScaler);
		OnSeatThreshholdChanged_Bind.Broadcast(!SeatInformation.bIsOverThreshold, SeatInformation.CurrentThresholdScaler);
	}
}

bool AVRBaseCharacter::SetSeatedMode(USceneComponent * SeatParent, bool bSetSeatedMode, FTransform TargetTransform, FTransform InitialRelCameraTransform, float AllowedRadius, float AllowedRadiusThreshold, bool bZeroToHead, EVRConjoinedMovementModes PostSeatedMovementMode)
{
	if (!this->HasAuthority())
		return false;

	if (bSetSeatedMode)
	{
		if (!SeatParent)
			return false;

		// Automate the intial relative camera transform for this mode
		// I think we can remove the initial value alltogether eventually right?
		if (!bRetainRoomscale && VRReplicatedCamera)
		{
			InitialRelCameraTransform = VRReplicatedCamera->GetHMDTrackingTransform();
		}

		SeatInformation.SeatParent = SeatParent;
		SeatInformation.bSitting = true;
		SeatInformation.bZeroToHead = bZeroToHead;
		SeatInformation.StoredTargetTransform = TargetTransform;
		SeatInformation.InitialRelCameraTransform = InitialRelCameraTransform;

		// Purify the yaw of the initial rotation
		SeatInformation.InitialRelCameraTransform.SetRotation(UVRExpansionFunctionLibrary::GetHMDPureYaw_I(InitialRelCameraTransform.Rotator()).Quaternion());
		SeatInformation.AllowedRadius = AllowedRadius;
		SeatInformation.AllowedRadiusThreshold = AllowedRadiusThreshold;

		// #TODO: Need to handle non 1 scaled values here eventually
		if (bZeroToHead)
		{
			FVector newLocation = SeatInformation.InitialRelCameraTransform.GetTranslation();
			SeatInformation.StoredTargetTransform.AddToTranslation(FVector(0, 0, -newLocation.Z));
		}

#if WITH_PUSH_MODEL
		MARK_PROPERTY_DIRTY_FROM_NAME(AVRBaseCharacter, SeatInformation, this);
#endif

		//SetReplicateMovement(false);/ / No longer doing this, allowing it to replicate down to simulated clients now instead
	}
	else
	{
		SeatInformation.SeatParent = nullptr;
		SeatInformation.StoredTargetTransform = TargetTransform;
		SeatInformation.PostSeatedMovementMode = PostSeatedMovementMode;
		//SetReplicateMovement(true); // No longer doing this, allowing it to replicate down to simulated clients now instead
		SeatInformation.bSitting = false;
	}
#if WITH_PUSH_MODEL
	MARK_PROPERTY_DIRTY_FROM_NAME(AVRBaseCharacter, SeatInformation, this);
#endif

	OnRep_SeatedCharInfo(); // Call this on server side because it won't call itself
	NotifyOfTeleport(); // Teleport the controllers

	return true;
}

void AVRBaseCharacter::SetSeatRelativeLocationAndRotationVR(FVector DeltaLoc)
{
	/*if (bUseYawOnly)
	{
		NewRot.Pitch = 0.0f;
		NewRot.Roll = 0.0f;
	}

	NewLoc = NewLoc + Pivot;
	NewLoc -= NewRot.RotateVector(Pivot);

	SetActorRelativeTransform(FTransform(NewRot, NewLoc, GetCapsuleComponent()->RelativeScale3D));*/

	FVector ZOffset = -GetTargetHeightOffset() * GetCapsuleComponent()->GetRelativeScale3D();

	FTransform NewTrans = SeatInformation.StoredTargetTransform;// *SeatInformation.SeatParent->GetComponentTransform();

	if (!bRetainRoomscale)
	{
		NewTrans.SetTranslation(FVector(0.0f, 0.0f, NewTrans.GetTranslation().Z));
	}


	FVector NewLocation;
	FRotator NewRotation;
	FVector PivotPoint = bRetainRoomscale ? SeatInformation.InitialRelCameraTransform.GetTranslation() : FVector::ZeroVector;
	PivotPoint.Z = 0.0f;

	NewRotation = SeatInformation.InitialRelCameraTransform.Rotator();
	NewRotation = (NewRotation.Quaternion().Inverse() * NewTrans.GetRotation()).Rotator();
	NewLocation = NewTrans.GetTranslation();
	NewLocation -= NewRotation.RotateVector(PivotPoint + (-DeltaLoc));	

	// Also setting actor rot because the control rot transfers to it anyway eventually
	SetActorRelativeTransform(FTransform(NewRotation, NewLocation + ZOffset, GetCapsuleComponent()->GetRelativeScale3D()));
}

FVector AVRBaseCharacter::GetProjectedVRLocation() const
{
	return GetVRLocation_Inline();
}

FVector AVRBaseCharacter::AddActorWorldRotationVR(FRotator DeltaRot, bool bUseYawOnly, bool bRotateAroundCapsule)
{
	AController* OwningController = GetController();

	FVector NewLocation;
	FRotator NewRotation;
	FVector OrigLocation = GetActorLocation();
	FVector PivotPoint = GetActorTransform().InverseTransformPosition(bRotateAroundCapsule ? GetVRLocation_Inline() : GetProjectedVRLocation());
	PivotPoint.Z = 0.0f;

	NewRotation = bUseControllerRotationYaw && OwningController ? OwningController->GetControlRotation() : GetActorRotation();

	if (bUseYawOnly)
	{
		NewRotation.Pitch = 0.0f;
		NewRotation.Roll = 0.0f;
	}

	NewLocation = OrigLocation + NewRotation.RotateVector(PivotPoint);
	NewRotation = (NewRotation.Quaternion() * DeltaRot.Quaternion()).Rotator();
	NewLocation -= NewRotation.RotateVector(PivotPoint);

	if (bUseControllerRotationYaw && OwningController /*&& IsLocallyControlled()*/)
		OwningController->SetControlRotation(NewRotation);

	// Also setting actor rot because the control rot transfers to it anyway eventually
	SetActorLocationAndRotation(NewLocation, NewRotation);
	return NewLocation - OrigLocation;
}

FVector AVRBaseCharacter::SetActorRotationVR(FRotator NewRot, bool bUseYawOnly, bool bAccountForHMDRotation, bool bRotateAroundCapsule)
{
	AController* OwningController = GetController();

	FVector NewLocation;
	FRotator NewRotation;
	FVector OrigLocation = GetActorLocation();
	FVector PivotPoint = GetActorTransform().InverseTransformPosition(bRotateAroundCapsule ? GetVRLocation_Inline() : GetProjectedVRLocation());
	PivotPoint.Z = 0.0f;

	FRotator OrigRotation = bUseControllerRotationYaw && OwningController ? OwningController->GetControlRotation() : GetActorRotation();

	if (bUseYawOnly)
	{
		NewRot.Pitch = 0.0f;
		NewRot.Roll = 0.0f;
	}

	if (bAccountForHMDRotation && VRReplicatedCamera)
	{
		NewRotation = UVRExpansionFunctionLibrary::GetHMDPureYaw_I(VRReplicatedCamera->GetRelativeRotation());
		NewRotation = (NewRot.Quaternion() * NewRotation.Quaternion().Inverse()).Rotator();
	}
	else
		NewRotation = NewRot;

	NewLocation = OrigLocation + OrigRotation.RotateVector(PivotPoint);
	//NewRotation = NewRot;
	NewLocation -= NewRotation.RotateVector(PivotPoint);

	if (bUseControllerRotationYaw && OwningController /*&& IsLocallyControlled()*/)
		OwningController->SetControlRotation(NewRotation);

	// Also setting actor rot because the control rot transfers to it anyway eventually
	SetActorLocationAndRotation(NewLocation, NewRotation);
	return NewLocation - OrigLocation;
}

FVector AVRBaseCharacter::SetActorLocationAndRotationVR(FVector NewLoc, FRotator NewRot, bool bUseYawOnly, bool bAccountForHMDRotation, bool bTeleport, bool bRotateAroundCapsule)
{
	AController* OwningController = GetController();

	FVector NewLocation;
	FRotator NewRotation;
	FVector PivotPoint = GetActorTransform().InverseTransformPosition(bRotateAroundCapsule ? GetVRLocation_Inline() : GetProjectedVRLocation());
	PivotPoint.Z = 0.0f;

	if (bUseYawOnly)
	{
		NewRot.Pitch = 0.0f;
		NewRot.Roll = 0.0f;
	}

	if (bAccountForHMDRotation && VRReplicatedCamera)
	{
		NewRotation = UVRExpansionFunctionLibrary::GetHMDPureYaw_I(VRReplicatedCamera->GetRelativeRotation());//bUseControllerRotationYaw && OwningController ? OwningController->GetControlRotation() : GetActorRotation();
		NewRotation = (NewRot.Quaternion() * NewRotation.Quaternion().Inverse()).Rotator();
	}
	else
		NewRotation = NewRot;

	NewLocation = NewLoc;// +PivotPoint;// NewRotation.RotateVector(PivotPoint);
						 //NewRotation = NewRot;
	NewLocation -= NewRotation.RotateVector(PivotPoint);

	if (bUseControllerRotationYaw && OwningController /*&& IsLocallyControlled()*/)
		OwningController->SetControlRotation(NewRotation);

	// Also setting actor rot because the control rot transfers to it anyway eventually
	SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None);
	return NewLocation - NewLoc;
}

FVector AVRBaseCharacter::SetActorLocationVR(FVector NewLoc, bool bTeleport, bool bSetCapsuleLocation)
{
	FVector NewLocation;
	//FRotator NewRotation;
	FVector PivotOffsetVal = (bSetCapsuleLocation ? GetVRLocation_Inline() : GetProjectedVRLocation()) - GetActorLocation();
	PivotOffsetVal.Z = 0.0f;


	NewLocation = NewLoc - PivotOffsetVal;// +PivotPoint;// NewRotation.RotateVector(PivotPoint);
						 //NewRotation = NewRot;


	// Also setting actor rot because the control rot transfers to it anyway eventually
	SetActorLocation(NewLocation, false, nullptr, bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None);
	return NewLocation - NewLoc;
}

void  AVRBaseCharacter::OnRep_CapsuleHeight()
{
	if (!VRReplicateCapsuleHeight)
		return;

	if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(GetRootComponent()))
	{
		if (ReplicatedCapsuleHeight.CapsuleHeight > 0.0f && !FMath::IsNearlyEqual(ReplicatedCapsuleHeight.CapsuleHeight, Capsule->GetUnscaledCapsuleHalfHeight()))
		{
			SetCharacterHalfHeightVR(ReplicatedCapsuleHeight.CapsuleHeight, false);
		}
	}
}

void AVRBaseCharacter::SetCharacterSizeVR(float NewRadius, float NewHalfHeight, bool bUpdateOverlaps)
{
	if (UCapsuleComponent * Capsule = Cast<UCapsuleComponent>(this->RootComponent))
	{
		if (!FMath::IsNearlyEqual(NewRadius, Capsule->GetUnscaledCapsuleRadius()) || !FMath::IsNearlyEqual(NewHalfHeight, Capsule->GetUnscaledCapsuleHalfHeight()))
			Capsule->SetCapsuleSize(NewRadius, NewHalfHeight, bUpdateOverlaps);

		if (GetNetMode() < ENetMode::NM_Client && VRReplicateCapsuleHeight)
			ReplicatedCapsuleHeight.CapsuleHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}
}

void AVRBaseCharacter::SetCharacterHalfHeightVR(float HalfHeight, bool bUpdateOverlaps)
{
	if (UCapsuleComponent * Capsule = Cast<UCapsuleComponent>(this->RootComponent))
	{
		if (!FMath::IsNearlyEqual(HalfHeight, Capsule->GetUnscaledCapsuleHalfHeight()))
			Capsule->SetCapsuleHalfHeight(HalfHeight, bUpdateOverlaps);

		if (GetNetMode() < ENetMode::NM_Client && VRReplicateCapsuleHeight)
			ReplicatedCapsuleHeight.CapsuleHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}
}

void AVRBaseCharacter::ExtendedSimpleMoveToLocation(const FVector& GoalLocation, float AcceptanceRadius, bool bStopOnOverlap, bool bUsePathfinding, bool bProjectDestinationToNavigation, bool bCanStrafe, TSubclassOf<UNavigationQueryFilter> FilterClass, bool bAllowPartialPaths)
{
	UNavigationSystemV1* NavSys = Controller ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Controller->GetWorld()) : nullptr;
	if (NavSys == nullptr || Controller == nullptr )
	{
		UE_LOG(LogBaseVRCharacter, Warning, TEXT("UVRSimpleCharacter::ExtendedSimpleMoveToLocation called for NavSys:%s Controller:%s (if any of these is None then there's your problem"),
			*GetNameSafe(NavSys), *GetNameSafe(Controller));
		return;
	}

	UPathFollowingComponent* PFollowComp = nullptr;
	//Controller->InitNavigationControl(PFollowComp);
	if (Controller)
	{
		// New for 4.20, spawning the missing path following component here if there isn't already one
		PFollowComp = Controller->FindComponentByClass<UPathFollowingComponent>();
		if (PFollowComp == nullptr)
		{
			PFollowComp = NewObject<UVRPathFollowingComponent>(Controller);
			PFollowComp->RegisterComponentWithWorld(Controller->GetWorld());
			PFollowComp->Initialize();
		}
	}

	if (PFollowComp == nullptr)
	{
		UE_LOG(LogBaseVRCharacter, Warning, TEXT("ExtendedSimpleMoveToLocation - No PathFollowingComponent Found"));
		return;
	}

	if (!PFollowComp->IsPathFollowingAllowed())
	{
		UE_LOG(LogBaseVRCharacter, Warning, TEXT("ExtendedSimpleMoveToLocation - Path Following Movement Is Not Set To Allowed"));
		return;
	}

	EPathFollowingReachMode ReachMode;
	if (bStopOnOverlap)
		ReachMode = EPathFollowingReachMode::OverlapAgent;
	else
		ReachMode = EPathFollowingReachMode::ExactLocation;

	bool bAlreadyAtGoal = false;

	if(UVRPathFollowingComponent * pathcomp = Cast<UVRPathFollowingComponent>(PFollowComp))
		bAlreadyAtGoal = pathcomp->HasReached(GoalLocation, /*EPathFollowingReachMode::OverlapAgent*/ReachMode);
	else
		bAlreadyAtGoal = PFollowComp->HasReached(GoalLocation, /*EPathFollowingReachMode::OverlapAgent*/ReachMode);

	// script source, keep only one move request at time
	if (PFollowComp->GetStatus() != EPathFollowingStatus::Idle)
	{
		if (GetNetMode() == ENetMode::NM_Client)
		{
			// Stop the movement here, not keeping the velocity because it bugs out for clients, might be able to fix.
			PFollowComp->AbortMove(*NavSys, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest
				, FAIRequestID::AnyRequest, /*bAlreadyAtGoal ? */EPathFollowingVelocityMode::Reset /*: EPathFollowingVelocityMode::Keep*/);
		}
		else
		{
			PFollowComp->AbortMove(*NavSys, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest
				, FAIRequestID::AnyRequest, bAlreadyAtGoal ? EPathFollowingVelocityMode::Reset : EPathFollowingVelocityMode::Keep);
		}
	}

	if (bAlreadyAtGoal)
	{
		PFollowComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
	}
	else
	{
		const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef());
		if (NavData)
		{
			FPathFindingQuery Query(Controller, *NavData, Controller->GetNavAgentLocation(), GoalLocation);
			FPathFindingResult Result = NavSys->FindPathSync(Query);
			if (Result.IsSuccessful())
			{
				FAIMoveRequest MoveReq(GoalLocation);
				MoveReq.SetUsePathfinding(bUsePathfinding);
				MoveReq.SetAllowPartialPath(bAllowPartialPaths);
				MoveReq.SetProjectGoalLocation(bProjectDestinationToNavigation);
				MoveReq.SetNavigationFilter(*FilterClass ? FilterClass : DefaultNavigationFilterClass);
				MoveReq.SetAcceptanceRadius(AcceptanceRadius);
				MoveReq.SetReachTestIncludesAgentRadius(bStopOnOverlap);
				MoveReq.SetCanStrafe(bCanStrafe);
				MoveReq.SetReachTestIncludesGoalRadius(true);

				PFollowComp->RequestMove(/*FAIMoveRequest(GoalLocation)*/MoveReq, Result.Path);
			}
			else if (PFollowComp->GetStatus() != EPathFollowingStatus::Idle)
			{
				PFollowComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
			}
		}
	}
}

bool AVRBaseCharacter::GetCurrentNavigationPathPoints(TArray<FVector>& NavigationPointList)
{
	UPathFollowingComponent* PFollowComp = nullptr;
	if (Controller)
	{
		// New for 4.20, spawning the missing path following component here if there isn't already one
		PFollowComp = Controller->FindComponentByClass<UPathFollowingComponent>();
		if (PFollowComp)
		{
			FNavPathSharedPtr NavPtr = PFollowComp->GetPath();
			if (NavPtr.IsValid())
			{
				TArray<FNavPathPoint>& NavPoints = NavPtr->GetPathPoints();
				if (NavPoints.Num())
				{
					FTransform BaseTransform = FTransform::Identity;
					if (AActor* BaseActor = NavPtr->GetBaseActor())
					{
						BaseTransform = BaseActor->GetActorTransform();
					}				

					NavigationPointList.Empty(NavPoints.Num());
					NavigationPointList.AddUninitialized(NavPoints.Num());

					int counter = 0;
					for (FNavPathPoint& pt : NavPoints)
					{
						NavigationPointList[counter++] = BaseTransform.TransformPosition(pt.Location);
					}

					return true;
				}
			}

			return false;
		}
	}

	return false;
}

void AVRBaseCharacter::NavigationMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	this->Controller->StopMovement();
	ReceiveNavigationMoveCompleted(Result.Code);
}

EPathFollowingStatus::Type AVRBaseCharacter::GetMoveStatus() const
{
	if (!Controller)
		return EPathFollowingStatus::Idle;

	if (UPathFollowingComponent* pathComp = Controller->FindComponentByClass<UPathFollowingComponent>())
	{
		pathComp->GetStatus();
	}

	return EPathFollowingStatus::Idle;
}

bool AVRBaseCharacter::HasPartialPath() const
{
	if (!Controller)
		return false;

	if (UPathFollowingComponent* pathComp = Controller->FindComponentByClass<UPathFollowingComponent>())
	{
		return pathComp->HasPartialPath();
	}

	return false;
}

void AVRBaseCharacter::StopNavigationMovement()
{
	if (!Controller)
		return;

	if (UPathFollowingComponent* pathComp = Controller->FindComponentByClass<UPathFollowingComponent>())
	{
		// @note FPathFollowingResultFlags::ForcedScript added to make AITask_MoveTo instances 
		// not ignore OnRequestFinished notify that's going to be sent out due to this call
		pathComp->AbortMove(*this, FPathFollowingResultFlags::MovementStop | FPathFollowingResultFlags::ForcedScript);
	}
}

void AVRBaseCharacter::SetVRReplicateCapsuleHeight(bool bNewVRReplicateCapsuleHeight)
{
	VRReplicateCapsuleHeight = bNewVRReplicateCapsuleHeight;
#if WITH_PUSH_MODEL
	MARK_PROPERTY_DIRTY_FROM_NAME(AVRBaseCharacter, VRReplicateCapsuleHeight, this);
#endif
}

bool FRepMovementVRCharacter::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar.UsingCustomVersion(FEngineNetworkCustomVersion::Guid);

	FRepMovement BaseSettings = Owner ? Owner->GetReplicatedMovement() : FRepMovement();

	// pack bitfield with flags
	const bool bServerFrameAndHandleSupported = Ar.EngineNetVer() >= FEngineNetworkCustomVersion::RepMoveServerFrameAndHandle && Ar.EngineNetVer() != FEngineNetworkCustomVersion::Ver21AndViewPitchOnly_DONOTUSE;
	uint8 Flags = (bSimulatedPhysicSleep << 0) | (bRepPhysics << 1) | (bJustTeleported << 2) | (bJustTeleportedGrips << 3) | (bPausedTracking << 4);
	Ar.SerializeBits(&Flags, 5);
	bSimulatedPhysicSleep = (Flags & (1 << 0)) ? 1 : 0;
	bRepPhysics = (Flags & (1 << 1)) ? 1 : 0;
	const bool bRepServerFrame = (Flags & (1 << 2) && bServerFrameAndHandleSupported) ? 1 : 0;
	const bool bRepServerHandle = (Flags & (1 << 3) && bServerFrameAndHandleSupported) ? 1 : 0;

	bJustTeleported = (Flags & (1 << 2)) ? 1 : 0;
	bJustTeleportedGrips = (Flags & (1 << 3)) ? 1 : 0;
	bPausedTracking = (Flags & (1 << 4)) ? 1 : 0;

	bOutSuccess = true;

	if (bPausedTracking)
	{
		bOutSuccess &= PausedTrackingLoc.NetSerialize(Ar, Map, bOutSuccess);

		uint16 Yaw = 0;
		if (Ar.IsSaving())
		{
			Yaw = FRotator::CompressAxisToShort(PausedTrackingRot);
			Ar << Yaw;
		}
		else
		{
			Ar << Yaw;
			PausedTrackingRot = Yaw;
		}

	}

	// update location, rotation, linear velocity
	bOutSuccess &= SerializeQuantizedVector(Ar, Location, BaseSettings.LocationQuantizationLevel);

	switch (BaseSettings.RotationQuantizationLevel)
	{
	case ERotatorQuantization::ByteComponents:
	{
		Rotation.SerializeCompressed(Ar);
		break;
	}

	case ERotatorQuantization::ShortComponents:
	{
		Rotation.SerializeCompressedShort(Ar);
		break;
	}
	}

	bOutSuccess &= SerializeQuantizedVector(Ar, LinearVelocity, BaseSettings.VelocityQuantizationLevel);

	// update angular velocity if required
	if (bRepPhysics)
	{
		bOutSuccess &= SerializeQuantizedVector(Ar, AngularVelocity, BaseSettings.VelocityQuantizationLevel);
	}

	if (bRepServerFrame)
	{
		uint32 uServerFrame = (uint32)ServerFrame;
		Ar.SerializeIntPacked(uServerFrame);
		ServerFrame = (int32)uServerFrame;
	}

	if (bRepServerHandle)
	{
		uint32 uServerPhysicsHandle = (uint32)ServerPhysicsHandle;
		Ar.SerializeIntPacked(uServerPhysicsHandle);
		ServerPhysicsHandle = (int32)uServerPhysicsHandle;
	}

	if (Ar.EngineNetVer() >= FEngineNetworkCustomVersion::RepMoveOptionalAcceleration)
	{
		uint8 AccelFlags = (bRepAcceleration << 0);
		Ar.SerializeBits(&AccelFlags, 1);
		bRepAcceleration = (AccelFlags & (1 << 0)) ? 1 : 0;

		if (bRepAcceleration)
		{
			// Note that we're using the same quantization as Velocity, since the units are commonly on the same order
			bOutSuccess &= SerializeQuantizedVector(Ar, Acceleration, VelocityQuantizationLevel);
		}
	}
	else if (Ar.IsLoading())
	{
		bRepAcceleration = false;
	}

	return true;
}


// IRIS NET SERIALIZERS

namespace UE::Net
{

	// -----------------------------------------------------------------------------
	// Iris serializer for FVRReplicatedCapsuleHeight
	// -----------------------------------------------------------------------------
	struct FVRReplicatedCapsuleHeightNetSerializer
	{

		class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
		{
		public:
			virtual ~FNetSerializerRegistryDelegates();

		private:
			virtual void OnPreFreezeNetSerializerRegistry() override;
			//virtual void OnPostFreezeNetSerializerRegistry() override;
		};

		inline static FVRReplicatedCapsuleHeightNetSerializer::FNetSerializerRegistryDelegates NetSerializerRegistryDelegates;


		/** Version is required. */
		static constexpr uint32 Version = 0;

		struct alignas(8) FQuantizedData
		{
			uint32 CompressedFloat;
		};

		typedef FVRReplicatedCapsuleHeight SourceType;
		typedef FQuantizedData QuantizedType;
		typedef FVRReplicatedCapsuleHeightNetSerializerConfig ConfigType;
		inline static const ConfigType DefaultConfig;

		/** Set to false when a same value delta compression method is undesirable, for example when the serializer only writes a single bit for the state. */
		static constexpr bool bUseDefaultDelta = true;
		// Not doing delta, the majority of the time a single bit (bool) controls the serialization of the entirity

		  // Called to create a "quantized snapshot" of the struct
		static void Quantize(FNetSerializationContext& Context, const FNetQuantizeArgs& Args)
		{
			// Actually do the real quantization step here next instead of just in serialize, will save on memory overall
			const SourceType& Source = *reinterpret_cast<const SourceType*>(Args.Source);
			QuantizedType& Target = *reinterpret_cast<QuantizedType*>(Args.Target);

			Target.CompressedFloat = GetCompressedFloat<1024, 18>(Source.CapsuleHeight);
		}

		// Called to apply the quantized snapshot back to gameplay memory
		static void Dequantize(FNetSerializationContext& Context, const FNetDequantizeArgs& Args)
		{
			const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
			SourceType& Target = *reinterpret_cast<SourceType*>(Args.Target);

			Target.CapsuleHeight = GetDecompressedFloat<1024, 18>(Source.CompressedFloat);
		}

		// Serialize into bitstream
		static void Serialize(FNetSerializationContext& Context, const FNetSerializeArgs& Args)
		{
			const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
			FNetBitStreamWriter* Writer = Context.GetBitStreamWriter();

			Writer->WriteBits(static_cast<uint32>(Source.CompressedFloat), 18);

		}

		// Deserialize from bitstream
		static void Deserialize(FNetSerializationContext& Context, const FNetDeserializeArgs& Args)
		{
			QuantizedType& Target = *reinterpret_cast<QuantizedType*>(Args.Target);
			FNetBitStreamReader* Reader = Context.GetBitStreamReader();

			Target.CompressedFloat = Reader->ReadBits(18);
		}

		// Compare two instances to see if they differ
		static bool IsEqual(FNetSerializationContext& Context, const FNetIsEqualArgs& Args)
		{
			if (Args.bStateIsQuantized)
			{
				const QuantizedType& QuantizedValue0 = *reinterpret_cast<const QuantizedType*>(Args.Source0);
				const QuantizedType& QuantizedValue1 = *reinterpret_cast<const QuantizedType*>(Args.Source1);
				return FPlatformMemory::Memcmp(&QuantizedValue0, &QuantizedValue1, sizeof(QuantizedType)) == 0;
			}
			else
			{
				const SourceType& L = *reinterpret_cast<const SourceType*>(Args.Source0);
				const SourceType& R = *reinterpret_cast<const SourceType*>(Args.Source1);

				return FMath::IsNearlyEqual(L.CapsuleHeight, R.CapsuleHeight);
			}
		}
	};


	static const FName PropertyNetSerializerRegistry_NAME_FVRReplicatedCapsuleHeight("VRReplicatedCapsuleHeight");
	UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRReplicatedCapsuleHeight, FVRReplicatedCapsuleHeightNetSerializer);

	FVRReplicatedCapsuleHeightNetSerializer::FNetSerializerRegistryDelegates::~FNetSerializerRegistryDelegates()
	{
		UE_NET_UNREGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRReplicatedCapsuleHeight);
	}

	void FVRReplicatedCapsuleHeightNetSerializer::FNetSerializerRegistryDelegates::OnPreFreezeNetSerializerRegistry()
	{
		UE_NET_REGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRReplicatedCapsuleHeight);
	}

	UE_NET_IMPLEMENT_SERIALIZER(FVRReplicatedCapsuleHeightNetSerializer);


	// -----------------------------------------------------------------------------
// Iris serializer for FVRSeatedCharacterInfo
// -----------------------------------------------------------------------------
	struct FVRSeatedCharacterInfoNetSerializer
	{
		inline static const FVectorNetQuantize100NetSerializerConfig FTransformQuantizeSerializerConfig;
		inline static const FObjectPtrNetSerializerConfig ObjectPtrNetSerializerConfig;

		inline static const FNetSerializerConfig* FTransformQuantizeSerializerConfigPtr = &FTransformQuantizeSerializerConfig;
		inline static const FNetSerializer* FTransformQuantizeNetSerializerPtr;

		inline static const FNetSerializerConfig* FObjectPtrSerializerConfigPtr = &ObjectPtrNetSerializerConfig;
		inline static const FNetSerializer* FObjectPtrNetSerializerPtr;

		class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
		{
		public:
			virtual ~FNetSerializerRegistryDelegates();

			void InitNetSerializer()
			{
				FVRSeatedCharacterInfoNetSerializer::FTransformQuantizeNetSerializerPtr = &UE_NET_GET_SERIALIZER(FTransformNetQuantizeNetSerializer);
				FVRSeatedCharacterInfoNetSerializer::FObjectPtrNetSerializerPtr = &UE_NET_GET_SERIALIZER(FObjectPtrNetSerializer);
			}

		private:
			virtual void OnPreFreezeNetSerializerRegistry() override;
			//virtual void OnPostFreezeNetSerializerRegistry() override;
		};

		inline static FVRSeatedCharacterInfoNetSerializer::FNetSerializerRegistryDelegates NetSerializerRegistryDelegates;


		// Version is required. 
		static constexpr uint32 Version = 0;

		struct alignas(8) FQuantizedData
		{
			uint8 bSitting : 1;
			uint8 bZeroToHead : 1;

			FTransformNetQuantizeQuantizedData StoredTargetTransform;
			FObjectNetSerializerQuantizedReferenceStorage SeatParent;
			uint8 PostSeatedMovementMode;

			// Only if bSitting is true
			FTransformNetQuantizeQuantizedData InitialRelCameraTransform;
			uint32 AllowedRadius; // Flt 256, 16 
			uint32 AllowedRadiusThreshold; // Flt 256, 16

			//uint8 bIsOverThreshold : 1; // Not Replicated
			//uint32 CurrentThresholdScaler; // Not Replicated
		};

		typedef FVRSeatedCharacterInfo SourceType;
		typedef FQuantizedData QuantizedType;
		typedef FVRSeatedCharacterInfoNetSerializerConfig ConfigType;
		inline static const ConfigType DefaultConfig;

		// Set to false when a same value delta compression method is undesirable, for example when the serializer only writes a single bit for the state. 
		static constexpr bool bUseDefaultDelta = true;
		// TODO: This is actually a struct that could use some delta serialization implementations.

		  // Called to create a "quantized snapshot" of the struct
		static void Quantize(FNetSerializationContext& Context, const FNetQuantizeArgs& Args)
		{
			// Actually do the real quantization step here next instead of just in serialize, will save on memory overall
			const SourceType& Source = *reinterpret_cast<const SourceType*>(Args.Source);
			QuantizedType& Target = *reinterpret_cast<QuantizedType*>(Args.Target);

			Target.bSitting = Source.bSitting;
			Target.bZeroToHead = Source.bZeroToHead;
			
			const FNetSerializer* Serializer = FTransformQuantizeNetSerializerPtr;
			const FNetSerializerConfig* SerializerConfig = FTransformQuantizeSerializerConfigPtr;

			//FTransformNetQuantizeQuantizedData StoredTargetTransform;
			FNetQuantizeArgs MemberArgs = Args;
			MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
			MemberArgs.Source = NetSerializerValuePointer(&Source.StoredTargetTransform);
			MemberArgs.Target = NetSerializerValuePointer(&Target.StoredTargetTransform);
			Serializer->Quantize(Context, MemberArgs);


			// Only if bSitting is true
			//FTransformNetQuantizeQuantizedData InitialRelCameraTransform;
			//uint32 AllowedRadius; // Flt 256, 16 
			//uint32 AllowedRadiusThreshold; // Flt 256, 16

			if (Source.bSitting)
			{
				// Initial relative transform doesn't need to be touched or set if not bsitting
				MemberArgs.Source = NetSerializerValuePointer(&Source.InitialRelCameraTransform);
				MemberArgs.Target = NetSerializerValuePointer(&Target.InitialRelCameraTransform);
				Serializer->Quantize(Context, MemberArgs);

				Target.AllowedRadius = GetCompressedFloat<256,16>(Source.AllowedRadius);
				Target.AllowedRadiusThreshold = GetCompressedFloat<256, 16>(Source.AllowedRadiusThreshold);
			}

			const FNetSerializer* ObjSerializer = FObjectPtrNetSerializerPtr;
			const FNetSerializerConfig* ObjSerializerConfig = FObjectPtrSerializerConfigPtr;

			//FObjectNetSerializerQuantizedReferenceStorage SeatParent;
			FNetQuantizeArgs MemberArgsObj = Args;
			MemberArgsObj.NetSerializerConfig = NetSerializerConfigParam(ObjSerializerConfig);
			MemberArgsObj.Source = NetSerializerValuePointer(&Source.SeatParent);
			MemberArgsObj.Target = NetSerializerValuePointer(&Target.SeatParent);
			ObjSerializer->Quantize(Context, MemberArgsObj);

			// Store full 8 bits
			Target.PostSeatedMovementMode = (uint8)Source.PostSeatedMovementMode;
		}

		// Called to apply the quantized snapshot back to gameplay memory
		static void Dequantize(FNetSerializationContext& Context, const FNetDequantizeArgs& Args)
		{
			const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
			SourceType& Target = *reinterpret_cast<SourceType*>(Args.Target);

			Target.bSitting = Source.bSitting != 0;
			Target.bZeroToHead = Source.bZeroToHead != 0;

			const FNetSerializer* Serializer = FTransformQuantizeNetSerializerPtr;
			const FNetSerializerConfig* SerializerConfig = FTransformQuantizeSerializerConfigPtr;

			//FTransformNetQuantizeQuantizedData StoredTargetTransform;
			FNetDequantizeArgs MemberArgs = Args;
			MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
			MemberArgs.Source = NetSerializerValuePointer(&Source.StoredTargetTransform);
			MemberArgs.Target = NetSerializerValuePointer(&Target.StoredTargetTransform);
			Serializer->Dequantize(Context, MemberArgs);


			// Only if bSitting is true
			//FTransformNetQuantizeQuantizedData InitialRelCameraTransform;
			//uint32 AllowedRadius; // Flt 256, 16 
			//uint32 AllowedRadiusThreshold; // Flt 256, 16

			if (Target.bSitting != 0)
			{
				// Initial relative transform doesn't need to be touched or set if not bsitting
				MemberArgs.Source = NetSerializerValuePointer(&Source.InitialRelCameraTransform);
				MemberArgs.Target = NetSerializerValuePointer(&Target.InitialRelCameraTransform);
				Serializer->Dequantize(Context, MemberArgs);

				Target.AllowedRadius = GetDecompressedFloat<256, 16>(Source.AllowedRadius);
				Target.AllowedRadiusThreshold = GetDecompressedFloat<256, 16>(Source.AllowedRadiusThreshold);
			}

			const FNetSerializer* ObjSerializer = FObjectPtrNetSerializerPtr;
			const FNetSerializerConfig* ObjSerializerConfig = FObjectPtrSerializerConfigPtr;

			//FObjectNetSerializerQuantizedReferenceStorage SeatParent;
			FNetDequantizeArgs MemberArgsObj = Args;
			MemberArgsObj.NetSerializerConfig = NetSerializerConfigParam(ObjSerializerConfig);
			MemberArgsObj.Source = NetSerializerValuePointer(&Source.SeatParent);
			MemberArgsObj.Target = NetSerializerValuePointer(&Target.SeatParent);
			ObjSerializer->Dequantize(Context, MemberArgsObj);

			// Store full 8 bits
			Target.PostSeatedMovementMode = (EVRConjoinedMovementModes)Source.PostSeatedMovementMode;
		}

		// Serialize into bitstream
		static void Serialize(FNetSerializationContext& Context, const FNetSerializeArgs& Args)
		{
			const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
			FNetBitStreamWriter* Writer = Context.GetBitStreamWriter();

			Writer->WriteBits(static_cast<uint32>(Source.bSitting), 1);
			Writer->WriteBits(static_cast<uint32>(Source.bZeroToHead), 1);

			const FNetSerializer* Serializer = FTransformQuantizeNetSerializerPtr;
			const FNetSerializerConfig* SerializerConfig = FTransformQuantizeSerializerConfigPtr;

			//FTransformNetQuantizeQuantizedData StoredTargetTransform;
			FNetSerializeArgs MemberArgs = Args;
			MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
			MemberArgs.Source = NetSerializerValuePointer(&Source.StoredTargetTransform);
			Serializer->Serialize(Context, MemberArgs);


			if (Source.bSitting != 0)
			{
				// Initial relative transform doesn't need to be touched or set if not bsitting
				MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
				MemberArgs.Source = NetSerializerValuePointer(&Source.InitialRelCameraTransform);
				Serializer->Serialize(Context, MemberArgs);

				Writer->WriteBits(static_cast<uint32>(Source.AllowedRadius), 16);
				Writer->WriteBits(static_cast<uint32>(Source.AllowedRadiusThreshold), 16);
			}

			const FNetSerializer* ObjSerializer = FObjectPtrNetSerializerPtr;
			const FNetSerializerConfig* ObjSerializerConfig = FObjectPtrSerializerConfigPtr;

			//FObjectNetSerializerQuantizedReferenceStorage SeatParent;
			FNetSerializeArgs MemberArgsObj = Args;
			MemberArgsObj.NetSerializerConfig = NetSerializerConfigParam(ObjSerializerConfig);
			MemberArgsObj.Source = NetSerializerValuePointer(&Source.SeatParent);
			ObjSerializer->Serialize(Context, MemberArgsObj);

			Writer->WriteBits(static_cast<uint32>(Source.PostSeatedMovementMode), 8);
		}

		// Deserialize from bitstream
		static void Deserialize(FNetSerializationContext& Context, const FNetDeserializeArgs& Args)
		{
			QuantizedType& Target = *reinterpret_cast<QuantizedType*>(Args.Target);
			FNetBitStreamReader* Reader = Context.GetBitStreamReader();

			Target.bSitting = Reader->ReadBits(1) != 0;
			Target.bZeroToHead = Reader->ReadBits(1) != 0;

			const FNetSerializer* Serializer = FTransformQuantizeNetSerializerPtr;
			const FNetSerializerConfig* SerializerConfig = FTransformQuantizeSerializerConfigPtr;

			//FTransformNetQuantizeQuantizedData StoredTargetTransform;
			FNetDeserializeArgs MemberArgs = Args;
			MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
			MemberArgs.Target = NetSerializerValuePointer(&Target.StoredTargetTransform);
			Serializer->Deserialize(Context, MemberArgs);


			if (Target.bSitting != 0)
			{
				// Initial relative transform doesn't need to be touched or set if not bsitting
				MemberArgs.NetSerializerConfig = NetSerializerConfigParam(SerializerConfig);
				MemberArgs.Target = NetSerializerValuePointer(&Target.InitialRelCameraTransform);
				Serializer->Deserialize(Context, MemberArgs);

				Target.AllowedRadius = Reader->ReadBits(16);
				Target.AllowedRadiusThreshold = Reader->ReadBits(16);
			}

			const FNetSerializer* ObjSerializer = FObjectPtrNetSerializerPtr;
			const FNetSerializerConfig* ObjSerializerConfig = FObjectPtrSerializerConfigPtr;

			//FObjectNetSerializerQuantizedReferenceStorage SeatParent;
			FNetDeserializeArgs MemberArgsObj = Args;
			MemberArgsObj.NetSerializerConfig = NetSerializerConfigParam(ObjSerializerConfig);
			MemberArgsObj.Target = NetSerializerValuePointer(&Target.SeatParent);
			ObjSerializer->Deserialize(Context, MemberArgsObj);

			Target.PostSeatedMovementMode = Reader->ReadBits(8);
		}

		// Compare two instances to see if they differ
		static bool IsEqual(FNetSerializationContext& Context, const FNetIsEqualArgs& Args)
		{
			if (Args.bStateIsQuantized)
			{
				const QuantizedType& QuantizedValue0 = *reinterpret_cast<const QuantizedType*>(Args.Source0);
				const QuantizedType& QuantizedValue1 = *reinterpret_cast<const QuantizedType*>(Args.Source1);
				return FPlatformMemory::Memcmp(&QuantizedValue0, &QuantizedValue1, sizeof(QuantizedType)) == 0;
			}
			else
			{
				const SourceType& L = *reinterpret_cast<const SourceType*>(Args.Source0);
				const SourceType& R = *reinterpret_cast<const SourceType*>(Args.Source1);

				if (L.bSitting != R.bSitting) return false;
				if (L.SeatParent != R.SeatParent) return false;
				if(!FMath::IsNearlyEqual(L.AllowedRadius, R.AllowedRadius)) return false;
				if (!FMath::IsNearlyEqual(L.AllowedRadiusThreshold, R.AllowedRadiusThreshold)) return false;
				if (L.bZeroToHead != R.bZeroToHead) return false;
				if (!L.StoredTargetTransform.Equals(R.StoredTargetTransform)) return false;

				if (L.bSitting && !L.InitialRelCameraTransform.Equals(R.InitialRelCameraTransform)) return false;

				return true;
			}
		}

		static void Apply(FNetSerializationContext&, const FNetApplyArgs& Args)
		{
			const SourceType& Source = *reinterpret_cast<const SourceType*>(Args.Source);
			SourceType& Target = *reinterpret_cast<SourceType*>(Args.Target);

			Target.bSitting = Source.bSitting;
			Target.bZeroToHead = Source.bZeroToHead;
			Target.StoredTargetTransform = Source.StoredTargetTransform;

			if (Target.bSitting)
			{
				Target.InitialRelCameraTransform = Source.InitialRelCameraTransform;
				Target.AllowedRadius = Source.AllowedRadius;
				Target.AllowedRadiusThreshold = Source.AllowedRadiusThreshold;
			}
			else
			{
				// Clear non repped values
				Target.InitialRelCameraTransform = FTransform::Identity;
				Target.AllowedRadius = 0.0f;
				Target.AllowedRadiusThreshold = 0.0f;
			}

			Target.SeatParent = Source.SeatParent;
			Target.PostSeatedMovementMode = Source.PostSeatedMovementMode;
		}
	};


	static const FName PropertyNetSerializerRegistry_NAME_FVRSeatedCharacterInfo("VRSeatedCharacterInfo");
	UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRSeatedCharacterInfo, FVRSeatedCharacterInfoNetSerializer);

	FVRSeatedCharacterInfoNetSerializer::FNetSerializerRegistryDelegates::~FNetSerializerRegistryDelegates()
	{
		UE_NET_UNREGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRSeatedCharacterInfo);
	}

	void FVRSeatedCharacterInfoNetSerializer::FNetSerializerRegistryDelegates::OnPreFreezeNetSerializerRegistry()
	{
		InitNetSerializer();
		UE_NET_REGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_FVRSeatedCharacterInfo);
	}

	UE_NET_IMPLEMENT_SERIALIZER(FVRSeatedCharacterInfoNetSerializer);
}
