// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Misc/OptionalRepSkeletalMeshActor.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsReplication.h"
#if WITH_PUSH_MODEL
#include "Net/Core/PushModel/PushModel.h"
#endif

void FSkeletalMeshComponentEndPhysicsTickFunctionVR::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	//QUICK_SCOPE_CYCLE_COUNTER(FSkeletalMeshComponentEndPhysicsTickFunction_ExecuteTick);
	//CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Physics);

	FActorComponentTickFunction::ExecuteTickHelper(TargetVR, /*bTickInEditor=*/ false, DeltaTime, TickType, [this](float DilatedTime)
		{
			TargetVR->EndPhysicsTickComponentVR(*this);
		});
}

FString FSkeletalMeshComponentEndPhysicsTickFunctionVR::DiagnosticMessage()
{
	if (TargetVR)
	{
		return TargetVR->GetFullName() + TEXT("[EndPhysicsTickVR]");
	}
	return TEXT("<NULL>[EndPhysicsTick]");
}

FName FSkeletalMeshComponentEndPhysicsTickFunctionVR::DiagnosticContext(bool bDetailed)
{
	return FName(TEXT("SkeletalMeshComponentEndPhysicsTickVR"));
}


UInversePhysicsSkeletalMeshComponent::UInversePhysicsSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicateMovement = true;
	this->EndPhysicsTickFunction.bCanEverTick = false;

	EndPhysicsTickFunctionVR.TickGroup = TG_EndPhysics;
	EndPhysicsTickFunctionVR.bCanEverTick = true;
	EndPhysicsTickFunctionVR.bStartWithTickEnabled = true;
}

void UInversePhysicsSkeletalMeshComponent::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

	// Don't replicate if set to not do it
	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeLocation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeRotation, bReplicateMovement);
	DOREPLIFETIME_ACTIVE_OVERRIDE_PRIVATE_PROPERTY(USceneComponent, RelativeScale3D, bReplicateMovement);
}

void UInversePhysicsSkeletalMeshComponent::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInversePhysicsSkeletalMeshComponent, bReplicateMovement);
}

AOptionalRepGrippableSkeletalMeshActor::AOptionalRepGrippableSkeletalMeshActor(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer.SetDefaultSubobjectClass<UInversePhysicsSkeletalMeshComponent>(TEXT("SkeletalMeshComponent0")))
{
	bIgnoreAttachmentReplication = false;
	bIgnorePhysicsReplication = false;
}

void AOptionalRepGrippableSkeletalMeshActor::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOptionalRepGrippableSkeletalMeshActor, bIgnoreAttachmentReplication);
	DOREPLIFETIME(AOptionalRepGrippableSkeletalMeshActor, bIgnorePhysicsReplication);

	if (bIgnoreAttachmentReplication)
	{
		RESET_REPLIFETIME_CONDITION_PRIVATE_PROPERTY(AActor, AttachmentReplication, COND_InitialOnly);
	}
	//DISABLE_REPLICATED_PRIVATE_PROPERTY(AActor, AttachmentReplication);
}

void AOptionalRepGrippableSkeletalMeshActor::OnRep_ReplicateMovement()
{
	if (bIgnorePhysicsReplication)
	{
		return;
	}

	Super::OnRep_ReplicateMovement();
}

void AOptionalRepGrippableSkeletalMeshActor::PostNetReceivePhysicState()
{
	if (bIgnorePhysicsReplication)
	{
		return;
	}

	Super::PostNetReceivePhysicState();
}