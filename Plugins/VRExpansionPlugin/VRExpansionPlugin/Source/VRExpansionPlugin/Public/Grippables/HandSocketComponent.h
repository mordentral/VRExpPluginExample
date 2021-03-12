// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GripMotionControllerComponent.h"
#include "VRBPDatatypes.h"
#include "VRGripInterface.h"
#include "VRExpansionFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "HandSocketComponent.generated.h"

/**
*
*/

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, ChildCanTick), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UHandSocketComponent : public USceneComponent, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:

	UHandSocketComponent(const FObjectInitializer& ObjectInitializer);
	~UHandSocketComponent();

	// Relative placement of the hand to this socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Socket Data")
		FTransform HandRelativePlacement;

	// Target Slot Prefix
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Socket Data")
		FName SlotPrefix;

	// Snap distance to use if you want to override the defaults.
	// Will be ignored if == 0.0f
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Socket Data")
		float OverrideDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Socket Data")
		UAnimSequence* HandTargetAnimation;

	// Returns the target animation of the hand
	UFUNCTION(BlueprintCallable, Category = "Hand Socket Data")
		virtual UAnimSequence* GetTargetAnimation();

	// Returns the target relative transform of the hand
	UFUNCTION(BlueprintCallable, Category = "Hand Socket Data")
		virtual FTransform GetHandRelativePlacement();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void OnAttachmentChanged() override;
	virtual void PostInitProperties() override;
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

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

	// Requires bReplicates to be true for the component
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface|Replication")
		bool bRepGameplayTags;

	// Overrides the default of : true and allows for controlling it like in an actor, should be default of off normally with grippable components
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface|Replication")
		bool bReplicateMovement;

	/** mesh component to indicate hand placement */
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, /*Transient,*/ Category = "Hand Visualization")
		class USkeletalMeshComponent* HandVisualizerComponent;

	UPROPERTY(EditAnywhere, Transient, BlueprintReadOnly, Category = "Hand Visualization")
		class USkeletalMesh* VisualizationMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Visualization")
		UMaterial* HandPreviewMaterial;
#endif
};