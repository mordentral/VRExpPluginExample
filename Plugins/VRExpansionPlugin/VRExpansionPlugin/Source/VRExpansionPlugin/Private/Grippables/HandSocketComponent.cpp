// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Grippables/HandSocketComponent.h"
#include "Engine/CollisionProfile.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogVRHandSocketComponent);

  //=============================================================================
UHandSocketComponent::UHandSocketComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicateMovement = false;
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	//this->bReplicates = true;

	bRepGameplayTags = true;

#if WITH_EDITORONLY_DATA
	bTickedPose = false;
	bShowVisualizationMesh = true;
#endif

	HandRelativePlacement = FTransform::Identity;
	OverrideDistance = 0.0f;
	SlotPrefix = FName("VRGripP");
	bUseCustomPoseDeltas = false;
	HandTargetAnimation = nullptr;
	HandTargetAnimationLeft = nullptr;
	bOnlySnapMesh = false;
	bFlipForLeftHand = false;

	MirrorAxis = EAxis::X;
	FlipAxis = EAxis::Y;
}

UAnimSequence* UHandSocketComponent::GetTargetAnimation(bool bIsRightHand)
{
	return (bIsRightHand || !HandTargetAnimationLeft) ? HandTargetAnimation : HandTargetAnimationLeft;
}

FTransform UHandSocketComponent::GetHandRelativePlacement(bool bIsRightHand)
{
	// Optionally mirror for left hand
	return HandRelativePlacement;
}

FTransform UHandSocketComponent::GetHandSocketTransform(UGripMotionControllerComponent* QueryController)
{
	// Optionally mirror for left hand

	if (bFlipForLeftHand)
	{
		if (!QueryController)
		{
			// No controller input
			UE_LOG(LogVRMotionController, Warning, TEXT("HandSocketComponent::GetHandSocketTransform was missing required motion controller for bFlipForLeftand! Check that you are passing a controller into GetClosestSocketInRange!"));
		}
		else
		{
			EControllerHand HandType;
			QueryController->GetHandType(HandType);
			if (HandType == EControllerHand::Left)
			{
				FTransform ReturnTrans = this->GetRelativeTransform();
				ReturnTrans.Mirror(MirrorAxis, FlipAxis);
				if (USceneComponent* AttParent = this->GetAttachParent())
				{
					ReturnTrans = ReturnTrans * AttParent->GetComponentTransform();
				}
				return ReturnTrans;
			}
		}
	}

	return this->GetComponentTransform();
}

FTransform UHandSocketComponent::GetMeshRelativeTransform(UGripMotionControllerComponent* QueryController)
{
	// Optionally mirror for left hand
	if (bFlipForLeftHand)
	{
		EControllerHand HandType;
		QueryController->GetHandType(HandType);
		if (HandType == EControllerHand::Left)
		{
			FTransform ReturnTrans = (HandRelativePlacement * this->GetRelativeTransform());
			ReturnTrans.Mirror(MirrorAxis, FlipAxis);
			if (USceneComponent* AttParent = this->GetAttachParent())
			{
				ReturnTrans = ReturnTrans * AttParent->GetComponentTransform();
			}
			return ReturnTrans;
		}
	}

	return (HandRelativePlacement * this->GetComponentTransform());
}

FTransform UHandSocketComponent::GetBoneTransformAtTime(UAnimSequence* MyAnimSequence, /*float AnimTime,*/ int BoneIdx, bool bUseRawDataOnly)
{
	float tracklen = MyAnimSequence->GetPlayLength();
	FTransform BoneTransform = FTransform::Identity;
	const TArray<FTrackToSkeletonMap>& TrackToSkeletonMap = bUseRawDataOnly ? MyAnimSequence->GetRawTrackToSkeletonMapTable() : MyAnimSequence->GetCompressedTrackToSkeletonMapTable();

	if ((TrackToSkeletonMap.Num() > 0) && (TrackToSkeletonMap[0].BoneTreeIndex == 0))
	{
		MyAnimSequence->GetBoneTransform(BoneTransform, BoneIdx, /*AnimTime*/ tracklen, bUseRawDataOnly);
		return BoneTransform;
	}
	return FTransform::Identity;
}

