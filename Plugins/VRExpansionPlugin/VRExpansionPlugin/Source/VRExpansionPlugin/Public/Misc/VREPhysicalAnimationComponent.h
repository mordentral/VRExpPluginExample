#pragma once

#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "EngineDefines.h"
#include "PhysicsEngine/ConstraintInstance.h"
#include "VREPhysicalAnimationComponent.generated.h"

USTRUCT()
struct VREXPANSIONPLUGIN_API FWeldedBoneDriverData
{
	GENERATED_BODY()
public:
	FTransform RelativeTransform;
	FName BoneName;
	FPhysicsShapeHandle ShapeHandle;

	FTransform LastLocal;

	FWeldedBoneDriverData() :
		RelativeTransform(FTransform::Identity),
		BoneName(NAME_None)
	{
	}

	FORCEINLINE bool operator==(const FPhysicsShapeHandle& Other) const
	{
		return (ShapeHandle == Other);
	}
};


UCLASS(meta = (BlueprintSpawnableComponent), ClassGroup = Physics)
class VREXPANSIONPLUGIN_API UVREPhysicalAnimationComponent : public UPhysicalAnimationComponent
{
	GENERATED_UCLASS_BODY()

public:

	/** IF true then we will auto adjust the sleep settings of the body so that it won't rest during welded bone driving */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
		bool bAutoSetPhysicsSleepSensitivity;

	/** IF true then we will auto adjust the sleep settings of the body so that it won't rest during welded bone driving */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
		float SleepThresholdMultiplier;

	/** The Base bone to use as the bone driver root */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
		FName BaseWeldedBoneDriverName;

	UPROPERTY()
		TArray<FWeldedBoneDriverData> BoneDriverMap;

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetupWeldedBoneDriver(bool bReInit = false);

	void UpdateWeldedBoneDriver(float DeltaTime);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
};
