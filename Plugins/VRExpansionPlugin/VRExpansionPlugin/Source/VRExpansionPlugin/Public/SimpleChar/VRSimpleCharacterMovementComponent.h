// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "VRBaseCharacterMovementComponent.h"
#include "AI/Navigation/NavigationAvoidanceTypes.h"
#include "AI/RVOAvoidanceInterface.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "NavigationSystem.h"
#include "Animation/AnimationAsset.h"
#include "Engine/EngineBaseTypes.h"
#include "Camera/CameraComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Interfaces/NetworkPredictionInterface.h"
#include "WorldCollision.h"
#include "Runtime/Launch/Resources/Version.h"
#include "VRSimpleCharacterMovementComponent.generated.h"

class FDebugDisplayInfo;
class ACharacter;
class AVRSimpleCharacter;
//class UVRSimpleRootComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSimpleCharacterMovement, Log, All);

/** Shared pointer for easy memory management of FSavedMove_Character, for accumulating and replaying network moves. */
//typedef TSharedPtr<class FSavedMove_Character> FSavedMovePtr;


//=============================================================================
/**
 * VRSimpleCharacterMovementComponent handles movement logic for the associated Character owner.
 * It supports various movement modes including: walking, falling, swimming, flying, custom.
 *
 * Movement is affected primarily by current Velocity and Acceleration. Acceleration is updated each frame
 * based on the input vector accumulated thus far (see UPawnMovementComponent::GetPendingInputVector()).
 *
 * Networking is fully implemented, with server-client correction and prediction included.
 *
 * @see ACharacter, UPawnMovementComponent
 * @see https://docs.unrealengine.com/latest/INT/Gameplay/Framework/Pawn/Character/
 */

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIMoveCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);


UCLASS()
class VREXPANSIONPLUGIN_API UVRSimpleCharacterMovementComponent : public UVRBaseCharacterMovementComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float defaultFriction;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float currentFriction;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float minDecayFriction;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float lowAngleFriction;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float highAngleFriction;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float frictionTransitionThreshold;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float frictionDecayRate;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float frictionReviveRate;

	UPROPERTY(Category = "Character Movement: Skiing", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0"))
		float maxSpeed;

	bool bIsFirstTick;
	FVector curCameraLoc;
	FRotator curCameraRot;

	FVector lastCameraLoc;
	FRotator lastCameraRot;

	UPROPERTY(BlueprintReadOnly, Transient, Category = VRMovement)
		UCapsuleComponent * VRRootCapsule;

	UPROPERTY(BlueprintReadOnly, Transient, Category = VRMovement)
		UCameraComponent * VRCameraComponent;

	// Skips checking for the HMD location on tick, for 2D pawns when a headset is connected
	UPROPERTY(BlueprintReadWrite, Category = VRMovement)
		bool bSkipHMDChecks;

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

	void PhysWalking(float deltaTime, int32 Iterations) override;
	void PhysFlying(float deltaTime, int32 Iterations) override;
	void PhysFalling(float deltaTime, int32 Iterations) override;
	void PhysNavWalking(float deltaTime, int32 Iterations) override;
	/**
	* Default UObject constructor.
	*/
	UVRSimpleCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool VRClimbStepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &InHit, FStepDownResult* OutStepDownResult = nullptr) override;

	///////////////////////////
	// Replication Functions
	///////////////////////////
	//virtual void CallServerMove(const class FSavedMove_Character* NewMove, const class FSavedMove_Character* OldMove) override;

	virtual void ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData) override;

	/** Default client to server move RPC data container. Can be bypassed via SetNetworkMoveDataContainer(). */
	//FCharacterNetworkMoveDataContainer VRNetworkMoveDataContainer;
	//FCharacterMoveResponseDataContainer VRMoveResponseDataContainer;
	
	// Use ServerMoveVR instead
	virtual void ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration) override;

	FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	FNetworkPredictionData_Server* GetPredictionData_Server() const override;

	///////////////////////////
	// End Replication Functions
	///////////////////////////


	void ApplyFriction(float DeltaTime, float CharacterFriction, float SurfaceFriction, float StopSpeed)
	{
		//FString String = "Acceleration: " + FString::SanitizeFloat(Acceleration.Size());
		//GEngine->AddOnScreenDebugMessage(1, 5, FColor::Red, String);

		if (Velocity.Size() > maxSpeed || Acceleration.Size() == 0.0f)
		{
			const auto Speed = Velocity.Size();

			// Check if the speed is too small to care about...
			if (Speed < 10.0f)
			{
				Velocity = FVector::ZeroVector;
				return;
			}

			auto ControlSpeed = FMath::Max(StopSpeed, Speed);
			auto SpeedDrop = ControlSpeed * CharacterFriction * SurfaceFriction * DeltaTime;

			Velocity *= FMath::Max(0.0f, Speed - SpeedDrop) / Speed;
		}
	}

	float AngleOfTerrain()
	{
		const FVector PawnLocation = UpdatedComponent->GetComponentLocation();
		FFindFloorResult FloorResult;

		FindFloor(PawnLocation, FloorResult, false);

		FVector zDirection = FVector(0.0f, 0.0f, 1.0f);

		return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FloorResult.HitResult.ImpactNormal, zDirection)));
	}

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override
	{
		return Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		// Do not update velocity when using root motion or when SimulatedProxy and not simulating root motion - SimulatedProxy are repped their Velocity
		if (!HasValidData() || HasAnimRootMotion() || DeltaTime < MIN_TICK_TIME || (CharacterOwner && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy && !bWasSimulatingRootMotion))
		{
			return;
		}

		//Velocity += Acceleration * DeltaTime;

		/*
			NOTES:

			+ The "Acceleration" variable actually gives us our requested acceleration due to controller input:
				+ This maxes out at our "MaxAcceleration" which is 2048 by default.

		*/

		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, *Acceleration.ToString());
		//GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, FString::SanitizeFloat(Friction));

		// If the angle of terrain is high or there is no terrain under us
		(AngleOfTerrain() > frictionTransitionThreshold || !IsMovingOnGround()) ? defaultFriction = highAngleFriction : defaultFriction = lowAngleFriction;

		if (!IsMovingOnGround())
		{
			// Decay friction
			currentFriction *= frictionDecayRate;
			currentFriction = FMath::Max(currentFriction, minDecayFriction);
		}
		else
		{
			// Revive friction
			currentFriction *= frictionReviveRate;
			currentFriction = FMath::Min(currentFriction, defaultFriction);
		}

		// Apply friction
		ApplyFriction(DeltaTime, currentFriction, 1.0f, 150.0f);

		// Get our final acceleration
		FVector finalAcceleration = FVector::ZeroVector;

		if (IsMovingOnGround())
		{
			finalAcceleration = FVector(Acceleration.X, Acceleration.Y, GetGravityZ());
		}
		else
		{
			finalAcceleration = FVector(0.0f, 0.0f, GetGravityZ());
		}

		// Calculate our current velocity
		Velocity += (finalAcceleration * DeltaTime);
	}

	virtual bool IsWalkable(const FHitResult& Hit) const override
	{
		if (!Hit.IsValidBlockingHit())
		{
			// No hit, or starting in penetration
			return false;
		}

		// Never walk up vertical surfaces.
		if (Hit.ImpactNormal.Z < KINDA_SMALL_NUMBER)
		{
			return false;
		}

		/*float TestWalkableZ = WalkableFloorZ;

		// See if this component overrides the walkable floor z.
		const UPrimitiveComponent* HitComponent = Hit.Component.Get();
		if (HitComponent)
		{
			const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
			TestWalkableZ = SlopeOverride.ModifyWalkableFloorZ(TestWalkableZ);
		}

		// Can't walk on this surface if it is too steep.
		if (Hit.ImpactNormal.Z < TestWalkableZ)
		{
			return false;
		}*/

		return true;
	}

	virtual void MaintainHorizontalGroundVelocity() override
	{
		/*if (Velocity.Z != 0.f)
		{
			if (bMaintainHorizontalGroundVelocity)
			{
				// Ramp movement already maintained the velocity, so we just want to remove the vertical component.
				Velocity.Z = 0.f;
			}
			else
			{
				// Rescale velocity to be horizontal but maintain magnitude of last update.
				Velocity = Velocity.GetSafeNormal2D() * Velocity.Size();
			}
		}*/
	}
};