void UHandSocketComponent::OnRegister()
{
#if WITH_EDITORONLY_DATA
	AActor* MyOwner = GetOwner();
	if (bShowVisualizationMesh && (MyOwner != nullptr) && !IsRunningCommandlet())
	{
		if (HandVisualizerComponent == nullptr)
		{
			//HandVisualizerComponent = NewObject<USkeletalMeshComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
			HandVisualizerComponent = NewObject<UPoseableMeshComponent>(MyOwner, NAME_None, RF_Transactional | RF_TextExportTransient);
			if (HandVisualizerComponent)
			{
				HandVisualizerComponent->SetupAttachment(this);
				HandVisualizerComponent->SetIsVisualizationComponent(true);
				HandVisualizerComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
				HandVisualizerComponent->CastShadow = false;
				HandVisualizerComponent->CreationMethod = CreationMethod;
				//HandVisualizerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				HandVisualizerComponent->SetComponentTickEnabled(false);
				HandVisualizerComponent->SetHiddenInGame(true);
				HandVisualizerComponent->RegisterComponentWithWorld(GetWorld());

				if (VisualizationMesh)
				{
					HandVisualizerComponent->SetSkeletalMesh(VisualizationMesh);
					if (HandPreviewMaterial)
					{
						HandVisualizerComponent->SetMaterial(0, HandPreviewMaterial);
					}
				}

				HandVisualizerComponent->SetRelativeTransform(HandRelativePlacement);

				if (HandTargetAnimation)
				{
					TArray<FName> BonesNames;
					HandVisualizerComponent->GetBoneNames(BonesNames);
					int32 Bones = HandVisualizerComponent->GetNumBones();

					for (int32 i=0; i< Bones; i++)
					{
						FName ParentBone = HandVisualizerComponent->GetParentBone(BonesNames[i]);
						FTransform ParentTrans = FTransform::Identity;
						if (ParentBone != NAME_None)
						{
							ParentTrans = HandVisualizerComponent->GetBoneTransformByName(ParentBone, EBoneSpaces::ComponentSpace);
						}
						

						FQuat DeltaQuat = FQuat::Identity;
						if (bUseCustomPoseDeltas)
						{
							for (FBPVRHandPoseBonePair BonePairC : CustomPoseDeltas)
							{
								if (BonePairC.BoneName == BonesNames[i])
								{
									DeltaQuat = BonePairC.DeltaPose;
									break;
								}
							}
						}

						FTransform BoneTrans = GetBoneTransformAtTime(HandTargetAnimation, /*FLT_MAX,*/ i, false); // true;

						/*int32 BoneIndex = HandVisualizerComponent->GetBoneIndex(BonesNames[i]);
						if (BoneIndex != INDEX_NONE)
						{
							FMatrix BoneToWorldTM = HandVisualizerComponent->GetBoneMatrix(BoneIndex);
							FMatrix LocalTM = BoneTrans.ToMatrixNoScale();//FRotationTranslationMatrix(InRotation, InPosition);
							BoneTrans = FTransform(LocalTM * BoneToWorldTM);
						}*/



						BoneTrans = BoneTrans * ParentTrans;// *HandVisualizerComponent->GetComponentTransform();
						BoneTrans.NormalizeRotation();
						DeltaQuat.Normalize();
						//DeltaQuat *= HandVisualizerComponent->GetComponentTransform().GetRotation().Inverse();

						BoneTrans.ConcatenateRotation(DeltaQuat);
						BoneTrans.NormalizeRotation();
						HandVisualizerComponent->SetBoneTransformByName(BonesNames[i], BoneTrans, EBoneSpaces::ComponentSpace);
						
					}

					if (HandVisualizerComponent && !bTickedPose)
					{
						// Tick Pose first
						if (HandVisualizerComponent->IsRegistered())
						{
							bTickedPose = true;
							HandVisualizerComponent->TickPose(1.0f, false);
							if (HandVisualizerComponent->MasterPoseComponent.IsValid())
							{
								HandVisualizerComponent->UpdateSlaveComponent();
							}
							else
							{
								HandVisualizerComponent->RefreshBoneTransforms(&HandVisualizerComponent->PrimaryComponentTick);
							}
						}
					}



					/*HandVisualizerComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
					HandVisualizerComponent->PlayAnimation(HandTargetAnimation, false);
					HandVisualizerComponent->AnimationData.AnimToPlay = HandTargetAnimation;

					if (HandVisualizerComponent && !bTickedPose)
					{
						// Tick Pose first
						if (HandVisualizerComponent->ShouldTickPose() && HandVisualizerComponent->IsRegistered())
						{
							bTickedPose = true;
							HandVisualizerComponent->TickPose(1.0f, false);
							if (HandVisualizerComponent->MasterPoseComponent.IsValid())
							{
								HandVisualizerComponent->UpdateSlaveComponent();
							}
							else
							{
								HandVisualizerComponent->RefreshBoneTransforms(&HandVisualizerComponent->PrimaryComponentTick);
							}
						}
					}*/
				}
			}
		}
	}

#endif	// WITH_EDITORONLY_DATA

	Super::OnRegister();
}

