// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "GripMotionControllerComponent.h"

#include "DLibLibrary.generated.h"

class UPrimitiveComponent;
class UGripMotionController;

USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct TORCHLITE_API FGripTrackedInfoStruct
{
	GENERATED_BODY()
public:

	// Target transform we are trying for
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FTransform TargetTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float TotalMass = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FVector COMPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FTransform GripPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	float BoundsRadius = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FVector CenterOfBounds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FVector LinearVelocity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FVector AngularVelocity;

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class TORCHLITE_API ATorchTrainer : public AActor
{
	GENERATED_BODY()

public:
	ATorchTrainer(const FObjectInitializer& ObjectInitializer)
	{
		PrimaryActorTick.bStartWithTickEnabled = false;
		PrimaryActorTick.bCanEverTick = true;
		//this->SetActorTickEnabled(false);
	}

	~ATorchTrainer()
	{

	}


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	TObjectPtr<UGripMotionControllerComponent> TargetController;

	UFUNCTION(BlueprintCallable, Category = "GripMotionController")
	void SetTargetController(UGripMotionControllerComponent* Target);

	virtual void BeginPlay() override
	{
		Super::BeginPlay();
	}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);
	}

	virtual void Tick(float DeltaTime) override;

	FGripTrackedInfoStruct GetTargetFrameProperties();
};

UCLASS()
class UDLibLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Inserts Current Object State
	//UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	//static void SetTargetFrameProperties(UPrimitiveComponent);
	
	// Runs a test function on DLib
	UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	static FString Train();

	// Runs a test function on DLib
	UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	static void Play();

};	
