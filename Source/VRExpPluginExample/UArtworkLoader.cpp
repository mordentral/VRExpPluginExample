// Fill out your copyright notice in the Description page of Project Settings.


#include "UArtworkLoader.h"

void UArtworkLoader::LoadArtwork() {
    FString test = OpenFileDialog();
    UTexture2D* tex = LoadTextureFromFile(test);
    CreateArtPiece(tex);
}

FString UArtworkLoader::OpenFileDialog()
{
    // Ensure the platform module is available
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        // Setup the file dialog filters (e.g., PNG files)
        const FString FileTypes = TEXT("PNG Files|.png|JPEG Files|.jpg|All Files|.");

        // Variable to hold the selected file path
        TArray<FString> SelectedFile;

        // Open the file dialog
        bool bOpened = DesktopPlatform->OpenFileDialog(
            nullptr,                            // Parent window handle (null means no parent)
            TEXT("Select a PNG file"),          // Dialog title
            TEXT(""),                           // Initial directory (empty means default directory)
            TEXT(""),                           // Default file name (optional)
            TEXT(""),                          // Filter for allowed file types
            EFileDialogFlags::None,             // Flags (use None unless you want additional behavior)
            SelectedFile                        // The path of the selected file
        );

        // Return the file path if a file was selected, else return empty
        if (bOpened && !SelectedFile.IsEmpty() && !SelectedFile[0].IsEmpty())
        {
            return SelectedFile[0];
        }
    }

    return FString(); // Return an empty string if no file was selected
}

UTexture2D* UArtworkLoader::LoadTextureFromFile(const FString& FilePath)
{
    // Try to load the texture using the ImageUtils helper function
    UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(FilePath);
    return Texture;
}


void UArtworkLoader::CreateArtPiece(UTexture2D* ArtTexture) {
    if (ArtTexture)
    {
        UWorld* World = GEngine->GetWorldContexts()[0].World();
        /*auto* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(World, 0);
        FVector PlayerLocation = PlayerCharacter->GetActorLocation();
        FVector PlayerForward = PlayerCharacter->GetActorForwardVector();*/
        FVector SpawnLocation = FVector(0, 0, 400);//PlayerLocation + PlayerForward * 200.0f;
        // Get the current world
        if (World)
        {
            // Set up spawn parameters
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            // Spawn the ArtPieceActor at the specified location
            AArtPiece* NewArtPiece = World->SpawnActor<AArtPiece>(AArtPiece::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);

            if (NewArtPiece)
            {
                // Set the texture for the spawned art piece
                NewArtPiece->SetArtTexture(ArtTexture);
            }
        }
    }
}

// Create a material instance and set the texture parameter
//UMaterialInstanceDynamic* UArtworkLoader::CreateMaterialWithTexture(UTexture2D* Texture)
//{
    ////if (!Texture) return nullptr;

    ////// Find the base material in the content folder
    ////UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Script/Engine.Material'/Game/VRE/Misc/Meshes/CubeMaterial.CubeMaterial'"));

    ////if (!BaseMaterial) return nullptr;

    ////// Create a dynamic material instance
    ////UMaterialInstanceDynamic* MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial);

    ////// Set the texture parameter
    ////if (MaterialInstance)
    ////{

    ////    TArray<AActor> FoundActors;
    ////    TArray<AActor> ExactClimbWallActors;
    ////    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

    ////    MaterialInstance->SetTextureParameterValue(FName("Param"), Texture);
    ////    // Apply the material to the mesh component

    ////    for (AActor* Actor : FoundActors)
    ////    {
    ////        TArray<UActorComponent> Components;
    ////        Actor->GetComponents(Components);
    ////        for (UActorComponent Component : Components)
    ////        {
    ////            UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Component);
    ////            if (MeshComponent && MeshComponent->GetName() == "StaticMeshComponent0")
    ////            {
    ////                MeshComponent->SetMaterial(0, MaterialInstance);
    ////            }
    ////        }
    ////    }
    ////}
    /*return nullptr;
}*/