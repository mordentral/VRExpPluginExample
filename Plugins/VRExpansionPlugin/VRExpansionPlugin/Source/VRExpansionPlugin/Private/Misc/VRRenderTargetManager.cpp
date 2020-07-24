// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Misc/VRRenderTargetManager.h"

UVRRenderTargetManager::UVRRenderTargetManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	PollRelevancyTime = 0.1f;

	// http://alumni.media.mit.edu/~wad/color/palette.html

	ColorMap.Reserve(16);
	ColorMap.Add(FColor::Black, 0);
	ColorMap.Add(FColor::White, 1);
	ColorMap.Add(FColor(87, 87, 87), 2);		// DK Gray
	ColorMap.Add(FColor(173, 35, 35), 3);		// Red
	ColorMap.Add(FColor(42, 75, 215), 4);		// Blue
	ColorMap.Add(FColor(29, 105, 20), 5);		// Green
	ColorMap.Add(FColor(129, 74, 125), 6);		// Brown
	ColorMap.Add(FColor(129, 38, 192), 7);		// Purple
	ColorMap.Add(FColor(160, 160, 160), 8);		// Light Gray
	ColorMap.Add(FColor(129, 197, 122), 9);		// Light Green
	ColorMap.Add(FColor(157, 175, 255), 10);	// Light Blue
	ColorMap.Add(FColor(41, 208, 208), 11);		// Cyan
	ColorMap.Add(FColor(255, 146, 51), 12);		// Orange
	ColorMap.Add(FColor(255, 238, 51), 13);		// Yellow
	ColorMap.Add(FColor(233, 222, 187), 14);	// Tan
	ColorMap.Add(FColor(255, 205, 243), 15);	// Pink

	bIsStoringImage = false;
	RenderTarget = nullptr;
	RenderTargetWidth = 100;
	RenderTargetHeight = 100;
	ClearColor = FColor::White;

	bInitiallyReplicateTexture = false;
}

ARenderTargetReplicationProxy::ARenderTargetReplicationProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bOnlyRelevantToOwner = true;
	bNetUseOwnerRelevancy = true;
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetReplicateMovement(false);
}

bool ARenderTargetReplicationProxy::ReceiveTexture_Validate(const FBPVRReplicatedTextureStore& TextureData, UVRRenderTargetManager* OwningManager)
{
	return true;
}

void ARenderTargetReplicationProxy::ReceiveTexture_Implementation(const FBPVRReplicatedTextureStore& TextureData, UVRRenderTargetManager* OwningManager)
{
	if (OwningManager)
	{
		OwningManager->RenderTargetStore = TextureData;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Recieved Texture, byte count: %i"), TextureData.PackedData.Num()));
		OwningManager->DeCompressRenderTarget2D();
	}
}