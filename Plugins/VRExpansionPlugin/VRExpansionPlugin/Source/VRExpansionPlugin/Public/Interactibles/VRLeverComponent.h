// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GripMotionControllerComponent.h"
#include "MotionControllerComponent.h"
#include "VRGripInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "PhysicsEngine/ConstraintInstance.h"

#include "PhysicsPublic.h"

#if WITH_PHYSX
#include "PhysXSupport.h"
#endif // WITH_PHYSX


#include "VRLeverComponent.generated.h"


UENUM(Blueprintable)
enum class EVRInteractibleLeverAxis : uint8
{
	Axis_X,
	Axis_Y,
	Axis_XY
};

UENUM(Blueprintable)
enum class EVRInteractibleLeverEventType : uint8
{
	LeverPositive,
	LeverNegative
};

UENUM(Blueprintable)
enum class EVRInteractibleLeverReturnType : uint8
{
	Stay,
	ReturnToZero,
	LerpToMax,
	LerpToMaxIfOverThreshold
};

/** Delegate for notification when the lever state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FVRLeverStateChangedSignature, bool, LeverStatus, EVRInteractibleLeverEventType, LeverStatusType, float, LeverAngleAtTime);

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRLeverComponent : public UStaticMeshComponent, public IVRGripInterface, public IGameplayTagAssetInterface
{
	GENERATED_UCLASS_BODY()

	~UVRLeverComponent();

	// Call to use an object
	UPROPERTY(BlueprintAssignable, Category = "VRLeverComponent")
		FVRLeverStateChangedSignature OnLeverStateChanged;

	// Primary axis angle only
	UPROPERTY(BlueprintReadOnly, Category = "VRLeverComponent")
		float CurrentLeverAngle;

	// Both axis's
	UPROPERTY(BlueprintReadOnly, Category = "VRLeverComponent")
		FVector2D CurrentLeverAngleXY;

	FVector2D CalcAngle(EVRInteractibleLeverAxis AxisToCalc, FVector CurInteractorLocation)
	{
		FVector2D ReturnAxis = FVector2D::ZeroVector;

		if (AxisToCalc == EVRInteractibleLeverAxis::Axis_X)
		{

			ReturnAxis.X = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.Z)) - InitialGripRot.X;

			// Ignore rotations that would flip the angle of the lever to the other side, with a 90 degree allowance
			if (!FMath::IsNearlyZero(LastDeltaAngle.X) && FMath::Sign(FRotator::NormalizeAxis(RotAtGrab.X + ReturnAxis.X)) != FMath::Sign(LastDeltaAngle.X) && FMath::Abs(LastDeltaAngle.X) > 90.0f)
			{
				ReturnAxis.X = LastDeltaAngle.X;
			}

			ReturnAxis.X = FMath::ClampAngle(FRotator::NormalizeAxis(RotAtGrab.X + ReturnAxis.X), -LeverLimitNegative, LeverLimitPositive);
		}
		else
		{
			ReturnAxis.Y = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Z, CurInteractorLocation.X)) - InitialGripRot.Y;

			// Ignore rotations that would flip the angle of the lever to the other side, with a 90 degree allowance
			if (!FMath::IsNearlyZero(LastDeltaAngle.Y) && FMath::Sign(FRotator::NormalizeAxis(RotAtGrab.Y + ReturnAxis.Y)) != FMath::Sign(LastDeltaAngle.Y) && FMath::Abs(LastDeltaAngle.Y) > 90.0f)
			{
				ReturnAxis.Y = LastDeltaAngle.Y;
			}

			ReturnAxis.Y = FMath::ClampAngle(FRotator::NormalizeAxis(RotAtGrab.Y + ReturnAxis.Y), -LeverLimitNegative, LeverLimitPositive);
		}
		
		return ReturnAxis;
	}

	float CalcAngleNumber(EVRInteractibleLeverAxis AxisToCalc, FTransform & CurrentRelativeTransform)
	{
		FQuat RotTransform = FQuat::Identity;

		if (AxisToCalc == EVRInteractibleLeverAxis::Axis_X)
			RotTransform = FRotator(FRotator(0.0, -90.0, 0.0)).Quaternion(); // Correct for roll and DotProduct

		FQuat newInitRot = (InitialRelativeTransform.GetRotation() * RotTransform);

		FVector v1 = (CurrentRelativeTransform.GetRotation() * RotTransform).Vector();
		FVector v2 = (newInitRot).Vector();
		v1.Normalize();
		v2.Normalize();

		FVector CrossP = FVector::CrossProduct(v1, v2);

		float angle = FMath::RadiansToDegrees(FMath::Atan2(CrossP.Size(), FVector::DotProduct(v1, v2)));
		angle *= FMath::Sign(FVector::DotProduct(CrossP, newInitRot.GetRightVector()));

		return angle;
		//	float angle = FMath::RadiansToDegrees(CurrentRelativeTransform.GetRotation().AngularDistance(InitialRelativeTransform.GetRotation())) * ((GetAxisValue(CurrentRelativeTransform.Rotator()) - GetAxisValue(InitialRelativeTransform.Rotator())) < 0 ? -1.0f : 1.0f);
	}

	float LerpAxis()
	{
		return 0.0f;
	/*	float TargetAngle = 0.0f;
		switch (LeverReturnTypeWhenReleased)
		{
		case EVRInteractibleLeverReturnType::LerpToMax:
		{
			if (CurrentLeverAngle >= 0)
				TargetAngle = FMath::RoundToFloat(LeverLimitPositive);
			else
				TargetAngle = -FMath::RoundToFloat(LeverLimitNegative);
		}break;
		case EVRInteractibleLeverReturnType::LerpToMaxIfOverThreshold:
		{
			if ((!FMath::IsNearlyZero(LeverLimitPositive) && CurrentLeverAngle >= (LeverLimitPositive * LeverTogglePercentage)))
				TargetAngle.X = FMath::RoundToFloat(LeverLimitPositive);
			else if ((!FMath::IsNearlyZero(LeverLimitNegative) && CurrentLeverAngle <= -(LeverLimitNegative * LeverTogglePercentage)))
				TargetAngle.X = -FMath::RoundToFloat(LeverLimitNegative);
			//else - Handled by the default value
			//TargetAngle = 0.0f;
		}break;
		case EVRInteractibleLeverReturnType::ReturnToZero:
		default:
		{}break;
		}

		float LerpedVal = FMath::FixedTurn(Angle.X, TargetAngle.X, LeverReturnSpeed * DeltaTime);
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

	// ------------------------------------------------
	// Gameplay tag interface
	// ------------------------------------------------

	/** Overridden to return requirements tags */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = GameplayTags;
	}

	/** Tags that are set on this object */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	// End Gameplay Tag Interface

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;
	

	// Requires bReplicates to be true for the component
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface")
		bool bRepGameplayTags;
		
	// Overrides the default of : true and allows for controlling it like in an actor, should be default of off normally with grippable components
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface|Replication")
		bool bReplicateMovement;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bIsPhysicsLever;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bUngripAtTargetRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		EVRInteractibleLeverAxis LeverRotationAxis;

	// The percentage of the angle at witch the lever will toggle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.01", ClampMax = "1.0", UIMin = "0.01", UIMax = "1.0"))
		float LeverTogglePercentage;

	// The max angle of the lever in the positive direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.0", ClampMax = "179.8", UIMin = "0.0", UIMax = "180.0"))
		float LeverLimitPositive;

	// The max angle of the lever in the negative direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent", meta = (ClampMin = "0.0", ClampMax = "179.8", UIMin = "0.0", UIMax = "180.0"))
		float LeverLimitNegative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		EVRInteractibleLeverReturnType LeverReturnTypeWhenReleased;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		bool bSendLeverEventsDuringLerp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRLeverComponent")
		float LeverReturnSpeed;

	float lerpCounter;
	FVector2D LastDeltaAngle;
	bool bIsLerping;
	FTransform InitialRelativeTransform;
	FVector InitialInteractorLocation;
	FVector InitialInteractorDropLocation;
	FVector2D InitialGripRot;
	FVector2D RotAtGrab;
	bool bLeverState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		EGripMovementReplicationSettings MovementReplicationSetting;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float BreakDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float Stiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float Damping;

	UPROPERTY(BlueprintReadWrite, Category = "VRGripInterface")
		bool bDenyGripping;

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface")
		bool bIsHeld; // Set on grip notify, not net serializing

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface")
		UGripMotionControllerComponent * HoldingController; // Set on grip notify, not net serializing

	TWeakObjectPtr<USceneComponent> ParentComponent;

	// Should be called after the lever is moved post begin play
	UFUNCTION(BlueprintCallable, Category = "VRLeverComponent")
	void ResetInitialLeverLocation()
	{
		// Get our initial relative transform to our parent (or not if un-parented).
		InitialRelativeTransform = this->GetRelativeTransform();
	}

	virtual void OnUnregister() override;;

