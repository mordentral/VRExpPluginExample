// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VRExpansionPluginPrivatePCH.h"
#include "VRExpansionFunctionLibrary.h"
#include "TextureResource.h"
#include "Engine/Texture.h"
#include "IStereoLayers.h"
#include "IHeadMountedDisplay.h"
#include "VRStereoWidgetComponent.h"

  //=============================================================================
UVRStereoWidgetComponent::UVRStereoWidgetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
//	, bLiveTexture(false)
	, bSupportsDepth(false)
	, bNoAlphaChannel(false)
	//, Texture(nullptr)
	//, LeftTexture(nullptr)
	, bQuadPreserveTextureRatio(false)
	, StereoLayerQuadSize(FVector2D(500.0f, 500.0f))
	, UVRect(FBox2D(FVector2D(0.0f, 0.0f), FVector2D(1.0f, 1.0f)))
	//, CylinderRadius(100)
	//, CylinderOverlayArc(100)
	//, CylinderHeight(50)
	//, StereoLayerType(SLT_TrackerLocked)
	, StereoLayerShape(SLSH_QuadLayer)
	, Priority(0)
	, bIsDirty(true)
	, bTextureNeedsUpdate(false)
	, LayerId(0)
	, LastTransform(FTransform::Identity)
	, bLastVisible(false)
{

	// Replace quad size with DrawSize instead
	//StereoLayerQuadSize = DrawSize;

	//Texture = nullptr;
}

//=============================================================================
UVRStereoWidgetComponent::~UVRStereoWidgetComponent()
{
}

void UVRStereoWidgetComponent::BeginDestroy()
{
	Super::BeginDestroy();

	IStereoLayers* StereoLayers;
	if (LayerId && GEngine->HMDDevice.IsValid() && (StereoLayers = GEngine->HMDDevice->GetStereoLayers()) != nullptr)
	{
		StereoLayers->DestroyLayer(LayerId);
		LayerId = 0;
	}
}


void UVRStereoWidgetComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	IStereoLayers* StereoLayers;
	if (!UVRExpansionFunctionLibrary::IsInVREditorPreviewOrGame() || !GEngine->HMDDevice.IsValid() || (StereoLayers = GEngine->HMDDevice->GetStereoLayers()) == nullptr || !RenderTarget)
	{
		return;
	}



	FTransform Transform;
	// Never true until epic fixes back end code
	// #TODO: FIXME when they FIXIT
	if (false)//StereoLayerType == SLT_WorldLocked)
	{
		Transform = GetComponentTransform();
	}
	else
	{
		// Fix this when stereo world locked works again
		// Thanks to mitch for the temp work around idea
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		
		if (PC)
		{
			APawn * mpawn = PC->GetPawnOrSpectator();

			if (mpawn)
			{
				// Set transform to this relative transform
				Transform = GetComponentTransform().GetRelativeTransform(mpawn->GetActorTransform());
				// I might need to inverse X axis here to get it facing the correct way, we'll see

				//Transform = mpawn->GetActorTransform().GetRelativeTransform(GetComponentTransform());
			}
		}
		//
		//Transform = GetRelativeTransform();
	}

	// If the transform changed dirty the layer and push the new transform
	if (!bIsDirty && (bLastVisible != bVisible || FMemory::Memcmp(&LastTransform, &Transform, sizeof(Transform)) != 0))
	{
		bIsDirty = true;
	}

	bool bCurrVisible = bVisible;
	if (!RenderTarget || !RenderTarget->Resource)
	{
		bCurrVisible = false;
	}

	if (bIsDirty)
	{
		if (!bCurrVisible)
		{
			if (LayerId)
			{
				StereoLayers->DestroyLayer(LayerId);
				LayerId = 0;
			}
		}
		else
		{
			IStereoLayers::FLayerDesc LayerDsec;
			LayerDsec.Priority = Priority;
			LayerDsec.QuadSize = StereoLayerQuadSize;
			LayerDsec.UVRect = UVRect;
			LayerDsec.Transform = Transform;
			if (RenderTarget)
			{
				LayerDsec.Texture = RenderTarget->Resource->TextureRHI;
			}
			// Forget the left texture implementation
			//if (LeftTexture)
			//{
			//	LayerDsec.LeftTexture = LeftTexture->Resource->TextureRHI;
			//}


			const float ArcAngleRadians = FMath::DegreesToRadians(CylinderArcAngle);
			const float Radius = GetDrawSize().X / ArcAngleRadians;

			LayerDsec.CylinderSize = FVector2D(/*CylinderRadius*/Radius, /*CylinderOverlayArc*/CylinderArcAngle);
			
			// This needs to be auto set from variables, need to work on it
			LayerDsec.CylinderHeight = GetDrawSize().Y;//CylinderHeight;

			LayerDsec.Flags |= (/*bLiveTexture*/true) ? IStereoLayers::LAYER_FLAG_TEX_CONTINUOUS_UPDATE : 0;
			LayerDsec.Flags |= (bNoAlphaChannel) ? IStereoLayers::LAYER_FLAG_TEX_NO_ALPHA_CHANNEL : 0;
			LayerDsec.Flags |= (bQuadPreserveTextureRatio) ? IStereoLayers::LAYER_FLAG_QUAD_PRESERVE_TEX_RATIO : 0;
			LayerDsec.Flags |= (bSupportsDepth) ? IStereoLayers::LAYER_FLAG_SUPPORT_DEPTH : 0;

			// Fix this later when WorldLocked is no longer wrong.
			LayerDsec.PositionType = IStereoLayers::TrackerLocked;

			/*switch (StereoLayerType)
			{
			case SLT_WorldLocked:
				LayerDsec.PositionType = IStereoLayers::WorldLocked;
				break;
			case SLT_TrackerLocked:
				LayerDsec.PositionType = IStereoLayers::TrackerLocked;
				break;
			case SLT_FaceLocked:
				LayerDsec.PositionType = IStereoLayers::FaceLocked;
				break;
			}*/

			switch (GeometryMode)
			{
			case EWidgetGeometryMode::Cylinder:
			{
				LayerDsec.ShapeType = IStereoLayers::CylinderLayer;
			}break;
			case EWidgetGeometryMode::Plane:
			default:
			{
				LayerDsec.ShapeType = IStereoLayers::QuadLayer;
			}break;
			}

			// Can't use the cubemap with widgets currently, maybe look into it?
			/*switch (StereoLayerShape)
			{
			case SLSH_QuadLayer:
				LayerDsec.ShapeType = IStereoLayers::QuadLayer;
				break;

			case SLSH_CylinderLayer:
				LayerDsec.ShapeType = IStereoLayers::CylinderLayer;
				break;

			case SLSH_CubemapLayer:
				LayerDsec.ShapeType = IStereoLayers::CubemapLayer;
				break;
			default:
				break;
			}*/

			if (LayerId)
			{
				StereoLayers->SetLayerDesc(LayerId, LayerDsec);
			}
			else
			{
				LayerId = StereoLayers->CreateLayer(LayerDsec);
			}
		}
		LastTransform = Transform;
		bLastVisible = bCurrVisible;
		bIsDirty = false;
	}

	if (bTextureNeedsUpdate && LayerId)
	{
		StereoLayers->MarkTextureForUpdate(LayerId);
		bTextureNeedsUpdate = false;
	}
}


