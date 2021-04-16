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
		bIsCore = false;
	}

	bool bIsCore;
};

IMPLEMENT_HIT_PROXY(HHandSocketVisProxy, HComponentVisProxy);
#define LOCTEXT_NAMESPACE "HandSocketVisualizer"

class VREXPANSIONEDITOR_API FHandSocketVisualizer : public FComponentVisualizer
{
public:
	FHandSocketVisualizer()
	{
		bIsSelected = false;
		CurrentlyEditingComponent = nullptr;
	}

	virtual ~FHandSocketVisualizer()
	{

	}

	bool bIsSelected;
	TWeakObjectPtr<UHandSocketComponent> CurrentlyEditingComponent;
	FQuat CachedRotation;

	UHandSocketComponent* GetEditedHandComponent() const
	{ 
		return CurrentlyEditingComponent.Get();
	}

	bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override
	{
		if (CurrentlyEditingComponent.IsValid() && bIsSelected)
		{
			if (ViewportClient->GetWidgetCoordSystemSpace() == COORD_Local || ViewportClient->GetWidgetMode() == FWidget::WM_Rotate)
			{
				//USplineComponent* SplineComp = GetEditedSplineComponent();
				FTransform newTrans = CurrentlyEditingComponent->HandRelativePlacement * CurrentlyEditingComponent->GetComponentTransform();
				OutMatrix = FRotationMatrix::Make(newTrans.GetRotation());
				return true;
			}
		}

		return false;
	}

	bool IsVisualizingArchetype() const override
	{
		return (CurrentlyEditingComponent.IsValid() && CurrentlyEditingComponent->GetOwner() && FActorEditorUtils::IsAPreviewOrInactiveActor(CurrentlyEditingComponent->GetOwner()));
	}

    // Begin FComponentVisualizer interface
   // virtual void OnRegister() override;
    virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
	{
		//UWorld* World = Component->GetWorld();
		//return World && (World->WorldType == EWorldType::EditorPreview || World->WorldType == EWorldType::Inactive);

		//cast the component into the expected component type
		if (const UHandSocketComponent* HandComponent = Cast<UHandSocketComponent>(Component))
		{
			if (!HandComponent->HandVisualizerComponent)
				return;

			//CurrentlyEditingComponent = HandComponent;
			//const UHandSocketComponent* targetComp = Cast<const UHandSocketComponent>(VisProxy->Component.Get());
			UHandSocketComponent* currentHand = const_cast<UHandSocketComponent*>(HandComponent);
			CurrentlyEditingComponent = currentHand;

			//get colors for selected and unselected targets
			//This is an editor only uproperty of our targeting component, that way we can change the colors if we can't see them against the background
			const FLinearColor SelectedColor = FLinearColor::Yellow;//TargetingComponent->EditorSelectedColor;
			const FLinearColor UnselectedColor = FLinearColor::White;//TargetingComponent->EditorUnselectedColor;

			FLinearColor Color = bIsSelected ? SelectedColor : UnselectedColor;
			const FVector Location = HandComponent->HandVisualizerComponent->GetComponentLocation();
			PDI->SetHitProxy(new HHandSocketVisProxy(Component));
			PDI->DrawPoint(Location, Color, 20.f, SDPG_Foreground);
			PDI->SetHitProxy(NULL);

			HHandSocketVisProxy* newHitProxy = new HHandSocketVisProxy(Component);
			newHitProxy->bIsCore = true;
			PDI->SetHitProxy(newHitProxy);
			PDI->DrawPoint(HandComponent->GetComponentLocation(), FLinearColor::Red, 20.f, SDPG_Foreground);
			PDI->SetHitProxy(NULL);

			TArray<FTransform> BoneTransforms = HandComponent->HandVisualizerComponent->GetBoneSpaceTransforms();
			FTransform ParentTrans = HandComponent->HandVisualizerComponent->GetComponentTransform();
			for (int i=0; i<HandComponent->HandVisualizerComponent->GetNumBones(); i++)
			{
				FName BoneName = HandComponent->HandVisualizerComponent->GetBoneName(i);
				FTransform BoneTransform = HandComponent->HandVisualizerComponent->GetBoneTransform(i);
				FVector BoneLoc = BoneTransform.GetLocation();
				float BoneScale = 1.0f - ((View->ViewLocation - BoneLoc).SizeSquared() / FMath::Square(100.0f));
				BoneScale = FMath::Clamp(BoneScale, 0.1f, 1.0f);
				PDI->SetHitProxy(new HHandSocketVisProxy(Component));
				PDI->DrawPoint(BoneLoc, Color, 20.f * BoneScale, SDPG_Foreground);
				PDI->SetHitProxy(NULL);
			}
			//HandComponent->HandVisualizerComponent->TransformFromBoneSpace()

			//Iterate over each target drawing a line and dot
			/*for (int i = 0; i < TargetingComponent->Targets.Num(); i++)
			{
				FLinearColor Color = (i == SelectedTargetIndex) ? SelectedColor : UnselectedColor;

				//Set our hit proxy
				PDI->SetHitProxy(new HTargetProxy(Component, i));
				PDI->DrawLine(Locaction, TargetingComponent->Targets[i], Color, SDPG_Foreground);
				PDI->DrawPoint(TargetingComponent->Targets[i], Color, 20.f, SDPG_Foreground);
				PDI->SetHitProxy(NULL);
			}*/
		}
	}

    virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override
	{
		bool bEditing = false;
		if (VisProxy && VisProxy->Component.IsValid())
		{
			bEditing = true;
			if (VisProxy->IsA(HHandSocketVisProxy::StaticGetType()))
			{
				HHandSocketVisProxy* Proxy = (HHandSocketVisProxy*)VisProxy;

				bIsSelected = !Proxy->bIsCore;
				//const UHandSocketComponent* targetComp = Cast<const UHandSocketComponent>(VisProxy->Component.Get());
				//UHandSocketComponent * currentHand = const_cast<UHandSocketComponent*>(targetComp);
				//CurrentlyEditingComponent = currentHand;
				//SelectedTargetIndex = VisProxy->TargetIndex;
			}
		}
		else
		{
			bIsSelected = false;
			//SelectedTargetIndex = INDEX_NONE;
		}

		return bEditing;
	}

	bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override
	{
		if (CurrentlyEditingComponent.IsValid() && bIsSelected)
		{
			OutLocation = (CurrentlyEditingComponent->HandRelativePlacement * CurrentlyEditingComponent->GetComponentTransform()).GetLocation();// HandVisualizerComponent->GetComponentLocation();
			return true;
		}

		return false;
	}

	bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override
	{
		bool bHandled = false;

		if (CurrentlyEditingComponent.IsValid() && bIsSelected)
		{
			const FScopedTransaction Transaction(LOCTEXT("ChangingComp", "ChangingComp"));

			CurrentlyEditingComponent->Modify();
			if (AActor* Owner = CurrentlyEditingComponent->GetOwner())
			{
				Owner->Modify();
			}
			bool bLevelEdit = ViewportClient->IsLevelEditorClient();
			//FTransform DeltaTrans(DeltaRotate.Quaternion(), DeltaTranslate, DeltaScale);
			//CurrentlyEditingComponent->HandRelativePlacement = DeltaTrans * CurrentlyEditingComponent->HandRelativePlacement;

			if (!DeltaTranslate.IsNearlyZero())
			{
				CurrentlyEditingComponent->HandRelativePlacement.AddToTranslation(DeltaTranslate);
			}

			if (!DeltaRotate.IsNearlyZero())
			{
				CurrentlyEditingComponent->HandRelativePlacement.ConcatenateRotation(DeltaRotate.Quaternion());
			}

			if (!DeltaScale.IsNearlyZero())
			{
				CurrentlyEditingComponent->HandRelativePlacement.MultiplyScale3D(DeltaScale);
			}

			/*if (CurrentlyEditingComponent->HandVisualizerComponent)
			{
				CurrentlyEditingComponent->HandVisualizerComponent->SetRelativeTransform(CurrentlyEditingComponent->HandRelativePlacement);
			}*/

			//if (AActor* Owner = CurrentlyEditingComponent->GetOwner())
			//{
			//	Owner->PostEditMove(false);
			//}

			GEditor->RedrawLevelEditingViewports(true);
			NotifyPropertyModified(CurrentlyEditingComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, HandRelativePlacement)));
			
			bHandled = true;
		}

		return bHandled;
	}

	virtual void EndEditing() override
	{
		bIsSelected = false;
		CurrentlyEditingComponent = nullptr;
	}
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

#undef LOCTEXT_NAMESPACE
