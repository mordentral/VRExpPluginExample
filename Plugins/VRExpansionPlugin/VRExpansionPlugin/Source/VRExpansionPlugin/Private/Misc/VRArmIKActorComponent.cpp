// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Misc/VRArmIKActorComponent.h"

UVRArmIKActorComponent::UVRArmIKActorComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = true;

	referencePlayerHeightHmd = 1.94f * 100.f; // World To Meters idealy
	referencePlayerWidthWrist = /*1.39f*/ 2.f * 100.f; // World To Meters idealy
	playerHeightHmd = 1.94f * 100.f; // World To Meters idealy
	playerWidthWrist = 1.39f/*2.f*/ * 100.f; // World To Meters idealy
	playerWidthShoulders = 0.31f * 100.f; // World To Meters idealy

	leftShoulderAnkerStartLocalPosition = FVector(0.f, -playerWidthShoulders / 2.f, 0.f);
	rightShoulderAnkerStartLocalPosition = FVector(0.f, playerWidthShoulders / 2.f, 0.f);

	loadPlayerSizeOnAwake = false;

	headNeckDistance = 0.03f * 100.f; // World To Meters idealy
	neckShoulderDistance = FVector(-0.02f, 0.f, -.15f) * 100.f; // World To Meters idealy

	bEnableDistinctShoulderRotation = true;
	distinctShoulderRotationMultiplier = 30.f;
	distinctShoulderRotationLimitForward = 33.f;
	distinctShoulderRotationLimitBackward = 0.f;
	distinctShoulderRotationLimitUpward = 33.f;
	bShoulderDislocated = false;
	bEnableShoulderDislocation = false;
	rightRotationStartHeight = 0.f;
	rightRotationHeightFactor = 142.f;
	rightRotationHeadRotationFactor = 0.3f;
	rightRotationHeadRotationOffset = -20.f;
	startShoulderDislocationBefore = 0.005f * 100.f; // World To Meters idealy
	headNeckDirectionVector = FVector(-.05f, 0.f, -1.f);
	bIgnoreZPos = true;


	HeadOffset = FVector(-5.f, 0.f, -5.f);
}