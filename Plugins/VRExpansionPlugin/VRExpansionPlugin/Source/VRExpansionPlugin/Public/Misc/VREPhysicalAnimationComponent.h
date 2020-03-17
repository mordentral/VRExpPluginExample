#pragma once

#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "EngineDefines.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "VREPhysicalAnimationComponent.generated.h"

class FPrimitiveDrawInterface;
class USkeletalMeshComponent;

namespace physx
{
	class PxRigidDynamic;
}


UCLASS(meta = (BlueprintSpawnableComponent), ClassGroup = Physics)
class VREXPANSIONPLUGIN_API UVREPhysicalAnimationComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	/** Sets the skeletal mesh we are driving through physical animation. Will erase any existing physical animation data. */
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);

	/** Applies the physical animation settings to the body given.*/
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void ApplyPhysicalAnimationSettings(FName BodyName, const FPhysicalAnimationData& PhysicalAnimationData);

	/** Applies the physical animation settings to the body given and all bodies below.*/
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void ApplyPhysicalAnimationSettingsBelow(FName BodyName, const FPhysicalAnimationData& PhysicalAnimationData, bool bIncludeSelf = true);

	/** Updates strength multiplyer and any active motors */
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetStrengthMultiplyer(float InStrengthMultiplyer);
	
	/** Multiplies the strength of any active motors. (can blend from 0-1 for example)*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalAnimation, meta = (ClampMin = "0"))
	float StrengthMultiplyer;

	/**
	*  Applies the physical animation profile to the body given and all bodies below.
	*  @param  BodyName			The body from which we'd like to start applying the physical animation profile. Finds all bodies below in the skeleton hierarchy. None implies all bodies
	*  @param  ProfileName		The physical animation profile we'd like to apply. For each body in the physics asset we search for physical animation settings with this name.
	*  @param  bIncludeSelf		Whether to include the provided body name in the list of bodies we act on (useful to ignore for cases where a root has multiple children)
	*  @param  bClearNotFound	If true, bodies without the given profile name will have any existing physical animation settings cleared. If false, bodies without the given profile name are left untouched.
	*/
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	void ApplyPhysicalAnimationProfileBelow(FName BodyName, FName ProfileName, bool bIncludeSelf = true, bool bClearNotFound = false);

	/** 
	 *	Returns the target transform for the given body. If physical animation component is not controlling this body, returns its current transform.
	 */
	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation, meta = (UnsafeDuringActorConstruction))
	FTransform GetBodyTargetTransform(FName BodyName) const;

	virtual void InitializeComponent() override;
	virtual void BeginDestroy() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

#if WITH_EDITOR
	void DebugDraw(FPrimitiveDrawInterface* PDI) const;
#endif

	FORCEINLINE USkeletalMeshComponent* GetSkeletalMesh() const { return SkeletalMeshComponent; }

private:
	UPROPERTY()
	USkeletalMeshComponent* SkeletalMeshComponent;

	struct FPhysicalAnimationInstanceDataVR
	{
		struct FConstraintInstance* ConstraintInstance;
#if WITH_PHYSX
		physx::PxRigidDynamic* TargetActor; // #PHYS2 Should prob change to FPhysicsActorReference?
#endif
	};

	/** constraints used to apply the drive data */
	TArray<FPhysicalAnimationInstanceDataVR> RuntimeInstanceData;
	TArray<FPhysicalAnimationData> DriveData;

	FDelegateHandle OnTeleportDelegateHandle;

	void UpdatePhysicsEngine();
	void UpdatePhysicsEngineImp();
	void ReleasePhysicsEngine();
	void InitComponent();

	void OnTeleport();
	void UpdateTargetActors(ETeleportType TeleportType);

	//static const FConstraintProfileProperties PhysicalAnimationProfileVR;

	bool bPhysicsEngineNeedsUpdating;
};
