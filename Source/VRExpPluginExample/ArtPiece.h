// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DesktopPlatformModule.h"
#include "IPlatformFilePak.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "ImageUtils.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"


#include "ArtPiece.generated.h"

UCLASS()
class VREXPPLUGINEXAMPLE_API AArtPiece : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArtPiece();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* MeshComponent;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Art Gallery")
	void SetArtTexture(UTexture2D* ArtTexture);

	void ResizeToTexture(int32 Width, int32 Height);

};