void UVRStereoWidgetComponent::SetQuadSize(FVector2D InQuadSize)
{
	if (StereoLayerQuadSize == InQuadSize)
	{
		return;
	}

	StereoLayerQuadSize = InQuadSize;
	bIsDirty = true;
}

void UVRStereoWidgetComponent::MarkTextureForUpdate()
{
	bTextureNeedsUpdate = true;
}

void UVRStereoWidgetComponent::SetPriority(int32 InPriority)
{
	if (Priority == InPriority)
	{
		return;
	}

	Priority = InPriority;
	bIsDirty = true;
}

void UVRStereoWidgetComponent::UpdateRenderTarget(FIntPoint DesiredRenderTargetSize)
{
	Super::UpdateRenderTarget(DesiredRenderTargetSize);

	//if (!Texture)
		//Texture = RenderTarget;
}


FPrimitiveSceneProxy* UVRStereoWidgetComponent::CreateSceneProxy()
{
	// If valid HMD and in VR preview or game, render as a stereo layer instead, so don't create the scene proxy.
	if (UVRExpansionFunctionLibrary::IsInVREditorPreviewOrGame() && GEngine->HMDDevice.IsValid() && (/*StereoLayers = */GEngine->HMDDevice->GetStereoLayers() != nullptr)/* || !Texture*/)
	{
		return nullptr;
	}

	// Else go ahead and create it so that it renders normally
	return Super::CreateSceneProxy();
}


class FVRStereoWidgetComponentInstanceData : public FSceneComponentInstanceData
{
public:
	FVRStereoWidgetComponentInstanceData(const UVRStereoWidgetComponent* SourceComponent)
		: FSceneComponentInstanceData(SourceComponent)
		, WidgetClass(SourceComponent->GetWidgetClass())
		, RenderTarget(SourceComponent->GetRenderTarget())
	{}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override
	{
		FSceneComponentInstanceData::ApplyToComponent(Component, CacheApplyPhase);
		CastChecked<UVRStereoWidgetComponent>(Component)->ApplyVRComponentInstanceData(this);
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		FSceneComponentInstanceData::AddReferencedObjects(Collector);

		UClass* WidgetUClass = *WidgetClass;
		Collector.AddReferencedObject(WidgetUClass);
		Collector.AddReferencedObject(RenderTarget);
	}

public:
	TSubclassOf<UUserWidget> WidgetClass;
	UTextureRenderTarget2D* RenderTarget;
};

FActorComponentInstanceData* UVRStereoWidgetComponent::GetComponentInstanceData() const
{
	return new FVRStereoWidgetComponentInstanceData(this);
}

void UVRStereoWidgetComponent::ApplyVRComponentInstanceData(FVRStereoWidgetComponentInstanceData* WidgetInstanceData)
{
	check(WidgetInstanceData);

	// Note: ApplyComponentInstanceData is called while the component is registered so the rendering thread is already using this component
	// That means all component state that is modified here must be mirrored on the scene proxy, which will be recreated to receive the changes later due to MarkRenderStateDirty.

	if (GetWidgetClass() != WidgetClass)
	{
		return;
	}

	RenderTarget = WidgetInstanceData->RenderTarget;

	// Also set the texture
	//Texture = RenderTarget;
	// Not needed anymore, just using the render target directly now

	if (MaterialInstance && RenderTarget)
	{
		MaterialInstance->SetTextureParameterValue("SlateUI", RenderTarget);
	}

	MarkRenderStateDirty();
}