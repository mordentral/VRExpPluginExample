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

IMPLEMENT_HIT_PROXY(HHandSocketVisProxy, HComponentVisProxy);
#define LOCTEXT_NAMESPACE "HandSocketVisualizer"

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

	bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override
	{
		if (CurrentlyEditingComponent.IsValid() && CurrentlySelectedBone != NAME_None && CurrentlySelectedBone != "HandSocket")
		{
			if (ViewportClient->GetWidgetCoordSystemSpace() == COORD_Local || ViewportClient->GetWidgetMode() == FWidget::WM_Rotate)
			{

				if (CurrentlySelectedBone == "Visualizer")
				{
					FTransform newTrans = CurrentlyEditingComponent->HandRelativePlacement * CurrentlyEditingComponent->GetComponentTransform();
					OutMatrix = FRotationMatrix::Make(newTrans.GetRotation());
				}
				else
				{
					FQuat DeltaQuat = FQuat::Identity;
					for (FBPVRHandPoseBonePair & HandPair : CurrentlyEditingComponent->CustomPoseDeltas)
					{
						if (HandPair.BoneName == CurrentlySelectedBone)
						{
							//DeltaQuat = HandPair.DeltaPose;
						}
					}

					FTransform newTrans = CurrentlyEditingComponent->HandVisualizerComponent->GetBoneTransform(CurrentlySelectedBoneIdx);
					newTrans.ConcatenateRotation(DeltaQuat);
					OutMatrix = FRotationMatrix::Make(newTrans.GetRotation());
				}

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

			const FVector Location = HandComponent->HandVisualizerComponent->GetComponentLocation();
			HHandSocketVisProxy* newHitProxy = new HHandSocketVisProxy(Component);
			newHitProxy->TargetBoneName = "Visualizer";
			PDI->SetHitProxy(newHitProxy);
			PDI->DrawPoint(Location, CurrentlySelectedBone == newHitProxy->TargetBoneName ? SelectedColor : UnselectedColor, 20.f, SDPG_Foreground);
			PDI->SetHitProxy(NULL);
			newHitProxy = nullptr;

			newHitProxy = new HHandSocketVisProxy(Component);
			newHitProxy->TargetBoneName = "HandSocket";
			PDI->SetHitProxy(newHitProxy);
			PDI->DrawPoint(HandComponent->GetComponentLocation(), FLinearColor::Red, 20.f, SDPG_Foreground);
			PDI->SetHitProxy(NULL);
			newHitProxy = nullptr;

			if (HandComponent->bUseCustomPoseDeltas)
			{
				TArray<FTransform> BoneTransforms = HandComponent->HandVisualizerComponent->GetBoneSpaceTransforms();
				FTransform ParentTrans = HandComponent->HandVisualizerComponent->GetComponentTransform();
				// We skip root bone, moving the visualizer itself handles that
				for (int i = 1; i < HandComponent->HandVisualizerComponent->GetNumBones(); i++)
				{
					FName BoneName = HandComponent->HandVisualizerComponent->GetBoneName(i);
					FTransform BoneTransform = HandComponent->HandVisualizerComponent->GetBoneTransform(i);
					FVector BoneLoc = BoneTransform.GetLocation();
					float BoneScale = 1.0f - ((View->ViewLocation - BoneLoc).SizeSquared() / FMath::Square(100.0f));
					BoneScale = FMath::Clamp(BoneScale, 0.1f, 1.0f);
					newHitProxy = new HHandSocketVisProxy(Component);
					newHitProxy->TargetBoneName = BoneName;
					newHitProxy->BoneIdx = i;
					PDI->SetHitProxy(newHitProxy);
					PDI->DrawPoint(BoneLoc, CurrentlySelectedBone == newHitProxy->TargetBoneName ? SelectedColor : UnselectedColor, 20.f * BoneScale, SDPG_Foreground);
					PDI->SetHitProxy(NULL);
					newHitProxy = nullptr;
				}
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
				//if (LastVisProxy.IsValid() && VisProxy != LastVisProxy.Get())
				{

					HHandSocketVisProxy* Proxy = (HHandSocketVisProxy*)VisProxy;
					//CurrentlySelectedBone = Proxy->TargetBoneName;
					CurrentlySelectedBone = Proxy->TargetBoneName;
					CurrentlySelectedBoneIdx = Proxy->BoneIdx;
					//LastVisProxy = Proxy;
				}
				//const UHandSocketComponent* targetComp = Cast<const UHandSocketComponent>(VisProxy->Component.Get());
				//UHandSocketComponent * currentHand = const_cast<UHandSocketComponent*>(targetComp);
				//CurrentlyEditingComponent = currentHand;
				//SelectedTargetIndex = VisProxy->TargetIndex;
			}
		}
		else
		{
			//SelectedTargetIndex = INDEX_NONE;
		}

		return bEditing;
	}

	bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override
	{
		if (CurrentlyEditingComponent.IsValid() && CurrentlySelectedBone != NAME_None && CurrentlySelectedBone != "HandSocket")
		{
			if (CurrentlySelectedBone == "Visualizer")
			{
				OutLocation = (CurrentlyEditingComponent->HandRelativePlacement * CurrentlyEditingComponent->GetComponentTransform()).GetLocation();// HandVisualizerComponent->GetComponentLocation();
			}
			else
			{
				OutLocation = CurrentlyEditingComponent->HandVisualizerComponent->GetBoneTransform(CurrentlySelectedBoneIdx).GetLocation();
			}

			return true;
		}

		return false;
	}

	bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override
	{
		bool bHandled = false;

		if (CurrentlyEditingComponent.IsValid())
		{	
			if (CurrentlySelectedBone == "HandSocket")
			{

			}
			else if (CurrentlySelectedBone == "Visualizer")
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

				NotifyPropertyModified(CurrentlyEditingComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, HandRelativePlacement)));

			}
			else
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

				bool bFoundBone = false;
				for (FBPVRHandPoseBonePair& BonePair : CurrentlyEditingComponent->CustomPoseDeltas)
				{
					if (BonePair.BoneName == CurrentlySelectedBone)
					{
						bFoundBone = true;
						BonePair.DeltaPose *= DeltaRotate.Quaternion();
						break;
					}
				}

				if (!bFoundBone)
				{
					FBPVRHandPoseBonePair newBonePair;
					newBonePair.BoneName = CurrentlySelectedBone;
					newBonePair.DeltaPose *= DeltaRotate.Quaternion();
					CurrentlyEditingComponent->CustomPoseDeltas.Add(newBonePair);
					bFoundBone = true;
				}

				if (bFoundBone)
				{
					NotifyPropertyModified(CurrentlyEditingComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, CustomPoseDeltas)));
				}
			}

			/*if (CurrentlyEditingComponent->HandVisualizerComponent)
			{
				CurrentlyEditingComponent->HandVisualizerComponent->SetRelativeTransform(CurrentlyEditingComponent->HandRelativePlacement);
			}*/

			//if (AActor* Owner = CurrentlyEditingComponent->GetOwner())
			//{
			//	Owner->PostEditMove(false);
			//}

			//GEditor->RedrawLevelEditingViewports(true);

			bHandled = true;
		}

		return bHandled;
	}

	virtual void EndEditing() override
	{
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