#if WITH_PHYSX
	physx::PxD6Joint* HandleData;
	int32 SceneIndex;
#endif

	bool DestroyConstraint()
	{
	#if WITH_PHYSX
		if (HandleData)
		{
			// use correct scene
			PxScene* PScene = GetPhysXSceneFromIndex(SceneIndex);
			if (PScene)
			{
				SCOPED_SCENE_WRITE_LOCK(PScene);
				// Destroy joint.
				HandleData->release();
			}

			HandleData = NULL;
			return true;
		}
		else
		{
			return false;
		}
	#endif // WITH_PHYSX

		return true;
	}

	bool SetupConstraint()
	{
#if WITH_PHYSX
		/*
		if (HandleData)
			return true;

		// Get the PxRigidDynamic that we want to grab.
		FBodyInstance* rBodyInstance = this->GetBodyInstance(NAME_None);
		if (!rBodyInstance)
		{
			return false;
		}


		FTransform A2Transform = FTransform::Identity;//GetComponentTransform().Inverse();
		if (ParentComponent.IsValid())
		{
			UPrimitiveComponent * PrimComp = Cast<UPrimitiveComponent>(ParentComponent.Get());

			if (PrimComp)
				A2Transform = PrimComp->GetComponentTransform();
		}

		float rotationalOffset = (LeverLimitPositive - LeverLimitNegative) / 2;
		FRotator AngularRotationOffset = SetAxisValue(rotationalOffset, FRotator::ZeroRotator);
		FTransform RefFrame2 = FTransform(InitialRelativeTransform.GetRotation() * AngularRotationOffset.Quaternion(), A2Transform.InverseTransformPosition(GetComponentLocation()));
		
		ExecuteOnPxRigidDynamicReadWrite(rBodyInstance, [&](PxRigidDynamic* Actor)
		{
			PxScene* Scene = Actor->getScene();

			// If we don't already have a handle - make one now.
			if (!HandleData)
			{
				PxD6Joint* NewJoint = NULL;
				PxRigidDynamic * ParentBody = NULL;

				if (ParentComponent.IsValid())
				{
					UPrimitiveComponent * PrimComp = Cast<UPrimitiveComponent>(ParentComponent.Get());

					if (PrimComp)
						ParentBody = PrimComp->BodyInstance.GetPxRigidDynamic_AssumesLocked();
				}

				NewJoint = PxD6JointCreate(Scene->getPhysics(), ParentBody, U2PTransform(RefFrame2), Actor, PxTransform(PxIdentity));

				if (!NewJoint)
				{
					HandleData = NULL;
				}
				else
				{
					// No constraint instance
					NewJoint->userData = NULL; // don't need
					HandleData = NewJoint;

					// Remember the scene index that the handle joint/actor are in.
					FPhysScene* RBScene = FPhysxUserData::Get<FPhysScene>(Scene->userData);
					const uint32 SceneType = rBodyInstance->UseAsyncScene(RBScene) ? PST_Async : PST_Sync;
					SceneIndex = RBScene->PhysXSceneIndex[SceneType];

					// Pretty Much Unbreakable
					NewJoint->setBreakForce(PX_MAX_REAL, PX_MAX_REAL);
				//	NewJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
					
				//	NewJoint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, false);

					PxConstraintFlags Flags = NewJoint->getConstraintFlags();

					// False flags
					//Flags |= PxConstraintFlag::ePROJECTION;
					Flags |= PxConstraintFlag::eCOLLISION_ENABLED;
					
					// True flags
					Flags &= ~PxConstraintFlag::ePROJECTION;

					NewJoint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
					NewJoint->setProjectionAngularTolerance(FMath::DegreesToRadians(0.1f));
					NewJoint->setProjectionLinearTolerance(0.1f);
					NewJoint->setConstraintFlags(Flags);
					
					// Setting up the joint
					NewJoint->setMotion(PxD6Axis::eX, PxD6Motion::eLOCKED);
					NewJoint->setMotion(PxD6Axis::eY, PxD6Motion::eLOCKED);
					NewJoint->setMotion(PxD6Axis::eZ, PxD6Motion::eLOCKED);

					NewJoint->setMotion(PxD6Axis::eTWIST, LeverRotationAxis == EVRInteractibleLeverAxis::Axis_X ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
					NewJoint->setMotion(PxD6Axis::eSWING1, LeverRotationAxis == EVRInteractibleLeverAxis::Axis_Y ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
					NewJoint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eLOCKED);

					const float CorrectedLeverLimit = (LeverLimitPositive + LeverLimitNegative) / 2;
					const float LeverLimitRad = CorrectedLeverLimit * (PI / 180.0f);
					//PxReal LimitContactDistance = FMath::DegreesToRadians(FMath::Max(1.f, ProfileInstance.ConeLimit.ContactDistance));

					//The limit values need to be clamped so it will be valid in PhysX
					PxReal ZLimitAngle = FMath::ClampAngle(CorrectedLeverLimit, KINDA_SMALL_NUMBER, 179.9999f) * (PI / 180.0f);
					PxReal YLimitAngle = FMath::ClampAngle(CorrectedLeverLimit, KINDA_SMALL_NUMBER, 179.9999f) * (PI / 180.0f);
					//PxReal LimitContactDistance = FMath::DegreesToRadians(FMath::Max(1.f, ProfileInstance.ConeLimit.ContactDistance * FMath::Min(InSwing1LimitScale, InSwing2LimitScale)));
					
					NewJoint->setSwingLimit(PxJointLimitCone(YLimitAngle, ZLimitAngle));
					NewJoint->setTwistLimit(PxJointAngularLimitPair(-LeverLimitRad, LeverLimitRad));

					return true;
				}
			}

			return false;
		});
		*/
#else
		return false;
#endif // WITH_PHYSX

		return false;
	}


	// Grip interface setup

	// Set up as deny instead of allow so that default allows for gripping
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool DenyGripping();

	// How an interfaced object behaves when teleporting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripInterfaceTeleportBehavior TeleportBehavior();

	// Should this object simulate on drop
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool SimulateOnDrop();

	// Grip type to use when gripping a slot
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripCollisionType SlotGripType();

	// Grip type to use when not gripping a slot
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripCollisionType FreeGripType();

	// Secondary grip type
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		ESecondaryGripType SecondaryGripType();

	// Define which movement repliation setting to use
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripMovementReplicationSettings GripMovementReplicationType();

	// Define the late update setting
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		EGripLateUpdateSettings GripLateUpdateSetting();

	// What grip stiffness to use if using a physics constraint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripStiffness();

	// What grip damping to use if using a physics constraint
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripDamping();

	// Get the advanced physics settings for this grip
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		FBPAdvGripPhysicsSettings AdvancedPhysicsSettings();

	// What distance to break a grip at (only relevent with physics enabled grips
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		float GripBreakDistance();

	// Get closest secondary slot in range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void ClosestSecondarySlotInRange(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController = nullptr, FName OverridePrefix = NAME_None);

	// Get closest primary slot in range
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void ClosestPrimarySlotInRange(FVector WorldLocation, bool & bHadSlotInRange, FTransform & SlotWorldTransform, UGripMotionControllerComponent * CallingController = nullptr, FName OverridePrefix = NAME_None);

	// Check if the object is an interactable
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		bool IsInteractible();

	// Returns if the object is held and if so, which pawn is holding it
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void IsHeld(UGripMotionControllerComponent *& CurHoldingController, bool & bCurIsHeld);

	// Sets is held, used by the plugin
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void SetHeld(UGripMotionControllerComponent * NewHoldingController, bool bNewIsHeld);

	// Get interactable settings
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		FBPInteractionSettings GetInteractionSettings();


	// Events //

	// Event triggered each tick on the interfaced object when gripped, can be used for custom movement or grip based logic
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void TickGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation, float DeltaTime);

	// Event triggered on the interfaced object when gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when grip is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnGripRelease(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when child component is gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnChildGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when child component is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnChildGripRelease(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when secondary gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnSecondaryGrip(USceneComponent * SecondaryGripComponent, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when secondary grip is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripInterface")
		void OnSecondaryGripRelease(USceneComponent * ReleasingSecondaryGripComponent, const FBPActorGripInformation & GripInformation);

	// Interaction Functions

	// Call to use an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnUsed();

	// Call to stop using an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnEndUsed();

	// Call to use an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnSecondaryUsed();

	// Call to stop using an object
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripInterface")
		void OnEndSecondaryUsed();

	protected:

		inline FVector2D GetAxisValue(FRotator CheckRotation)
		{
			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				return FVector2D(CheckRotation.Roll, 0.0f); break;
			case EVRInteractibleLeverAxis::Axis_Y:
				return FVector2D(0.0f,CheckRotation.Pitch); break;
			case EVRInteractibleLeverAxis::Axis_XY:
			{
				return FVector2D(CheckRotation.Roll, CheckRotation.Pitch);
			}break;
			default:return FVector2D::ZeroVector; break;
			}
		}

		inline FRotator SetAxisValue(FVector2D SetValue)
		{
			FRotator vec = FRotator::ZeroRotator;

			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				vec.Roll = SetValue.X; break;
			case EVRInteractibleLeverAxis::Axis_Y:
				vec.Pitch = SetValue.Y; break;
			case EVRInteractibleLeverAxis::Axis_XY:
			{
				vec.Roll = SetValue.X;
				vec.Pitch = SetValue.Y;
			}break;
			default:break;
			}

			return vec;
		}

		inline FRotator SetAxisValue(FVector2D SetValue, FRotator Var)
		{
			FRotator vec = Var;
			switch (LeverRotationAxis)
			{
			case EVRInteractibleLeverAxis::Axis_X:
				vec.Roll = SetValue.X; break;
			case EVRInteractibleLeverAxis::Axis_Y:
				vec.Pitch = SetValue.Y; break;
			case EVRInteractibleLeverAxis::Axis_XY:
			{
				vec.Roll = SetValue.X;
				vec.Pitch = SetValue.Y;
			}break;
			default:break;
			}

			return vec;
		}

};