// This class is intended to automate offsetting a character in a seated position easily and without 
// Massive effort on the users part.

#pragma once
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "CoreMinimal.h"
#include "IHeadMountedDisplay.h"
#include "VRBaseCharacter.h"

#include "VRSeatComponent.generated.h"

/*USTRUCT(Blueprintable)
struct VREXPANSIONPLUGIN_API FVRSeatedCharacterInfo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY()
		AVRBaseCharacter * SeatedCharacter;
	//UPROPERTY()
	//	FVector_NetQuantize100 OriginalRelativeLocation;
	UPROPERTY()
		float OriginalRotationYaw;

	void Clear()
	{
		SeatedCharacter = nullptr;
	//	OriginalRelativeLocation = FVector::ZeroVector;
		OriginalRotationYaw = 0;
	}


	// Doing a custom NetSerialize here because this is sent via RPCs and should change on every update
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		Ar << SeatedCharacter;
		//OriginalRelativeLocation.NetSerialize(Ar, Map, bOutSuccess);

		uint16 val;
		if (Ar.IsSaving())
		{
			val = FRotator::CompressAxisToShort(OriginalRotationYaw);
			Ar << val;
		}
		else
		{
			Ar << val;
			OriginalRotationYaw = FRotator::DecompressAxisFromShort(val);
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
};*/



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

	UPROPERTY(BlueprintReadOnly, Replicated, EditAnywhere, Category = "VRSeatComponent", ReplicatedUsing = OnRep_SeatedCharInfo)
		FVRSeatedCharacterInfo SeatedCharacter;

	UFUNCTION()
	virtual void OnRep_SeatedCharInfo()
	{
		// Handle setting up the player here

		SeatedCharacter.SeatedCharacter->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ZeroSeatedPlayer();
	}

	void ZeroSeatedPlayer()
	{
		if (SeatedCharacter.SeatedCharacter != nullptr)
		{
			SeatedCharacter.SeatedCharacter->SetActorLocationAndRotationVR(SeatedCharacter.SeatedCharacter->GetVRLocation() - this->GetComponentLocation(), FRotator(0.0f, -SeatedCharacter.OriginalRotationYaw, 0.0f), true);
		}
	}

	void CheckCurrentOffset()
	{
		// Darken the camera if locally controlled, otherwise just move back into range
	}

	UFUNCTION(BlueprintCallable, Category = "VRSeatComponent")
	bool SeatPlayer(AVRBaseCharacter * CharacterToSeat)
	{
		if (CharacterToSeat == nullptr)
			return false;

		AController* OwningController = CharacterToSeat->GetController();

		SeatedCharacter.SeatedCharacter = CharacterToSeat;
		SeatedCharacter.OriginalRotationYaw = UVRExpansionFunctionLibrary::GetHMDPureYaw_I(CharacterToSeat->bUseControllerRotationYaw && OwningController ? OwningController->GetControlRotation() : CharacterToSeat->GetActorRotation()).Yaw;
		OnRep_SeatedCharInfo(); // Call this on server side because it won't call itself

		SeatedCharacter.SeatedCharacter->SetReplicateMovement(false);
		return true;
	}

	// Need to call this on local client to set the rotation....
	UFUNCTION(BlueprintCallable, Category = "VRSeatComponent")
	bool UnSeatPlayer(FVector NewWorldLocation, FRotator NewWorldRotation)
	{
		if (SeatedCharacter.SeatedCharacter == nullptr)
			return false;

		SeatedCharacter.SeatedCharacter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SeatedCharacter.SeatedCharacter->SetActorLocationAndRotationVR(NewWorldLocation - (SeatedCharacter.SeatedCharacter->GetVRLocation() - SeatedCharacter.SeatedCharacter->GetActorLocation()), NewWorldRotation, true);
		SeatedCharacter.SeatedCharacter->SetReplicateMovement(true);
		return true;
	}
};