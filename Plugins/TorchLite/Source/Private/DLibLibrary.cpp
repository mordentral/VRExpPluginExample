// Fill out your copyright notice in the Description page of Project Settings.
#include "DLibLibrary.h"


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


FGripTrackedInfoStruct ATorchTrainer::GetTargetFrameProperties()
{
    FGripTrackedInfoStruct TrackedInfo;
    
    if (IsValid(TargetController))
    {
        if (FBPActorGripInformation* GripInfo = TargetController->GetFirstActiveGrip())
        {
            if (UPrimitiveComponent* PrimTarget = GripInfo->GetGripPrimitiveComponent())
            {
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
    }
}

void ATorchTrainer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Fill in the struct with the current frames information
    FGripTrackedInfoStruct CurrentFrameProperties = GetTargetFrameProperties();
}



class GridWorld {
public:
    int state; // Position in grid (0 to 4)
    const int goal = 4;

    GridWorld() { reset(); }

    int step(int action) {
        if (action == 1) state++;  // Move Right
        else if (action == 0) state--; // Move Left

        // Clip state to stay within grid (0-4)
        state = std::max(0, std::min(state, goal));

        // Reward: +10 if at goal, else -1
        return (state == goal) ? 10 : -1;
    }

    void reset() { state = 0; }
};

    // Define Q-Network using LibTorch
struct QNetwork : torch::nn::Module {
    torch::nn::Linear fc1{ nullptr }, fc2{ nullptr };

    QNetwork(int state_size, int action_size)
        : fc1(register_module("fc1", torch::nn::Linear(state_size, 16))),
        fc2(register_module("fc2", torch::nn::Linear(16, action_size))) {}

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(fc1->forward(x));
        return fc2->forward(x);
    }
};


FString  UDLibLibrary::Train()
{
    GridWorld env;
    QNetwork model(1, 2);  // 1 state, 2 actions (left, right)
    torch::optim::Adam optimizer(model.parameters(), 0.01);

    const int episodes = 1000;
    float gamma = 0.9, epsilon = 0.1;

    for (int episode = 0; episode < episodes; episode++) {
        env.reset();
        int state = env.state;

        for (int t = 0; t < 10; t++) {
            // Convert state to tensor
            torch::Tensor state_tensor = torch::tensor({ (float)state });

            // Choose action: epsilon-greedy
            int action;
            if ((rand() / double(RAND_MAX)) < epsilon) action = rand() % 2;
            else {
                auto q_values = model.forward(state_tensor);
                action = q_values.argmax().item<int>();
            }

            // Take step
            int reward = env.step(action);
            int next_state = env.state;
            torch::Tensor next_state_tensor = torch::tensor({ (float)next_state });

            // Compute target Q-value
            auto next_q_values = model.forward(next_state_tensor);
            float max_next_q = next_q_values.max().item<float>();
            float target_q = reward + gamma * max_next_q;

            // Compute loss
            auto q_values = model.forward(state_tensor);
            torch::Tensor loss = torch::mse_loss(q_values[action], torch::tensor(target_q));

            // Optimize model
            optimizer.zero_grad();
            loss.backward();
            optimizer.step();

            state = next_state;
            if (state == env.goal) break;
        }
    }
    std::cout << "Training Complete!" << std::endl;
	return FString();
}

void UDLibLibrary::Play() {
    GridWorld env;
    QNetwork model(1, 2);  // Load trained model here

    env.reset();
    while (env.state != env.goal) {
        torch::Tensor state_tensor = torch::tensor({ (float)env.state });
        int action = model.forward(state_tensor).argmax().item<int>();

        env.step(action);
        std::cout << "Agent moved to: " << env.state << std::endl;
    }
    std::cout << "Agent reached the goal!" << std::endl;
}