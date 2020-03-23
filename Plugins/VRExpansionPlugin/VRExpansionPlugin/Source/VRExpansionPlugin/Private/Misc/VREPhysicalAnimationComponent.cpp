#include "Misc/VREPhysicalAnimationComponent.h"
#include "SceneManagement.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysXPublic.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "Physics/PhysicsInterfaceTypes.h"

UVREPhysicalAnimationComponent::UVREPhysicalAnimationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BaseWeldedBoneDriverName = FName(TEXT("hand_r"));
	bAutoSetPhysicsSleepSensitivity = true;
	SleepThresholdMultiplier = 0.0f;
}


void UVREPhysicalAnimationComponent::SetupWeldedBoneDriver(bool bReInit)
{
	TArray<FWeldedBoneDriverData> OriginalData;
	if (bReInit)
	{
		OriginalData = BoneDriverMap;
	}

	BoneDriverMap.Empty();

	USkeletalMeshComponent* SkeleMesh = GetSkeletalMesh();

	if (!SkeleMesh || !SkeleMesh->Bodies.Num())
		return;

	UPhysicsAsset* PhysAsset = SkeleMesh ? SkeleMesh->GetPhysicsAsset() : nullptr;
	if (PhysAsset && SkeleMesh->SkeletalMesh)
	{

#if WITH_PHYSX

		int32 ParentBodyIdx = PhysAsset->FindBodyIndex(BaseWeldedBoneDriverName);

		if (FBodyInstance * ParentBody = (ParentBodyIdx == INDEX_NONE ? nullptr : SkeleMesh->Bodies[ParentBodyIdx]))
		{
			// Build map of bodies that we want to control.
			FPhysicsActorHandle& ActorHandle = ParentBody->GetPhysicsActorHandle();

			if (FPhysicsInterface::IsValid(ActorHandle) /*&& FPhysicsInterface::IsRigidBody(ActorHandle)*/)
			{
				FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle& Actor)
					{
						//TArray<FPhysicsShapeHandle> Shapes;
						PhysicsInterfaceTypes::FInlineShapeArray Shapes;
						FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

						for (FPhysicsShapeHandle& Shape : Shapes)
						{
							FKShapeElem* ShapeElem = FPhysxUserData::Get<FKShapeElem>(FPhysicsInterface::GetUserData(Shape));

							if (ShapeElem)
							{
								FName TargetBoneName = ShapeElem->GetName();
								int32 BoneIdx = SkeleMesh->GetBoneIndex(TargetBoneName);

								if (BoneIdx != INDEX_NONE)
								{
									FWeldedBoneDriverData DriverData;
									DriverData.BoneName = TargetBoneName;
									DriverData.ShapeHandle = Shape;

									if (bReInit && OriginalData.Num() - 1 >= BoneDriverMap.Num())
									{
										DriverData.RelativeTransform = OriginalData[BoneDriverMap.Num()].RelativeTransform;
									}
									else
									{
										FTransform BoneTransform = SkeleMesh->GetSocketTransform(TargetBoneName, ERelativeTransformSpace::RTS_World);
										// Calc shape global pose
										FTransform RelativeTM = FPhysicsInterface::GetLocalTransform(Shape) * FPhysicsInterface::GetGlobalPose_AssumesLocked(ActorHandle);

										RelativeTM = RelativeTM * BoneTransform.Inverse();
										DriverData.RelativeTransform = RelativeTM;
									}

									BoneDriverMap.Add(DriverData);
								}
							}
						}

						if (bAutoSetPhysicsSleepSensitivity && BoneDriverMap.Num() > 0)
						{
							ParentBody->SleepFamily = ESleepFamily::Custom;
							ParentBody->CustomSleepThresholdMultiplier = SleepThresholdMultiplier;
							float SleepEnergyThresh = FPhysicsInterface::GetSleepEnergyThreshold_AssumesLocked(Actor);
							SleepEnergyThresh *= ParentBody->GetSleepThresholdMultiplier();
							FPhysicsInterface::SetSleepEnergyThreshold_AssumesLocked(Actor, SleepEnergyThresh);
						}
					});

				/*	const FTransform& FBodyInstance::GetRelativeBodyTransform(const FPhysicsShapeHandle & InShape) const
{
	check(IsInGameThread());
	const FBodyInstance* BI = WeldParent ? WeldParent : this;
	const FWeldInfo* Result = BI->ShapeToBodiesMap.IsValid() ? BI->ShapeToBodiesMap->Find(InShape) : nullptr;
	return Result ? Result->RelativeTM : FTransform::Identity;
}
*/
//const FTransform& RelativeTM = GetRelativeBodyTransform(Shape);

		// If shape matches one of our updated bone names
		/*FPhysicsCommand::ExecuteShapeWrite(ParentBody, Shape, [&](FPhysicsShapeHandle& InShape)
		{
			FPhysicsInterface::SetLocalTransform(InShape, LocalTransform);
		//	FPhysicsInterface::SetGeometry(InShape, *UpdatedGeometry);
		});*/
			}
		}
