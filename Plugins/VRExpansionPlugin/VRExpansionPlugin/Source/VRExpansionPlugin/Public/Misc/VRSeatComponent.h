// This class is intended to automate offsetting a character in a seated position easily and without 
// Massive effort on the users part.

#pragma once
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "CoreMinimal.h"
#include "IHeadMountedDisplay.h"
#include "VRBaseCharacter.h"

#include "VRSeatComponent.generated.h"

USTRUCT()
struct VREXPANSIONPLUGIN_API FVRSeatedCharacterInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
		AVRBaseCharacter * SeatedCharacter;
	UPROPERTY()
		FVector_NetQuantize100 OriginalRelativeLocation;
	UPROPERTY()
		float OriginalRotationYaw;

	void Clear()
	{
		SeatedCharacter = nullptr;
		OriginalRelativeLocation = FVector::ZeroVector;
		OriginalRotationYaw = 0;
	}


	/** Network serialization */
	// Doing a custom NetSerialize here because this is sent via RPCs and should change on every update
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		Ar << SeatedCharacter;
		OriginalRelativeLocation.NetSerialize(Ar, Map, bOutSuccess);

		uint16 val;
		if (Ar.IsSaving())
		{
			val = FRotator::CompressAxisToShort(OriginalRotationYaw);
			Ar << FRotator::CompressAxisToShort(val);
		}
		else
		{
			Ar << val;
			OriginalRotationYaw = FRotator::DeCompressAxisFromShort(val);
		}

		return bOutSuccess;
	}
};
template<>
struct TStructOpsTypeTraits< FVRSeatedCharacterInfo > : public TStructOpsTypeTraitsBase2<FVRSeatedCharacterInfo>
{
	enum
	{
		WithNetSerializer = true
	};
};



UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = VRExpansionLibrary)
class VREXPANSIONPLUGIN_API UVRSeatComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:

	~UVRSeatComponent();

	// How far we allow the head to move from this seats central position
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRSeatComponent")
		float HeadRadiusAllowance;

	// How far from the edge of the head radius allowance that we start fading the player camera to black
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRSeatComponent")
		float HeadRadiusThreshold;

	// Allow for initial position recording
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, Category = "VRSeatComponent", ReplicatedUsing = OnRep_SeatedCharInfo)
		FVRSeatedCharacterInfo SeatedCharacter;

	bool UnSeatPlayer(AVRBaseCharacter * CharacterToUnSeat, FVector NewWorldLocation)
	{

	}

	bool SeatPlayer(AVRBaseCharacter * CharacterToSeat)
	{
		if(SeatedCharacter.SeatedCharacter != nullptr)
		{ 
			if (SeatedCharacter.SeatedCharacter == CharacterToSeat)
			{
				// Reinit here?
			}
			else
			{
				return false; // Already have a seated player
			}
		}

		CharacterToSeat->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	FVector InitialRelativePosition;
	FRotator InitialRelativeRotation;

	// Should be called if the seat is moved post begin play
	UFUNCTION(BlueprintCallable, Category = "VRSeatComponent")
	void ResetInitialRelativePosition()
	{
		InitialRelativePosition = this->RelativeLocation;
		InitialRelativeRotation = this->RelativeRotation;
	}

	virtual void OnChildAttached(USceneComponent * ChildComponent) override
	{
		Super::OnChildAttached(ChildComponent);

		if (AVRBaseCharacter * BaseCharacterChild = Cast<AVRBaseCharacter>(ChildComponent->GetOwner()))
		{
			InitialCharacterCameraPosition = BaseCharacterChild->VRReplicatedCamera->RelativeLocation;
		}
	}

	virtual void OnChildDetached(USceneComponent* ChildComponent) 
	{
		Super::OnChildDetached(ChildComponent);
	}

	void UpdatePosition()
	{
	}

};