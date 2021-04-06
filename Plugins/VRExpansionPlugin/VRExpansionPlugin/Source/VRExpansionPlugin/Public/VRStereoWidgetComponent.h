// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "VRBPDatatypes.h"
#include "VRGripInterface.h"
#include "Components/WidgetComponent.h"
#include "Components/StereoLayerComponent.h"
#include "Slate/WidgetRenderer.h"
#include "Blueprint/UserWidget.h"
#include "Components/StereoLayerComponent.h"
#include "Engine/TextureRenderTarget2D.h"
//#include "Animation/UMGSequencePlayer.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SViewport.h"
#include "StereoLayerShapes.h"

#include "VRStereoWidgetComponent.generated.h"


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRStereoWidgetRenderComponent : public UStereoLayerComponent
{
	GENERATED_BODY()

public:
	UVRStereoWidgetRenderComponent(const FObjectInitializer& ObjectInitializer);

	/** The class of User Widget to create and display an instance of */
	UPROPERTY(EditAnywhere, Category = UserInterface)
		TSubclassOf<UUserWidget> WidgetClass;

	/** The User Widget object displayed and managed by this component */
	UPROPERTY(Transient, DuplicateTransient)
		UUserWidget* Widget;

	/** The desired render scale of the widget */
	UPROPERTY(Transient, DuplicateTransient)
		float WidgetRenderScale;

	/** The Slate widget to be displayed by this component.  Only one of either Widget or SlateWidget can be used */
//	TSharedPtr<SWidget> SlateWidget;

	class FWidgetRenderer* WidgetRenderer;

	/** The render target being display */
	UPROPERTY(BlueprintReadOnly, Transient, DuplicateTransient)
		UTextureRenderTarget2D* RenderTarget;

	/** The slate window that contains the user widget content */
	TSharedPtr<class SVirtualWindow> SlateWindow;
	
	//TSharedRef<SWidget> SharedWidget;

	//float TargetGamma;

	// #TODO remove level from world in cleanup

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		RenderWidget(DeltaTime);

		IStereoLayers* StereoLayers;
		if (!GEngine->StereoRenderingDevice.IsValid() || (StereoLayers = GEngine->StereoRenderingDevice->GetStereoLayers()) == nullptr)
		{
			return;
		}

		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	virtual void BeginPlay() override 
	{
		Super::BeginPlay();

		if (WidgetClass.Get() != nullptr)
		{
			InitWidget();
		}

	}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);

		ReleaseResources();
	}

	void ReleaseResources()
	{

#if !UE_SERVER
		FWorldDelegates::LevelRemovedFromWorld.RemoveAll(this);
#endif
		if (Widget)
		{
		//	RemoveWidgetFromScreen();
			Widget = nullptr;
		}

		/*if (SharedWidget)
		{
			//RemoveWidgetFromScreen();
			SharedWidget = nullptr;
		}*/

		if (WidgetRenderer)
		{
			BeginCleanup(WidgetRenderer);
			WidgetRenderer = nullptr;
		}

		Texture = nullptr;
		bLiveTexture = false;

		if (SlateWindow.IsValid())
		{
			if (/*!CanReceiveHardwareInput() && */FSlateApplication::IsInitialized())
			{
				FSlateApplication::Get().UnregisterVirtualWindow(SlateWindow.ToSharedRef());
			}

			SlateWindow.Reset();
		}
	}

	virtual void DestroyComponent(bool bPromoteChildren/*= false*/) override
	{
		Super::DestroyComponent(bPromoteChildren);

		ReleaseResources();
	}

	UFUNCTION(BlueprintCallable)
	void SetWidgetAndInit(TSubclassOf<UUserWidget> NewWidgetClass)
	{
		WidgetClass = NewWidgetClass;
		InitWidget();

	}

	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
	{
		// If the InLevel is null, it's a signal that the entire world is about to disappear, so
		// go ahead and remove this widget from the viewport, it could be holding onto too many
		// dangerous actor references that won't carry over into the next world.
		if (InLevel == nullptr && InWorld == GetWorld())
		{
			ReleaseResources();
		}
	}

	void InitWidget()
	{
		if (IsTemplate())
			return;

#if !UE_SERVER
		FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &ThisClass::OnLevelRemovedFromWorld);
