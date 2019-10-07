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