#if WITH_EDITORONLY_DATA
void UHandSocketComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	UHandSocketComponent* This = CastChecked<UHandSocketComponent>(InThis);
	Collector.AddReferencedObject(This->HandVisualizerComponent);

	Super::AddReferencedObjects(InThis, Collector);
}

void UHandSocketComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	if (HandVisualizerComponent)
	{
		HandVisualizerComponent->DestroyComponent();
	}
}

#endif

void UHandSocketComponent::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UHandSocketComponent, bRepGameplayTags);
	DOREPLIFETIME(UHandSocketComponent, bReplicateMovement);
	DOREPLIFETIME_CONDITION(UHandSocketComponent, GameplayTags, COND_Custom);
}

void UHandSocketComponent::PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE(UHandSocketComponent, GameplayTags, bRepGameplayTags);

	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeLocation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeRotation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeScale3D, bReplicateMovement);
}

//=============================================================================
UHandSocketComponent::~UHandSocketComponent()
{
}

#if WITH_EDITOR

void UHandSocketComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FProperty* PropertyThatChanged = PropertyChangedEvent.Property;

	if (PropertyThatChanged != nullptr)
	{
#if WITH_EDITORONLY_DATA
		if (PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(UHandSocketComponent, HandTargetAnimation) ||
			PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(UHandSocketComponent, VisualizationMesh)
			)
		{
			if (HandVisualizerComponent)
			{
				HandVisualizerComponent->SetSkeletalMesh(VisualizationMesh);
				if (HandPreviewMaterial)
				{
					HandVisualizerComponent->SetMaterial(0, HandPreviewMaterial);
				}

				// make sure the animation skeleton matches the current skeletalmesh
				if (HandTargetAnimation != nullptr && HandVisualizerComponent->SkeletalMesh && HandTargetAnimation->GetSkeleton() == HandVisualizerComponent->SkeletalMesh->Skeleton)
				{
					TArray<FName> BonesNames;
					HandVisualizerComponent->GetBoneNames(BonesNames);
					int32 Bones = HandVisualizerComponent->GetNumBones();
					for (int32 i = 0; i < Bones; i++)
					{
						FName ParentBone = HandVisualizerComponent->GetParentBone(BonesNames[i]);
						FTransform ParentTrans = FTransform::Identity;
						if (ParentBone != NAME_None)
						{
							ParentTrans = HandVisualizerComponent->GetBoneTransformByName(ParentBone, EBoneSpaces::ComponentSpace);
						}

						FTransform BoneTrans = GetBoneTransformAtTime(HandTargetAnimation, /*FLT_MAX,*/ i, false); // true;
						BoneTrans = BoneTrans * ParentTrans;
						HandVisualizerComponent->SetBoneTransformByName(BonesNames[i], BoneTrans, EBoneSpaces::ComponentSpace);
					}

					//HandVisualizerComponent->AnimationData.AnimToPlay = HandTargetAnimation;
					//HandVisualizerComponent->PlayAnimation(HandTargetAnimation, false);
				}
			}
		}
		/*else if (PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(UHandSocketComponent, HandRelativePlacement))
		{
			if (HandVisualizerComponent)
			{
				HandVisualizerComponent->SetRelativeTransform(HandRelativePlacement);
			}
		}*/
#endif
	}
}
#endif

