// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/Actor.h"
#include "GripMotionControllerComponent.h"

#pragma warning(push)
#pragma warning(disable: 4800)
#pragma warning(disable: 4668)
#pragma warning(disable: 4686)
#pragma warning(disable: 4541)
#pragma warning(disable: 4702)
#pragma push_macro("check")
#undef check
#pragma push_macro("ensure")
#undef ensure
//THIRD_PARTY_INCLUDES_START
#include <torch/script.h>
#include <torch/torch.h>
//THIRD_PARTY_INCLUDES_END
#pragma pop_macro("ensure")
#pragma pop_macro("check")
#pragma warning(pop)

#include "DLibLibrary.generated.h"

class UPrimitiveComponent;
class UGripMotionController;
class ATorchTrainer;



class PPOModel : public torch::nn::Module
{
public:
	static PPOModel& GetInstance() {
		static PPOModel instance;
		return instance;
	}

	torch::nn::Sequential& GetActorModel() { return Actor; }
	torch::nn::Sequential& GetCriticModel() { return Critic; }

	torch::optim::Adam& GetActorOptimizer() { return *ActorOptimizer.get(); }
	torch::optim::Adam& GetCriticOptimizer() { return *CriticOptimizer.get(); }

	torch::Tensor InputTensor;
	torch::Tensor OutputTensor;

	void SetInputState(FVector State)
	{
		InputTensor = torch::tensor({ State.X, State.Y, State.Z }).unsqueeze(0);
		// Ensure input tensor is on the correct device
		InputTensor = InputTensor.to(torch::kFloat32);

		if (GetActorModel()->size() > 0) 
		{ // Ensure model is initialized
			OutputTensor = GetActorModel()->forward(InputTensor);
		}
	}

private:
	PPOModel()
	{
		// Initialize Actor Model
		Actor = register_module("actor", torch::nn::Sequential(
			torch::nn::Linear(3, 128),
			torch::nn::ReLU(),
			torch::nn::Linear(128, 64),
			torch::nn::ReLU(),
			torch::nn::Linear(64, 2),
			torch::nn::Tanh()
		));

		// Initialize Critic Model
		Critic = register_module("critic", torch::nn::Sequential(
			torch::nn::Linear(3, 128),
			torch::nn::ReLU(),
			torch::nn::Linear(128, 64),
			torch::nn::ReLU(),
			torch::nn::Linear(64, 1)
		));

		// Initialize optimizers AFTER models are created
		ActorOptimizer = std::make_unique<torch::optim::Adam>(Actor->parameters(), torch::optim::AdamOptions(1e-3));
		CriticOptimizer = std::make_unique<torch::optim::Adam>(Critic->parameters(), torch::optim::AdamOptions(1e-3));

	}

	// Models (Static, Single Instance)
	torch::nn::Sequential Actor{ nullptr };
	torch::nn::Sequential Critic{ nullptr };

	// Initialize optimizers correctly
	std::unique_ptr<torch::optim::Adam> ActorOptimizer;
	std::unique_ptr<torch::optim::Adam> CriticOptimizer;
};


USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct TORCHLITE_API FGripTrackedInfoStruct
{
	GENERATED_BODY()
public:

	// Objects Transform
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	FTransform CurrentTransform;

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



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	TObjectPtr<UPrimitiveComponent> TargetComponent;

};

/**
 * Tick function that calls UCharacterMovementComponent::PostPhysicsTickComponent
 **/
USTRUCT()
struct FATorchTrainerPostPhysicsTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	/** CharacterMovementComponent that is the target of this tick **/
	class ATorchTrainer* Target;

	/**
	 * Abstract function actually execute the tick.
	 * @param DeltaTime - frame time to advance, in seconds
	 * @param TickType - kind of tick for this frame
	 * @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
	 * @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completion of this task until certain child tasks are complete.
	 **/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;

	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage() override;

	/** Function used to describe this tick for active tick reporting. **/
	virtual FName DiagnosticContext(bool bDetailed) override;
};

template<>
struct TStructOpsTypeTraits<FATorchTrainerPostPhysicsTickFunction> : public TStructOpsTypeTraitsBase2<FATorchTrainerPostPhysicsTickFunction>
{
	enum
	{
		WithCopy = false
	};
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

		PostPhysicsTickFunction.bCanEverTick = true;
		PostPhysicsTickFunction.bStartWithTickEnabled = false;
		PostPhysicsTickFunction.SetTickFunctionEnable(false);
		PostPhysicsTickFunction.TickGroup = TG_PostPhysics;
	}

	~ATorchTrainer()
	{

	}

	/** Post-physics tick function for this character */
	UPROPERTY()
	struct FATorchTrainerPostPhysicsTickFunction PostPhysicsTickFunction;

	/** Tick function called after physics (sync scene) has finished simulation, before cloth */
	virtual void PostPhysicsTick(float DeltaTime, FATorchTrainerPostPhysicsTickFunction& ThisTickFunction);


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	TObjectPtr<UGripMotionControllerComponent> TargetController;

	UFUNCTION(BlueprintCallable, Category = "GripMotionController")
	void SetTargetController(UGripMotionControllerComponent* Target);

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		bool isEmpty = PPOModel::GetInstance().GetActorModel().is_empty();

	}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);
	}

	virtual void BeginDestroy() override
	{
		Super::BeginDestroy();
	}

	virtual void Tick(float DeltaTime) override;

	FGripTrackedInfoStruct GetTargetFrameProperties();

	//torch::Tensor InputTensor;
	//torch::Tensor OutputTensor;

	UPROPERTY(EditAnywhere)
	float ForceScale = 100.0f; // This likely needs to be treated differently based on mass

	void ApplyForce(FGripTrackedInfoStruct& CurrentFrameProperties, FVector Force);
	FVector GetState();
	float ComputeReward(FGripTrackedInfoStruct& CurrentFrameProperties);
	void TrainPPO();  // Add training function

	std::vector<std::tuple<torch::Tensor, torch::Tensor, float, torch::Tensor>> Memory;

	//std::shared_ptr<torch::nn::Sequential> Actor;
	//std::shared_ptr<torch::nn::Sequential> Critic;

	//std::unique_ptr<torch::optim::Adam> ActorOptimizer;
	//std::unique_ptr<torch::optim::Adam> CriticOptimizer;
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
	//UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	//static FString Train();

	// Runs a test function on DLib
	//UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	//static void Play();

};	