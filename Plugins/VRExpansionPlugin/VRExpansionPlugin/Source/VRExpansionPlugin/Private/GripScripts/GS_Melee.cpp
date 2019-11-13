// Fill out your copyright notice in the Description page of Project Settings.

#include "GripScripts/GS_Melee.h"
#include "VRGripInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsConstraintActor.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "GripMotionControllerComponent.h"

UGS_Melee::UGS_Melee(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	bIsActive = true;
	WorldTransformOverrideType = EGSTransformOverrideType::ModifiesWorldTransform;


	bAllowPenetration = true;
	bUseDensityForPenetrationCalcs = false;
	bTraceComplex = false;

	bSubstepTrace = false;
	MaxSubsteps = 1.0f;

	BaseDamage = 100.0f;
	VelocityDamageScaler = 1.0f;
	MaximumVelocityForDamage = 1000.0f;

	bLodged = false;
	bInjectPrePhysicsHandle = true;
	bInjectPostPhysicsHandle = true;
	WeaponRootOrientationComponent = NAME_None;
	OrientationComponentRelativeFacing = FTransform::Identity;

	bAutoSetPrimaryAndSecondaryHands = true;
}

void UGS_Melee::SetCOMOffsetInLocalSpace(UGripMotionControllerComponent * GrippingController, FBPActorGripInformation & Grip, FVector Offset, bool bOffsetIsInWorldSpace, bool bLimitToXOnly)
{
	// Alter the com offset and constraint COM to match this
	// Allows world space to set to a components location

	// LimitToXOnly means that we will only be sampling the X location and applying an offset from it
	// This allows you to shift grips up and down on the Orientation Components forward axis

	UPrimitiveComponent * Root;
	if (AActor * ParentActor = Cast<AActor>(Grip.GrippedObject))
	{
		Root = Cast<UPrimitiveComponent>(ParentActor->GetRootComponent());
	}
	else
	{
		Root = Cast<UPrimitiveComponent>(Grip.GrippedObject);
	}

	if (!Root)
		return;

	FBodyInstance* rBodyInstance = Root->GetBodyInstance(Grip.GrippedBoneName);

	if (!rBodyInstance)
		return;

	FBPActorPhysicsHandleInformation * PhysicsHandle = GrippingController->GetPhysicsGrip(Grip);

	if (!PhysicsHandle)
		return;

	FPhysicsCommand::ExecuteWrite(rBodyInstance->ActorHandle, [&](const FPhysicsActorHandle& Actor)
		{
		
			FTransform currentTransform = rBodyInstance->GetUnrealWorldTransform_AssumesLocked();
			if (bOffsetIsInWorldSpace)
			{
				// Make our offset be in relative space now
				Offset = currentTransform.InverseTransformPosition(Offset);
			}

			if (bLimitToXOnly)
			{

				//OrientationComponentRelativeFacing
			}

			//PhysicsHandle->COMPosition.SetLocation(Offset);

			FTransform localCom = FPhysicsInterface::GetComTransformLocal_AssumesLocked(Actor);
			localCom.SetLocation(Offset);// PhysicsHandle->COMPosition.GetTranslation());//Loc);
			FPhysicsInterface::SetComLocalPose_AssumesLocked(Actor, localCom);

			//PhysicsHandle->COMPosition.SetLocation(PhysicsHandle->COMPosition.GetTranslation() * -1.f);
		});

	/*
				// Update the center of mass
			FVector Loc = (FTransform((RootBoneRotation * NewGrip.RelativeTransform).ToInverseMatrixWithScale())).GetLocation();
			Loc *= rBodyInstance->Scale3D;

			FTransform localCom = FPhysicsInterface::GetComTransformLocal_AssumesLocked(Actor);
			localCom.SetLocation(Loc);
			FPhysicsInterface::SetComLocalPose_AssumesLocked(Actor, localCom);

			FVector ComLoc = FPhysicsInterface::GetComTransform_AssumesLocked(Actor).GetLocation();
			trans.SetLocation(ComLoc);
			HandleInfo->COMPosition = FTransform(rBodyInstance->GetUnrealWorldTransform().InverseTransformPosition(ComLoc));
			HandleInfo->bSetCOM = true;
	
	
	*/
}

void UGS_Melee::SetPrimaryAndSecondaryHands(FBPGripPair& PrimaryGrip, FBPGripPair& SecondaryGrip)
{

}