#endif
		if (IsRunningDedicatedServer())
			return;

		if (Widget && Widget->GetClass() == WidgetClass)
			return;

		if (Widget != nullptr)
		{
			Widget->MarkPendingKill();
			Widget = nullptr;
		}

		// Don't do any work if Slate is not initialized
		if (FSlateApplication::IsInitialized())
		{
			UWorld* World = GetWorld();

			if (WidgetClass && Widget == nullptr && World && !World->bIsTearingDown)
			{
				Widget = CreateWidget(GetWorld(), WidgetClass);
				//SetTickMode(TickMode);
			}

#if WITH_EDITOR
			if (Widget && !World->IsGameWorld())// && !bEditTimeUsable)
			{
				if (!GEnableVREditorHacks)
				{
					// Prevent native ticking of editor component previews
					Widget->SetDesignerFlags(EWidgetDesignFlags::Designing);
				}
			}
#endif

			if (Widget)
			{
				//SharedWidget = Widget->TakeWidget();
			}
		}
	}

	//FWidget::Render()
	//TSharedRef<SWidget>SharedWidget = Widget->TakeWidget();
	void RenderWidget(float DeltaTime)
	{
		if (!Widget)
			return;
		
		if (WidgetRenderer == nullptr)
		{
			const bool bUseGammaCorrection = true;
			WidgetRenderer = new FWidgetRenderer(bUseGammaCorrection);
			check(WidgetRenderer);
		}
		
			if (RenderTarget == nullptr)
		{
			RenderTarget = NewObject<UTextureRenderTarget2D>();
			check(RenderTarget);
			RenderTarget->AddToRoot();
			RenderTarget->ClearColor = FLinearColor::Transparent;
			RenderTarget->TargetGamma = 1.0f;//TargetGamma;
			FVector2D TargetSize = this->GetQuadSize();
			RenderTarget->InitCustomFormat(TargetSize.X, TargetSize.Y, PF_FloatRGBA, false);
			//FTextureRenderTargetResource* RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource();

		}

			FVector2D DrawSize = this->GetQuadSize();//WidgetToRender->GetDesiredSize();

			TSharedRef<SWidget> MyWidget = Widget->TakeWidget();

			// Create the SlateWindow if it doesn't exists
			bool bNeededNewWindow = false;
			if (!SlateWindow.IsValid())
			{
	

				SlateWindow = SNew(SVirtualWindow).Size(DrawSize);
				SlateWindow->SetIsFocusable(false);
				SlateWindow->SetVisibility(EVisibility::Visible);
				//RegisterWindow();

				if (Widget && !Widget->IsDesignTime())
				{
					if (UWorld* LocalWorld = GetWorld())
					{
						if (LocalWorld->IsGameWorld())
						{
							UGameInstance* GameInstance = LocalWorld->GetGameInstance();
							check(GameInstance);

							UGameViewportClient* GameViewportClient = GameInstance->GetGameViewportClient();
							if (GameViewportClient)
							{
								//const TSharedPtr<SWidget> WidEntry = StaticCastSharedPtr<SWidget>(GameViewportClient->GetGameViewportWidget());
								SlateWindow->AssignParentWidget(GameViewportClient->GetGameViewportWidget());
							}
						}
					}
				}


				SlateWindow->Resize(DrawSize);
				bNeededNewWindow = true;

				/*if (NewSlateWidget != CurrentSlateWidget || bNeededNewWindow)
				{
					CurrentSlateWidget = NewSlateWidget;*/
					SlateWindow->SetContent(MyWidget);
					//bRenderCleared = false;
					//bWidgetChanged = true;
				//}
			}



		WidgetRenderer->DrawWidget(RenderTarget, MyWidget, WidgetRenderScale, DrawSize, 0.0f);//DeltaTime);
		Texture = RenderTarget;
		bLiveTexture = true;	
	}

};