class VREXPANSIONPLUGIN_API FSavedMove_VRSimpleCharacter : public FSavedMove_VRBaseCharacter
{

public:

	//FVector VRCapsuleLocation;
	//FVector LFDiff;
	//FVector CustomVRInputVector;
	//FRotator VRCapsuleRotation;
	//FVector RequestedVelocity;

	void Clear();
	virtual void SetInitialPosition(ACharacter* C);
	virtual void PrepMoveFor(ACharacter* Character) override;

	FSavedMove_VRSimpleCharacter() : FSavedMove_VRBaseCharacter()
	{
		//VRCapsuleLocation = FVector::ZeroVector;
		LFDiff = FVector::ZeroVector;
		//CustomVRInputVector = FVector::ZeroVector;
		//VRCapsuleRotation = FRotator::ZeroRotator;
		//RequestedVelocity = FVector::ZeroVector;
	}

};

// Need this for capsule location replication
class VREXPANSIONPLUGIN_API FNetworkPredictionData_Client_VRSimpleCharacter : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_VRSimpleCharacter(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Client_Character(ClientMovement)
	{

	}

	FSavedMovePtr AllocateNewMove()
	{
		return FSavedMovePtr(new FSavedMove_VRSimpleCharacter());
	}
};


// Need this for capsule location replication?????
class VREXPANSIONPLUGIN_API FNetworkPredictionData_Server_VRSimpleCharacter : public FNetworkPredictionData_Server_Character
{
public:
	FNetworkPredictionData_Server_VRSimpleCharacter(const UCharacterMovementComponent& ClientMovement)
		: FNetworkPredictionData_Server_Character(ClientMovement)
	{

	}

	FSavedMovePtr AllocateNewMove()
	{
		return FSavedMovePtr(new FSavedMove_VRSimpleCharacter());
	}
};