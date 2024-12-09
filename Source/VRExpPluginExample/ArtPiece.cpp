// Fill out your copyright notice in the Description page of Project Settings.


#include "ArtPiece.h"

// Sets default values
AArtPiece::AArtPiece()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    // Create the Static Mesh Component (the frame or surface)
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetSimulatePhysics(true);
    RootComponent = MeshComponent;

    // Set a default mesh (like a frame or wall)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CanvasMesh(TEXT("/Script/Engine.StaticMesh'/Game/VRArtGallery/canvasModel.canvasModel'"));
    if (CanvasMesh.Succeeded())
    {
        MeshComponent->SetStaticMesh(CanvasMesh.Object);
    }

}

// Called when the game starts or when spawned
void AArtPiece::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AArtPiece::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AArtPiece::SetArtTexture(UTexture2D* ArtTexture) {
    if (MeshComponent && ArtTexture)
    {

        // Create a dynamic material instance from the current material
        UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Script/Engine.Material'/Game/VRArtGallery/ArtPieceMaterial.ArtPieceMaterial'"));
        UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);

        // Set the texture parameter dynamically
        DynamicMaterial->SetTextureParameterValue(FName("ArtTexture"), ArtTexture);

        // Apply the dynamic material to the mesh
        MeshComponent->SetMaterial(0, DynamicMaterial);

        int32 Width = ArtTexture->GetSizeX();
        int32 Height = ArtTexture->GetSizeY();
        ResizeToTexture(Width, Height);

        //TArray<AActor*> FoundActors;
        //TArray<AActor*> ExactClimbWallActors;
        //UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

        //DynamicMaterial->SetTextureParameterValue(FName("ArtTexture"), ArtTexture);
        //// Apply the material to the mesh component

        //for (AActor* Actor : FoundActors)
        //{
        //    TArray<UActorComponent> Components;
        //    Actor->GetComponents(Components);
        //    for (UActorComponent Component : Components)
        //    {
        //        UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component);
        //        if (Mesh && Mesh->GetName() == "StaticMeshComponent0")
        //        {
        //            Mesh->SetMaterial(0, DynamicMaterial);
        //        }
        //    }
        //}

    }
}

void AArtPiece::ResizeToTexture(int32 Width, int32 Height) {
    if (MeshComponent)
    {
        float ratio = (float)Width / Height;

        FVector NewScale = MeshComponent->GetOwner()->GetActorScale3D();

        if (ratio > 1)
            NewScale.Z *= 1 / ratio;
        else
            NewScale.X *= ratio;

        MeshComponent->GetOwner()->SetActorScale3D(NewScale);
    }
}