/**
* A widget component that displays the widget in a stereo layer instead of in worldspace.
* Currently this class uses a custom postion instead of the engines WorldLocked for stereo layers
* This is because world locked stereo layers don't account for player movement currently.
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRStereoWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UVRStereoWidgetComponent(const FObjectInitializer& ObjectInitializer);

	friend class FStereoLayerComponentVisualizer;

	~UVRStereoWidgetComponent();


	/** Specifies which shape of layer it is.  Note that some shapes will be supported only on certain platforms! **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, NoClear, Instanced, Category = "StereoLayer", DisplayName = "Stereo Layer Shape")
		UStereoLayerShape* Shape;

	void BeginDestroy() override;
	void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void DrawWidgetToRenderTarget(float DeltaTime) override;
	virtual TStructOnScope<FActorComponentInstanceData>  GetComponentInstanceData() const override;
	void ApplyVRComponentInstanceData(class FVRStereoWidgetComponentInstanceData* WidgetInstanceData);

	virtual void UpdateRenderTarget(FIntPoint DesiredRenderTargetSize) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;


	// If true forces the widget to render both stereo and world widgets
	// Overriden by the console command vr.ForceNoStereoWithVRWidgets if it is set to 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		bool bRenderBothStereoAndWorld;

	// If true, use Epics world locked stereo implementation instead of my own temp solution
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		bool bUseEpicsWorldLockedStereo;

	// If true, will cache and delay the transform adjustment for one frame in order to sync with the game thread better
	// Not for use with late updated parents.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		bool bDelayForRenderThread;

	// If true will not render or update until false
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		bool bIsSleeping;

	/**
	* Change the layer's render priority, higher priorities render on top of lower priorities
	* @param	InPriority: Priority value
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
		void SetPriority(int32 InPriority);

	// @return the render priority
	UFUNCTION(BlueprintCallable, Category = "Components|Stereo Layer")
		int32 GetPriority() const { return Priority; }

	/** True if the stereo layer needs to support depth intersections with the scene geometry, if available on the platform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		uint32 bSupportsDepth : 1;

	/** True if the texture should not use its own alpha channel (1.0 will be substituted) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		uint32 bNoAlphaChannel : 1;

	/** True if the quad should internally set it's Y value based on the set texture's dimensions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StereoLayer")
		uint32 bQuadPreserveTextureRatio : 1;

protected:
	/** Texture displayed on the stereo layer (is stereocopic textures are supported on the platfrom and more than one texture is provided, this will be the right eye) **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StereoLayer")
	//	class UTexture* Texture;

	// Forget left texture implementation
	/** Texture displayed on the stereo layer for left eye, if stereoscopic textures are supported on the platform **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StereoLayer | Cubemap Overlay Properties")
	//	class UTexture* LeftTexture;

public:
	/** Size of the rendered stereo layer quad **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer | Quad Overlay Properties")
	//	FVector2D StereoLayerQuadSize;

	/** UV coordinates mapped to the quad face **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer | Quad Overlay Properties")
		FBox2D UVRect;

	/** Radial size of the rendered stereo layer cylinder **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer | Cylinder Overlay Properties")
	//	float CylinderRadius;

	/** Arc angle for the stereo layer cylinder **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer | Cylinder Overlay Properties")
		//float CylinderOverlayArc;

	/** Height of the stereo layer cylinder **/
	///UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer | Cylinder Overlay Properties")
	//	int CylinderHeight;

	/** Specifies how and where the quad is rendered to the screen **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer")
	//TEnumAsByte<enum EStereoLayerType> StereoLayerType;

	// Forcing quad layer so that it works with the widget better
	/** Specifies which type of layer it is.  Note that some shapes will be supported only on certain platforms! **/
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer")
	//	TEnumAsByte<enum EStereoLayerShape> StereoLayerShape;

	/** Render priority among all stereo layers, higher priority render on top of lower priority **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category = "StereoLayer")
		int32 Priority;

	bool bShouldCreateProxy;

private:
	/** Dirty state determines whether the stereo layer needs updating **/
	bool bIsDirty;
	bool bDirtyRenderTarget;

	/** Texture needs to be marked for update **/
	bool bTextureNeedsUpdate;

	/** IStereoLayer id, 0 is unassigned **/
	uint32 LayerId;

	/** Last transform is cached to determine if the new frames transform has changed **/
	FTransform LastTransform;

	/** Last frames visiblity state **/
	bool bLastVisible;

};