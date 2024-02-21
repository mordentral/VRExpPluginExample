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
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRSeqStateComponent : public UStaticMeshComponent, public IVRGripInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UVRSeqStateComponent(const FObjectInitializer& ObjectInitializer);

	~UVRSeqStateComponent();

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		EGripMovementReplicationSettings MovementReplicationSetting;

	// Distance before the object will break out of the hand, 0.0f == never will
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface")
		float BreakDistance;

	// Checks and applies auto drop if we need too
	bool CheckAutoDrop(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation);

	// Should we deny gripping on this object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRGripInterface", meta = (ScriptName = "IsDenyGripping"))
		bool bDenyGripping;

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface", meta = (ScriptName = "IsCurrentlyHeld"))
		bool bIsHeld; // Set on grip notify, not net serializing

	UPROPERTY(BlueprintReadOnly, Category = "VRGripInterface")
		FBPGripPair HoldingGrip; // Set on grip notify, not net serializing
	bool bOriginalReplicatesMovement;

	// Called when a object is gripped
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnGripSignature OnGripped;

	// Called when a object is dropped
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnDropSignature OnDropped;

	// Grip interface setup

	// Set up as deny instead of allow so that default allows for gripping
	// The GripInitiator is not guaranteed to be valid, check it for validity
	bool DenyGripping_Implementation(UGripMotionControllerComponent * GripInitiator = nullptr) override;

	// How an interfaced object behaves when teleporting
	EGripInterfaceTeleportBehavior TeleportBehavior_Implementation() override;

	// Should this object simulate on drop
	bool SimulateOnDrop_Implementation() override;

	// Grip type to use
	EGripCollisionType GetPrimaryGripType_Implementation(bool bIsSlot) override;

	// Secondary grip type
	ESecondaryGripType SecondaryGripType_Implementation() override;

	// Define which movement repliation setting to use
	EGripMovementReplicationSettings GripMovementReplicationType_Implementation() override;

	// Define the late update setting
	EGripLateUpdateSettings GripLateUpdateSetting_Implementation() override;

	// What grip stiffness and damping to use if using a physics constraint
	void GetGripStiffnessAndDamping_Implementation(float& GripStiffnessOut, float& GripDampingOut) override;

	// Get the advanced physics settings for this grip
	FBPAdvGripSettings AdvancedGripSettings_Implementation() override;

	// What distance to break a grip at (only relevent with physics enabled grips
	float GripBreakDistance_Implementation() override;

	// Get grip slot in range
	void ClosestGripSlotInRange_Implementation(FVector WorldLocation, bool bSecondarySlot, bool& bHadSlotInRange, FTransform& SlotWorldTransform, FName& SlotName, UGripMotionControllerComponent* CallingController = nullptr, FName OverridePrefix = NAME_None) override;

	// Check if an object allows multiple grips at one time
	bool AllowsMultipleGrips_Implementation() override;

	// Returns if the object is held and if so, which controllers are holding it
	void IsHeld_Implementation(TArray<FBPGripPair>& CurHoldingControllers, bool& bCurIsHeld) override;

	// Interface function used to throw the delegates that is invisible to blueprints so that it can't be overridden
	virtual void Native_NotifyThrowGripDelegates(UGripMotionControllerComponent* Controller, bool bGripped, const FBPActorGripInformation& GripInformation, bool bWasSocketed = false) override;

	// Sets is held, used by the plugin
	void SetHeld_Implementation(UGripMotionControllerComponent* NewHoldingController, uint8 GripID, bool bNewIsHeld) override;

	// Returns if the object wants to be socketed
	bool RequestsSocketing_Implementation(USceneComponent*& ParentToSocketTo, FName& OptionalSocketName, FTransform_NetQuantize& RelativeTransform) override;

	// Get grip scripts
	bool GetGripScripts_Implementation(TArray<UVRGripScriptBase*>& ArrayReference) override;


	// Events //

	// Event triggered each tick on the interfaced object when gripped, can be used for custom movement or grip based logic
	void TickGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation, float DeltaTime) override;

	// Event triggered on the interfaced object when gripped
	void OnGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when grip is released
	void OnGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController, const FBPActorGripInformation& GripInformation, bool bWasSocketed = false) override;

	// Event triggered on the interfaced object when child component is gripped
	void OnChildGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when child component is released
	void OnChildGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController, const FBPActorGripInformation& GripInformation, bool bWasSocketed = false) override;

	// Event triggered on the interfaced object when secondary gripped
	void OnSecondaryGrip_Implementation(UGripMotionControllerComponent* GripOwningController, USceneComponent* SecondaryGripComponent, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when secondary grip is released
	void OnSecondaryGripRelease_Implementation(UGripMotionControllerComponent* GripOwningController, USceneComponent* ReleasingSecondaryGripComponent, const FBPActorGripInformation& GripInformation) override;

	// Interaction Functions

	// Call to use an object
	void OnUsed_Implementation() override;

	// Call to stop using an object
	void OnEndUsed_Implementation() override;

	// Call to use an object
	void OnSecondaryUsed_Implementation() override;

	// Call to stop using an object
	void OnEndSecondaryUsed_Implementation() override;

	// Call to send an action event to the object
	void OnInput_Implementation(FKey Key, EInputEvent KeyEvent) override;

	protected:

};