// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Grippables/HandSocketComponent.h"
#include "Net/UnrealNetwork.h"

  //=============================================================================
UHandSocketComponent::UHandSocketComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicateMovement = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	//this->bReplicates = true;

	bRepGameplayTags = true;

#if WITH_EDITORONLY_DATA
	//if (!IsTemplate())
	{
		if (!IsRunningCommandlet())
		{
			HandVisualizerComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<USkeletalMeshComponent>(this, FName("HandVisualization"));
			HandVisualizerComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HandVisualizerComponent->SetComponentTickEnabled(false);
			HandVisualizerComponent->SetHiddenInGame(true);
		}
	}
#endif


	HandRelativePlacement = FTransform::Identity;
	OverrideDistance = 0.0f;
	SlotPrefix = FName("VRGripP");
	HandTargetAnimation = nullptr;
}

UAnimSequence* UHandSocketComponent::GetTargetAnimation()
{
	return HandTargetAnimation;
}

FTransform UHandSocketComponent::GetHandRelativePlacement()
{
	return HandRelativePlacement;
}

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

void UHandSocketComponent::PostInitProperties()
{
	Super::PostInitProperties();
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
					HandVisualizerComponent->AnimationData.AnimToPlay = HandTargetAnimation;
				}
			}
		}
#endif
	}
}
#endif

void UHandSocketComponent::OnAttachmentChanged()
{
	Super::OnAttachmentChanged();

#if WITH_EDITORONLY_DATA
	//if (!IsTemplate())
	if (HandVisualizerComponent)
	{
		HandVisualizerComponent->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetIncludingScale);

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
			HandVisualizerComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			//HandVisualizerComponent->PlayAnimation(HandTargetAnimation, false);
			HandVisualizerComponent->AnimationData.AnimToPlay = HandTargetAnimation;
		}
	}
#endif
}

void UHandSocketComponent::BeginPlay()
{
	// Call the base class 
	Super::BeginPlay();


}

void UHandSocketComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Call the base class 
	Super::EndPlay(EndPlayReason);

#if WITH_EDITORONLY_DATA
	if (HandVisualizerComponent)
	{
		HandVisualizerComponent->MarkPendingKill();
		HandVisualizerComponent = nullptr;
	}
#endif
}

void UHandSocketComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// Call the super at the end, after we've done what we needed to do
	Super::OnComponentDestroyed(bDestroyingHierarchy);

#if WITH_EDITORONLY_DATA
	if (HandVisualizerComponent)
	{
		HandVisualizerComponent->MarkPendingKill();
		HandVisualizerComponent = nullptr;
	}
#endif

}