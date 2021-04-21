#pragma once

#include "ComponentVisualizer.h"
#include "Grippables/HandSocketComponent.h"
#include "ActorEditorUtils.h"

/**Base class for clickable targeting editing proxies*/
struct VREXPANSIONEDITOR_API HHandSocketVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

    HHandSocketVisProxy(const UActorComponent* InComponent)
		: HComponentVisProxy(InComponent, HPP_Wireframe)
	{
		BoneIdx = 0;
		TargetBoneName = NAME_None;
	}

	uint32 BoneIdx;
	FName TargetBoneName;
};

class VREXPANSIONEDITOR_API FHandSocketVisualizer : public FComponentVisualizer
{
public:
	FHandSocketVisualizer()
	{
		CurrentlyEditingComponent = nullptr;
	}

	virtual ~FHandSocketVisualizer()
	{

	}

	TWeakObjectPtr<UHandSocketComponent> CurrentlyEditingComponent;
	//TWeakObjectPtr<HHandSocketVisProxy> LastVisProxy;
	FQuat CachedRotation;
	FName CurrentlySelectedBone;
	uint32 CurrentlySelectedBoneIdx;

	UHandSocketComponent* GetEditedHandComponent() const
	{ 
		return CurrentlyEditingComponent.Get();
	}


	bool SaveAnimationAsset(const FString& InAssetPath, const FString& InAssetName);


	bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override;

	bool IsVisualizingArchetype() const override;

	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual void EndEditing() override;
   // virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector&amp; OutLocation) const override;
   // virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix&amp; OutMatrix) const override;
   // virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector&amp; DeltaTranslate, FRotator&amp; DeltaRotate, FVector&amp; DeltaScale) override;
   // virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
   // virtual TSharedPtr GenerateContextMenu() const override;
    // End FComponentVisualizer interface

    /** Get the target component we are currently editing */
    //UTargetingComponent* GetEditedTargetingComponent() const;

private:
        /**Index of target in selected component*/
        //int32 CurrentlySelectedTarget;

    /**Output log commands*/
   // TSharedPtr TargetingComponentVisualizerActions;
};
