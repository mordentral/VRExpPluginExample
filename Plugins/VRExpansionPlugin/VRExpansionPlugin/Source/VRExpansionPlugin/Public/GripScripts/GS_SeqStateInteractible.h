// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "VRGripInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Interactibles/VRInteractibleFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "VRSeqStateComponent.generated.h"

class UGripMotionControllerComponent;

/*UENUM(Blueprintable)
enum class EVRSeqStateType : uint8
{
	Seq_Linear,
	Seq_Rotation,
	Seq_LinearAndRotional,
	Seq_Max
};*/

USTRUCT(BlueprintType, Category = "VRSeqStateComponent")
struct VREXPANSIONPLUGIN_API FBPVRSeqStateEntryBase
{
	GENERATED_BODY()
public:
	
	// If it hit the end and is ready to progress then the HitEnd will be 1 or -1
	virtual void DoAction(int8 HitEnd) {};


};

USTRUCT(BlueprintType, Category = "VRSeqStateComponent")
struct VREXPANSIONPLUGIN_API FBPVRSeqStateEntry_Linear : public FBPVRSeqStateEntryBase
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "Settings")
		FVector MinLinearTranslation;

	UPROPERTY(EditAnywhere, Category = "Settings")
		FVector MaxLinearTranslation;


	// If it hit the end and is ready to progress then the HitEnd will be 1 or -1
	virtual void DoAction(int8 HitEnd) override {};


};


/** Delegate for notification when the slider state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVRSeqStateHitNewStateSignature, float, SliderProgressPoint);

/**
* A slider component, can act like a scroll bar, or gun bolt, or spline following component
*/
UCLASS(NotBlueprintable, ClassGroup = (VRExpansionPlugin), hideCategories = TickSettings)
class VREXPANSIONPLUGIN_API UGS_SeqStateInteractible : public UGS_Default
{
	GENERATED_BODY()

public:
	UGS_SeqStateInteractible(const FObjectInitializer& ObjectInitializer);

	~UGS_SeqStateInteractible();
/*
	// Call to use an object
	UPROPERTY(BlueprintAssignable, Category = "VRSeqStateComponent")
		FVRSeqStateHitNewStateSignature OnSliderHitPoint;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Slider State Changed"))
		void ReceiveSliderHitPoint(float SliderProgressPoint);

	// If true then this slider will only update in its tick event instead of normally using the controllers update event
	// Keep in mind that you then must adjust the tick group in order to make sure it happens after the gripping controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
		bool bUpdateInTick;
	bool bPassThrough;

	float LastSliderProgressState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
	FVector MaxSlideDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
	FVector MinSlideDistance;

	// Gets filled in with the current slider location progress
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VRSeqStateComponent")
	float CurrentSliderProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
	bool bSlideDistanceIsInParentSpace;

	// If true then this slider is locked in place until unlocked again
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
		bool bIsLocked;

	// If true then this slider will auto drop even when locked
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRSeqStateComponent")
		bool bAutoDropWhenLocked;

	// Sets if the slider is locked or not
	UFUNCTION(BlueprintCallable, Category = "GripSettings")
		void SetIsLocked(bool bNewLockedState);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GripSettings")
		float PrimarySlotRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GripSettings")
		float SecondarySlotRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GripSettings")
	int GripPriority;

	// Sets the grip priority
	UFUNCTION(BlueprintCallable, Category = "GripSettings")
		void SetGripPriority(int NewGripPriority);

	// Resetting the initial transform here so that it comes in prior to BeginPlay and save loading.
	virtual void OnRegister() override;

	// Now replicating this so that it works correctly over the network
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_InitialRelativeTransform, Category = "VRSeqStateComponent")
		FTransform_NetQuantize InitialRelativeTransform;

	UFUNCTION()
	virtual void OnRep_InitialRelativeTransform()
	{
		CalculateSliderProgress();
	}

	FVector InitialInteractorLocation;
	FVector InitialGripLoc;
	FVector InitialDropLocation;

	// Checks if we should throw some events
	void CheckSliderProgress();

	// Calculates the current slider progress
	UFUNCTION(BlueprintCallable, Category = "VRSeqStateComponent")
		float CalculateSliderProgress();

	// Forcefully sets the slider progress to the defined value
	UFUNCTION(BlueprintCallable, Category = "VRSeqStateComponent")
		void SetSliderProgress(float NewSliderProgress);

	// Should be called after the slider is moved post begin play
	UFUNCTION(BlueprintCallable, Category = "VRSeqStateComponent")
		void ResetInitialSliderLocation();

	void GetLerpedKey(float &ClosestKey, float DeltaTime);
	float GetCurrentSliderProgress(FVector CurLocation, bool bUseKeyInstead = false, float CurKey = 0.f);

	FVector ClampSlideVector(FVector ValueToClamp);
*/
};