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

	/** The Base bone to use as the bone driver root */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = WeldedBoneDriver)
		FName BaseWeldedBoneDriverName;

	UPROPERTY()
		TArray<FWeldedBoneDriverData> BoneDriverMap;

	UFUNCTION(BlueprintCallable, Category = PhysicalAnimation)
	void SetupWeldedBoneDriver();

	void UpdateWeldedBoneDriver(float DeltaTime);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
};