void UGS_Melee::OnGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation)
{
	// Not storing an id, we should only be doing this once
//	GetOwner()->OnActorHit.AddDynamic(this, &UGS_Melee::OnActorHit);

	// This lets us change the grip settings prior to actually starting the grip off

	if (GrippingController->HasGripAuthority(GripInformation))
	{
		TArray<FBPGripPair> HoldingControllers;
		bool bIsHeld;
		IVRGripInterface::Execute_IsHeld(GetParent(), HoldingControllers, bIsHeld);

		for (FBPGripPair& Grip : HoldingControllers)
		{
			FBPActorGripInformation * GripInfo = Grip.HoldingController->GetGripPtrByID(Grip.GripID);
			if (GripInfo)
			{
				float GripDistanceOnPrimaryAxis = 0.f;
				FTransform relTransform(GripInfo->RelativeTransform.ToInverseMatrixWithScale());
				relTransform = relTransform.GetRelativeTransform(OrientationComponentRelativeFacing);

				// This is the Forward vector projected transform
				// The most negative one of these is the rearmost hand
				FVector localLoc = relTransform.GetTranslation();
			}

		}


		/*FBPActorGripInformation* Grip = GrippingController->LocallyGrippedObjects.FindByKey(GripInformation.GripID);

		if (!Grip)
			Grip = GrippingController->GrippedObjects.FindByKey(GripInformation.GripID);

		if (Grip)
		{
			Grip->AdvancedGripSettings.PhysicsSettings.PhysicsGripLocationSettings = EPhysicsGripCOMType::COM_GripAtControllerLoc;
		}*/
	}
}

void UGS_Melee::OnBeginPlay_Implementation(UObject * CallingOwner)
{
	// Grip base has no super of this

	if (WeaponRootOrientationComponent.IsValid())
	{
		if (AActor * Owner = GetOwner())
		{
			for (UActorComponent* Comp : Owner->GetComponents())
			{
				if (Comp->GetFName() == WeaponRootOrientationComponent)
				{	
					if (USceneComponent * SceneComp = Cast<USceneComponent>(Comp))
					{
						OrientationComponentRelativeFacing = SceneComp->GetRelativeTransform();
					}
					break;
				}
			}
		}
	}

}

void UGS_Melee::HandlePrePhysicsHandle(FBPActorPhysicsHandleInformation * HandleInfo, FTransform & KinPose)
{
	// Alter to rotate to x+ if we have an orientation component
	FQuat DeltaQuat = OrientationComponentRelativeFacing.GetRotation();

	// This moves the kinematic actor to face its X+ in the direction designated
	KinPose.SetRotation(KinPose.GetRotation() * DeltaQuat);
	HandleInfo->COMPosition.SetRotation(HandleInfo->COMPosition.GetRotation() * DeltaQuat);
}

void UGS_Melee::HandlePostPhysicsHandle(FBPActorPhysicsHandleInformation * HandleInfo)
{
	HandleInfo->AngConstraint.AngularDriveMode = EAngularDriveMode::TwistAndSwing;
	HandleInfo->AngConstraint.TwistDrive = HandleInfo->AngConstraint.SlerpDrive;
	HandleInfo->AngConstraint.SwingDrive = HandleInfo->AngConstraint.SlerpDrive;

	HandleInfo->AngConstraint.SwingDrive.Damping /= 4.f;
	HandleInfo->AngConstraint.SwingDrive.Stiffness /= 4.f;


	HandleInfo->AngConstraint.TwistDrive.bEnablePositionDrive = true;
	HandleInfo->AngConstraint.TwistDrive.bEnableVelocityDrive = true;
	HandleInfo->AngConstraint.SwingDrive.bEnablePositionDrive = true;
	HandleInfo->AngConstraint.SwingDrive.bEnableVelocityDrive = true;

	HandleInfo->AngConstraint.SlerpDrive.bEnablePositionDrive = false;
	HandleInfo->AngConstraint.SlerpDrive.bEnableVelocityDrive = false;
}

bool UGS_Melee::GetWorldTransform_Implementation
(
	UGripMotionControllerComponent* GrippingController, 
	float DeltaTime, FTransform & WorldTransform, 
	const FTransform &ParentTransform, 
	FBPActorGripInformation &Grip, 
	AActor * actor, 
	UPrimitiveComponent * root, 
	bool bRootHasInterface, 
	bool bActorHasInterface, 
	bool bIsForTeleport
) 
{

	return true;
}
