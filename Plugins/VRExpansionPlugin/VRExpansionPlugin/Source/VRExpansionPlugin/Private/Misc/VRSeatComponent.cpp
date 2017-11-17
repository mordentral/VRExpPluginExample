// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VRSeatComponent.h"
//#include "Net/UnrealNetwork.h"

  //=============================================================================
UVRSeatComponent::UVRSeatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//=============================================================================
UVRSeatComponent::~UVRSeatComponent()
{
}

void UVRSeatComponent::BeginPlay()
{
	// Call the base class 
	Super::BeginPlay();

	ResetInitialRelativePosition();
}

void UVRSeatComponent::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Skipping the owner with this as the owner will use the controllers location directly
	DOREPLIFETIME(UVRSeatComponent, SeatedCharacter);
}
