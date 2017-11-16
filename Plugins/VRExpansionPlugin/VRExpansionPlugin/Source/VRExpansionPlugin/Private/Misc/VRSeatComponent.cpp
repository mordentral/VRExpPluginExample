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
