// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Net/UnrealNetwork.h"
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

	TextureBlobSize = 512;
	MaxBytesPerSecondRate = 5000;

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

bool ARenderTargetReplicationProxy::ReceiveTexture_Validate(const FBPVRReplicatedTextureStore& TextureData)
{
	return true;
}

void ARenderTargetReplicationProxy::ReceiveTexture_Implementation(const FBPVRReplicatedTextureStore& TextureData)
{
	if (OwningManager.IsValid())
	{
		OwningManager->RenderTargetStore = TextureData;
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Recieved Texture, byte count: %i"), TextureData.PackedData.Num()));
		OwningManager->DeCompressRenderTarget2D();
	}
}

bool ARenderTargetReplicationProxy::InitTextureSend_Validate(int32 Width, int32 Height, int32 TotalDataCount, int32 BlobCount, EPixelFormat PixelFormat, bool bIsZipped)
{
	return true;
}

void ARenderTargetReplicationProxy::InitTextureSend_Implementation(int32 Width, int32 Height, int32 TotalDataCount, int32 BlobCount, EPixelFormat PixelFormat, bool bIsZipped)
{
	TextureStore.Reset();
	TextureStore.PixelFormat = PixelFormat;
	TextureStore.bIsZipped = bIsZipped;
	TextureStore.Width = Width;
	TextureStore.Height = Height;

	TextureStore.PackedData.Reset(TotalDataCount);
	TextureStore.PackedData.AddUninitialized(TotalDataCount);

	BlobNum = BlobCount;

	Ack_InitTextureSend(TotalDataCount);
}

bool ARenderTargetReplicationProxy::Ack_InitTextureSend_Validate(int32 TotalDataCount)
{
	return true;
}

void ARenderTargetReplicationProxy::Ack_InitTextureSend_Implementation(int32 TotalDataCount)
{
	if (TotalDataCount == TextureStore.PackedData.Num())
	{
		BlobNum = 0;

		// Calculate time offset to achieve our max bytes per second with the given blob size
		float SendRate = 1.f / (MaxBytesPerSecondRate / (float)TextureBlobSize);

		GetWorld()->GetTimerManager().SetTimer(SendTimer_Handle, this, &ARenderTargetReplicationProxy::SendNextDataBlob, SendRate, true);

		// Start sending data blobs
		//SendNextDataBlob();
	}
}
void ARenderTargetReplicationProxy::SendInitMessage()
{
	int32 TotalBlobs = TextureStore.PackedData.Num() / TextureBlobSize + (TextureStore.PackedData.Num() % TextureBlobSize > 0 ? 1 : 0);

	InitTextureSend(TextureStore.Width, TextureStore.Height, TextureStore.PackedData.Num(), TotalBlobs, TextureStore.PixelFormat, TextureStore.bIsZipped);

}

void ARenderTargetReplicationProxy::SendNextDataBlob()
{
	if (this->IsPendingKill() || !this->GetOwner() || this->GetOwner()->IsPendingKill())
	{	
		TextureStore.Reset();
		TextureStore.PackedData.Empty();
		TextureStore.UnpackedData.Empty();
		BlobNum = 0;
		if (SendTimer_Handle.IsValid())
			GetWorld()->GetTimerManager().ClearTimer(SendTimer_Handle);

		return;
	}

	BlobNum++;
	int32 TotalBlobs = TextureStore.PackedData.Num() / TextureBlobSize + (TextureStore.PackedData.Num() % TextureBlobSize > 0 ? 1 : 0);

	if (BlobNum <= TotalBlobs)
	{
		TArray<uint8> BlobStore;
		int32 BlobLen = (BlobNum == TotalBlobs ? TextureStore.PackedData.Num() % TextureBlobSize : TextureBlobSize);


		BlobStore.AddUninitialized(BlobLen);
		uint8* MemLoc = TextureStore.PackedData.GetData();
		int32 MemCount = (BlobNum - 1) * TextureBlobSize;
		MemLoc += MemCount;

		FMemory::Memcpy(BlobStore.GetData(), MemLoc, BlobLen);

		ReceiveTextureBlob(BlobStore, MemCount, BlobNum);
	}
	else
	{
		TextureStore.Reset();
		TextureStore.PackedData.Empty();
		TextureStore.UnpackedData.Empty();
		if (SendTimer_Handle.IsValid())
			GetWorld()->GetTimerManager().ClearTimer(SendTimer_Handle);
		BlobNum = 0;
	}
}

//=============================================================================
void ARenderTargetReplicationProxy::GetLifetimeReplicatedProps(TArray< class FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);


	DOREPLIFETIME(ARenderTargetReplicationProxy, OwningManager);
}

bool ARenderTargetReplicationProxy::ReceiveTextureBlob_Validate(const TArray<uint8>& TextureBlob, int32 LocationInData, int32 BlobNumber)
{
	return true;
}

void ARenderTargetReplicationProxy::ReceiveTextureBlob_Implementation(const TArray<uint8>& TextureBlob, int32 LocationInData, int32 BlobNumber)
{
	if (LocationInData + TextureBlob.Num() <= TextureStore.PackedData.Num())
	{
		uint8* MemLoc = TextureStore.PackedData.GetData();
		MemLoc += LocationInData;
		FMemory::Memcpy(MemLoc, TextureBlob.GetData(), TextureBlob.Num());

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Recieved Texture blob, byte count: %i"), TextureBlob.Num()));
	}

	if (BlobNumber == BlobNum)
	{
		Ack_ReceiveTextureBlob(BlobNum);

		// We finished, unpack and display
		if (OwningManager.IsValid())
		{
			OwningManager->RenderTargetStore = TextureStore;
			TextureStore.Reset();
			TextureStore.PackedData.Empty();
			TextureStore.UnpackedData.Empty();
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Recieved Texture, total byte count: %i"), OwningManager->RenderTargetStore.PackedData.Num()));
			OwningManager->DeCompressRenderTarget2D();
		}
	}

}

bool ARenderTargetReplicationProxy::Ack_ReceiveTextureBlob_Validate(int32 BlobCount)
{
	return true;
}

void ARenderTargetReplicationProxy::Ack_ReceiveTextureBlob_Implementation(int32 BlobCount)
{
	// Send next data blob
	//SendNextDataBlob();

}