// Fill out your copyright notice in the Description page of Project Settings.

#include "GripScripts/GS_Melee.h"
#include "VRGripInterface.h"
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
	if (!GrippingController)
		return false;

	if (bLodged)
	{
		//WorldTransform = root->GetComponentTransform();
		return false;
	}

	//root->OnComponentHit.AddUObject(this, &UGS_Melee::OnComponentHit);

	// Blade edge / angle take into account
	// Should we use the custom values of a physical material instead of the density?

	FVector Difference = (WorldTransform.GetLocation() - root->GetComponentLocation());
	StrikeVelocity = Difference / DeltaTime;

	static float MinimumPenetrationVelocity = 100.0f;
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

					//FVector VelocityOnNormalPlane;
					if (HitResult.PhysMaterial != nullptr) //&& SurfaceTypesToPenetrate.Contains(HitResult.PhysMaterial->SurfaceType.GetValue()))
					{
						float ModifiedPenetrationVelocity = MinimumPenetrationVelocity * (DensityToVelocityScaler * HitResult.PhysMaterial->Density);
						bLodged = true;
						OnMeleeLodgedChanged.Broadcast(bLodged);

						
						// If the mass of the object hit is < threshold of the weapon, then we attach the hit object to the weapon instead of attaching the weapon
						// to the hit object, could also add a constraint between the two...mmm
						// would need a velocity for breaking this hold as well.

					
						// If we penetrated, then project forward by the penetration at the same angle.

						FVector HitNormal = HitResult.ImpactNormal;
						FTransform HitTransform = root->GetComponentTransform();
						HitTransform.SetRotation(WorldTransform.GetRotation());
						HitTransform.Blend(HitTransform, WorldTransform, HitResult.Time);

						// Blend in penetration depth by hit normal
						HitTransform.AddToTranslation((-HitNormal) * 20.0f);

						// Should also handle backing out if it goes outside of the body
						root->SetWorldTransform(HitTransform);
						root->AttachToComponent(HitResult.Component.Get(), FAttachmentTransformRules::KeepWorldTransform, HitResult.BoneName);

						// If other is simulating, then attach to the sword?
						// Otherwise just penetrate?

						// We can penetrate it
						// Sample density
						return false;
					}

					//FHitResult.Normal
					//FHitResult.PenetrationDepth
				}
			}
		}
	}

	return true;
}
