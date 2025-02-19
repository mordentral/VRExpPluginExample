// Fill out your copyright notice in the Description page of Project Settings.
#include "DLibLibrary.h"


FVector ActorPPO::PredictForce(FVector State) {
    auto InputTensor = torch::tensor({ State.X, State.Y, State.Z }).unsqueeze(0);
    auto OutputTensor = Model.forward({ InputTensor }).toTensor();
    return FVector(OutputTensor[0][0].item<float>(), OutputTensor[0][1].item<float>(), 0);
}


FGripTrackedInfoStruct ATorchTrainer::GetTargetFrameProperties()
{
    FGripTrackedInfoStruct TrackedInfo;
    
    if (IsValid(TargetController))
    {
        if (FBPActorGripInformation* GripInfo = TargetController->GetFirstActiveGrip())
        {
            if (UPrimitiveComponent* PrimTarget = GripInfo->GetGripPrimitiveComponent())
            {
                TrackedInfo.TargetComponent = PrimTarget;

                TrackedInfo.CurrentTransform = PrimTarget->GetComponentTransform();
                TrackedInfo.BoundsRadius = PrimTarget->Bounds.GetSphere().W; // World Space
                TrackedInfo.CenterOfBounds = PrimTarget->Bounds.GetSphere().Center; // Save instead of get twice
                TrackedInfo.COMPosition = PrimTarget->GetCenterOfMass(); // Bone name? - WorldSpace
                TrackedInfo.GripPosition = GripInfo->RelativeTransform; // Relative to the controller, need to invert it likely
                TrackedInfo.TargetTransform = GripInfo->RelativeTransform * TargetController->GetComponentTransform(); // Not using addition and ect, this is for training only
                TrackedInfo.TotalMass = PrimTarget->GetMass();
                TrackedInfo.LinearVelocity = PrimTarget->GetPhysicsLinearVelocity(); // In case we need these for a stable simulation
                TrackedInfo.AngularVelocity = PrimTarget->GetPhysicsAngularVelocityInRadians();
            }
        }
    }

    return TrackedInfo;
}

void ATorchTrainer::SetTargetController(UGripMotionControllerComponent* Target)
{
    if (IsValid(Target))
    {
        TargetController = Target;
        AddTickPrerequisiteComponent(Target);
        this->SetActorTickEnabled(true);

        PostPhysicsTickFunction.SetTickFunctionEnable(true);
    }
}

void ATorchTrainer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Fill in the struct with the current frames information
    FGripTrackedInfoStruct CurrentFrameProperties = GetTargetFrameProperties();

    // Implement training to position the object at the target transform
    
    // Get Action from PPO Policy
    FVector State = CurrentFrameProperties.CurrentTransform.GetLocation();
    InputTensor = torch::tensor({ State.X, State.Y, State.Z }).unsqueeze(0);
    OutputTensor = (*Actor)->forward(InputTensor);
    FVector Force(OutputTensor[0][0].item<float>(), OutputTensor[0][1].item<float>(), 0);

    // Apply Force - prob won't work with unlocked physics thread
    // Also need to look into handling COM and mass
    // Also need to expand all of this to handle rotation as well
    ApplyForce(CurrentFrameProperties, Force);
}

void ATorchTrainer::PostPhysicsTick(float DeltaTime, FATorchTrainerPostPhysicsTickFunction& ThisTickFunction)
{

    FGripTrackedInfoStruct CurrentFrameProperties = GetTargetFrameProperties();
    // Compute Reward
    float Reward = ComputeReward(CurrentFrameProperties);

    // Store Experience
    FVector NextState = CurrentFrameProperties.CurrentTransform.GetLocation();
    Memory.push_back({ InputTensor, OutputTensor, Reward, torch::tensor({NextState.X, NextState.Y, NextState.Z}) });

    // Train Every 100 Steps
    if (Memory.size() >= 100) {
        TrainPPO();
        Memory.clear();
    }



    //torch::save(Actor, "PPO_Model.pt");
}


// This needs to be called post physics
float ATorchTrainer::ComputeReward(FGripTrackedInfoStruct& CurrentFrameProperties) {
    return -FVector::Dist(CurrentFrameProperties.TargetComponent->GetComponentLocation(), CurrentFrameProperties.TargetTransform.GetLocation());
}

void ATorchTrainer::ApplyForce(FGripTrackedInfoStruct &CurrentFrameProperties, FVector Force) {
    FVector AdjustedForce = Force * ForceScale;
    CurrentFrameProperties.TargetComponent->AddForceAtLocation(AdjustedForce, CurrentFrameProperties.COMPosition); // Add Force At Location - COM?
}

void ATorchTrainer::TrainPPO() {
    for (auto& [state, action, reward, next_state] : Memory) {
        // Compute Advantage
        torch::Tensor Value = (*Critic)->forward(state);
        torch::Tensor NextValue = (*Critic)->forward(next_state);
        torch::Tensor Advantage = reward + 0.99 * NextValue - Value;

        // Actor Loss (PPO Clipped)
        torch::Tensor OldAction = (*Actor)->forward(state).detach();
        torch::Tensor Ratio = (action / OldAction).exp();
        torch::Tensor ClippedRatio = Ratio.clamp(0.8, 1.2);
        torch::Tensor ActorLoss = -torch::min(Ratio * Advantage, ClippedRatio * Advantage).mean();

        // Critic Loss
        torch::Tensor CriticLoss = torch::mse_loss(Value, reward + 0.99 * NextValue);

        // Optimize Actor
        ActorOptimizer->zero_grad();
        ActorLoss.backward();
        ActorOptimizer->step();

        // Optimize Critic
        CriticOptimizer->zero_grad();
        CriticLoss.backward();
        CriticOptimizer->step();
    }
}



void FATorchTrainerPostPhysicsTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
    FActorComponentTickFunction::ExecuteTickHelper(Target->GetRootComponent(), /*bTickInEditor=*/ false, DeltaTime, TickType, [this](float DilatedTime)
        {
            Target->PostPhysicsTick(DilatedTime, *this);
        });
}

/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
FString FATorchTrainerPostPhysicsTickFunction::DiagnosticMessage()
{
    return Target->GetFullName() + TEXT("[UCharacterMovementComponent::PostPhysicsTick]");
}

/** Function used to describe this tick for active tick reporting. **/
FName FATorchTrainerPostPhysicsTickFunction::DiagnosticContext(bool bDetailed)
{
    if (bDetailed)
    {
        return FName(*FString::Printf(TEXT("CharacterMovementComponentPostPhysicsTick/%s"), *GetFullNameSafe(Target)));
    }

    return FName(TEXT("CharacterMovementComponentPostPhysicsTick"));
}