#endif
	}
}

void UVREPhysicalAnimationComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UpdateWeldedBoneDriver(DeltaTime);

	//UpdateTargetActors(ETeleportType::None);
}

void UVREPhysicalAnimationComponent::UpdateWeldedBoneDriver(float DeltaTime)
{

	if (!BoneDriverMap.Num())
		return;

	/*
	#if WITH_CHAOS || WITH_IMMEDIATE_PHYSX
		//ensure(false);
#else
#endif
	*/

	USkeletalMeshComponent* SkeleMesh = GetSkeletalMesh();

	if (!SkeleMesh || !SkeleMesh->IsSimulatingPhysics(BaseWeldedBoneDriverName))
		return;

	UPhysicsAsset* PhysAsset = SkeleMesh ? SkeleMesh->GetPhysicsAsset() : nullptr;
	if(PhysAsset && SkeleMesh->SkeletalMesh)
	{

#if WITH_PHYSX

		int32 ParentBodyIdx = PhysAsset->FindBodyIndex(BaseWeldedBoneDriverName);

		if (FBodyInstance * ParentBody = (ParentBodyIdx == INDEX_NONE ? nullptr : SkeleMesh->Bodies[ParentBodyIdx]))
		{
			FPhysicsActorHandle& ActorHandle = ParentBody->GetPhysicsActorHandle();

			if (FPhysicsInterface::IsValid(ActorHandle) /*&& FPhysicsInterface::IsRigidBody(ActorHandle)*/)
			{

				bool bModifiedBody = false;
				FPhysicsCommand::ExecuteWrite(ActorHandle, [&](FPhysicsActorHandle& Actor)
					{
						//TArray<FPhysicsShapeHandle> Shapes;
						PhysicsInterfaceTypes::FInlineShapeArray Shapes;
						FPhysicsInterface::GetAllShapes_AssumedLocked(Actor, Shapes);

						FTransform GlobalPose = FPhysicsInterface::GetGlobalPose_AssumesLocked(ActorHandle).Inverse();

						for (FPhysicsShapeHandle& Shape : Shapes)
						{
							if (FWeldedBoneDriverData * WeldedData = BoneDriverMap.FindByKey(Shape))
							{
								bModifiedBody = true;
								FTransform GlobalTransform = WeldedData->RelativeTransform * SkeleMesh->GetSocketTransform(WeldedData->BoneName, ERelativeTransformSpace::RTS_World);
								FTransform RelativeTM = GlobalTransform * GlobalPose;

								if (!WeldedData->LastLocal.Equals(RelativeTM))
								{
									FPhysicsInterface::SetLocalTransform(Shape, RelativeTM);
									WeldedData->LastLocal = RelativeTM;
								}
							}
						}
					});
			}
			
		}
#endif
	}
}