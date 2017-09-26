// This class is intended to provide support for Local Mixed play between a mouse and keyboard player
// and a VR player. It is not needed outside of that use.

#pragma once
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/Engine.h"
#include "CoreMinimal.h"
#include "IHeadMountedDisplay.h"

#include "VRSceneCaptureComponent2D.generated.h"


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = VRExpansionLibrary)
class VREXPANSIONPLUGIN_API UVRSceneCaptureComponent2D : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:

	// Toggles applying late HMD positional / rotational updates to the capture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRExpansionPlugin")
		bool bTrackLocalHMDOrCamera;

	//virtual void UpdateSceneCaptureContents(USceneCaptureComponent2D* CaptureComponent) override
	virtual void UpdateSceneCaptureContents(FSceneInterface* Scene) override
	{

		// Apply eye matrix
		// Apply eye offset
		// Apply late update

		//SceneCaptureR->ClipPlaneBase = Root->GetComponentLocation();
		//SceneCaptureR->ClipPlaneNormal = Root->GetForwardVector();

		if (bTrackLocalHMDOrCamera)
		{

			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Tracking local camera!"));

			FQuat Orientation = FQuat::Identity;
			FVector Position = FVector::ZeroVector;

			if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D() && GEngine->HMDDevice->IsHeadTrackingAllowed() && GEngine->HMDDevice->HasValidTrackingPosition())
			{
				GEngine->HMDDevice->GetCurrentOrientationAndPosition(Orientation, Position);

				float WorldToMeters = GetWorld() ? GetWorld()->GetWorldSettings()->WorldToMeters : 100.0f;

				GEngine->StereoRenderingDevice->CalculateStereoViewOffset(EStereoscopicPass::eSSP_LEFT_EYE, Orientation.Rotator(), WorldToMeters /*1.0f?*/, Position);
				this->bUseCustomProjectionMatrix = true;

				//float HFOV, VFOV;
				//GEngine->HMDDevice->GetFieldOfView(HFOV, VFOV);
				// Query steamVR if above fails instead of falling back
				float fov = FMath::Max(105.756425f, 111.469296f);
				this->CustomProjectionMatrix = GEngine->HMDDevice.Get()->GetStereoProjectionMatrix(EStereoscopicPass::eSSP_LEFT_EYE, fov);
				this->CaptureStereoPass = EStereoscopicPass::eSSP_LEFT_EYE;

				//SceneCaptureL->SetWorldLocationAndRotation(LeftLoc, LeftRot);
				//GEngine->StereoRenderingDevice->CalculateStereoViewOffset(EStereoscopicPass::eSSP_RIGHT_EYE, RightRot, WorldToMeters /*1.0f?*/, RightLoc);
				//SceneCaptureR->SetWorldLocationAndRotation(RightLoc, RightRot);

				//SceneCaptureL->bUseCustomProjectionMatrix = true;
				//SceneCaptureL->CustomProjectionMatrix = GEngine->HMDDevice.Get()->GetStereoProjectionMatrix(EStereoscopicPass::eSSP_LEFT_EYE, SceneCaptureL->FOVAngle);

				//SceneCaptureR->bUseCustomProjectionMatrix = true;
				//SceneCaptureR->CustomProjectionMatrix = GEngine->HMDDevice.Get()->GetStereoProjectionMatrix(EStereoscopicPass::eSSP_RIGHT_EYE, SceneCaptureR->FOVAngle);
			}
			else
			{
				this->bUseCustomProjectionMatrix = false;
				this->CaptureStereoPass = EStereoscopicPass::eSSP_FULL;

				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("getting camera!"));

				APlayerController* Player = GetWorld()->GetFirstPlayerController();
				if (Player != nullptr && Player->IsLocalController())
				{
					if (Player->GetPawn())
					{
						for (UActorComponent* CamComponent : Player->GetPawn()->GetComponentsByClass(UCameraComponent::StaticClass()))
						{
							UCameraComponent * CameraComponent = Cast<UCameraComponent>(CamComponent);

							if (CameraComponent != nullptr && CameraComponent->bIsActive)
							{
								GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Got camera!"));
								FTransform trans = CameraComponent->GetRelativeTransform();

								Orientation = trans.GetRotation();
								Position = trans.GetTranslation();
								break;
							}
						}
					}
				}
			}

			/*	FTransform Transform = CaptureComponent->GetComponentToWorld();
				FVector ViewLocation = Transform.GetTranslation();

				// Remove the translation from Transform because we only need rotation.
				Transform.SetTranslation(FVector::ZeroVector);
				Transform.SetScale3D(FVector::OneVector);
				FMatrix ViewRotationMatrix = Transform.ToInverseMatrixWithScale();

				// swap axis st. x=z,y=x,z=y (unreal coord space) so that z is up
				ViewRotationMatrix = ViewRotationMatrix * FMatrix(
					FPlane(0, 0, 1, 0),
					FPlane(1, 0, 0, 0),
					FPlane(0, 1, 0, 0),
					FPlane(0, 0, 0, 1));
				const float FOV = CaptureComponent->FOVAngle * (float)PI / 360.0f;
				FIntPoint CaptureSize(CaptureComponent->TextureTarget->GetSurfaceWidth(), CaptureComponent->TextureTarget->GetSurfaceHeight());
				*/

			this->SetRelativeLocationAndRotation(Position, Orientation);
		}

		// This pulls from the GetComponentToWorld so setting just prior to it should have worked	
		Super::UpdateSceneCaptureContents(Scene);
		//Super::UpdateSceneCaptureContents(CaptureComponent);
	}

};