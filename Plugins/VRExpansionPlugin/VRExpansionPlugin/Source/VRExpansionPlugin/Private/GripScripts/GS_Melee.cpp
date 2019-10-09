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
			FBPActorGripInformation GripInfo;
			EBPVRResultSwitch Result;
			Grip.HoldingController->GetGripByID(GripInfo, Grip.GripID, Result);

			if (Result == EBPVRResultSwitch::OnSucceeded)
			{
				float GripDistanceOnPrimaryAxis = 0.f;
				FTransform relTransform(GripInfo.RelativeTransform.ToInverseMatrixWithScale());
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

	if (!GrippingController)
		return false;

	if (bLodged)
	{

		return true;
	}

	//root->OnComponentHit.AddUObject(this, &UGS_Melee::OnComponentHit);

	// Blade edge / angle take into account
	// Should we use the custom values of a physical material instead of the density?

	FVector Difference = (WorldTransform.GetLocation() - root->GetComponentLocation());
	StrikeVelocity = Difference / DeltaTime;

	static float MinimumPenetrationVelocity = 1000.0f;
	static float DensityToVelocityScaler = 0.5f;

	// Only even consider penetration if the velocity is at least this size, later we scale the density of the material to see if we can penetrate a specific other material.
	if (bAllowPenetration && StrikeVelocity.SizeSquared() > FMath::Square(MinimumPenetrationVelocity))
	{

		// World transform is already set for us
		const float MinMovementDistSq = (FMath::Square(4.f*KINDA_SMALL_NUMBER));
		if (Difference.SizeSquared() > MinMovementDistSq)
		{
			TArray<FHitResult> SweepHits;
			FComponentQueryParams Params(NAME_None, GetOwner());
			Params.bReturnPhysicalMaterial = true;
			Params.bTraceComplex = bTraceComplex;
			//Params.bIgnoreTouches = true;
			Params.AddIgnoredActor(actor);
			Params.AddIgnoredActor(GrippingController->GetOwner());
			Params.AddIgnoredActors(root->MoveIgnoreActors);

			// Should we sub step this across rotation?
			GetWorld()->ComponentSweepMulti(SweepHits, root, root->GetComponentLocation(), WorldTransform.GetLocation(), WorldTransform.GetRotation(), Params);


			/*
				// Now sweep collision separately so we can get hits but not have the location altered
					if (UPrimitiveComponent * primComp = Cast<UPrimitiveComponent>(Prim))
					{
					CheckComponentWithSweep(primComp, move, primComp->GetComponentRotation(), false);
			*/


			//if (FHitResult::GetNumBlockingHits(SweepHits) > 0)
			{
				for (FHitResult & HitResult : SweepHits)
				{
					if (!HitResult.IsValidBlockingHit())
						continue;


					// Impart force when being pushed in / pulled out

					//FVector VelocityOnNormalPlane;
					if (HitResult.PhysMaterial != nullptr) //&& SurfaceTypesToPenetrate.Contains(HitResult.PhysMaterial->SurfaceType.GetValue()))
					{
						float ModifiedPenetrationVelocity = MinimumPenetrationVelocity* (DensityToVelocityScaler * HitResult.PhysMaterial->Density);
						bLodged = true;
						OnMeleeLodgedChanged.Broadcast(bLodged);
						LodgeParent = HitResult.Component;
						

						if (HitResult.Component->GetMass() < 1000.0f)
						{
							FTransform HitTransform = HitResult.Component->GetComponentTransform();
							HitTransform.AddToTranslation((HitResult.ImpactNormal) * 20.0f);

							// Should also handle backing out if it goes outside of the body
							HitResult.Component->SetWorldTransform(HitTransform);

							//AttachmentRules.bWeldSimulatedBodies = true;
							//FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules::KeepWor
							//HitResult.Component->AttachToComponent(root, AttachmentRules);
							
							
							/*FVector SpawnLoc = HitResult.ImpactPoint;
							FRotator SpawnRot = FRotator::ZeroRotator;
							APhysicsConstraintActor * aConstraint = Cast<APhysicsConstraintActor>(GetWorld()->SpawnActor(APhysicsConstraintActor::StaticClass(), &SpawnLoc, &SpawnRot));
							aConstraint->GetConstraintComp()->SetConstrainedComponents(root, NAME_None, HitResult.Component.Get(), HitResult.BoneName);
							aConstraint->GetConstraintComp()->SetDisableCollision(true);*/
						}
						else
						{
							// If the mass of the object hit is < threshold of the weapon, then we attach the hit object to the weapon instead of attaching the weapon
							// to the hit object, could also add a constraint between the two...mmm
							// would need a velocity for breaking this hold as well.
							HitResult.Component->AddForceAtLocation((-HitResult.ImpactNormal * 10000.0f) * ModifiedPenetrationVelocity, HitResult.ImpactPoint, HitResult.BoneName);
							//HitResult.Component->AddImpulseAtLocation((-HitResult.ImpactNormal) * ModifiedPenetrationVelocity, HitResult.ImpactPoint, HitResult.BoneName);

							// If we penetrated, then project forward by the penetration at the same angle.

							FVector HitNormal = HitResult.ImpactNormal;
							FTransform HitTransform = root->GetComponentTransform();
							HitTransform.SetRotation(WorldTransform.GetRotation());
							HitTransform.Blend(HitTransform, WorldTransform, HitResult.Time);

							// Blend in penetration depth by hit normal
							HitTransform.AddToTranslation((-HitNormal) * 20.0f);

							// Should also handle backing out if it goes outside of the body
							root->SetWorldTransform(HitTransform);
							FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules::KeepWorldTransform;
							AttachmentRules.bWeldSimulatedBodies = true;
							root->AttachToComponent(HitResult.Component.Get(), AttachmentRules, HitResult.BoneName);

							// If other is simulating, then attach to the sword?
							// Otherwise just penetrate?

							// We can penetrate it
							// Sample density
							return false;
						}
					}

					//FHitResult.Normal
					//FHitResult.PenetrationDepth
				}
			}
		}
	}

	return true;
}
