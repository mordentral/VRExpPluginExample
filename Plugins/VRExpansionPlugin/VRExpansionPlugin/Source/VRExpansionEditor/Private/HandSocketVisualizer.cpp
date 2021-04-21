// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HandSocketVisualizer.h"

#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
//#include "Persona.h"

IMPLEMENT_HIT_PROXY(HHandSocketVisProxy, HComponentVisProxy);
#define LOCTEXT_NAMESPACE "HandSocketVisualizer"

bool FHandSocketVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
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


bool FHandSocketVisualizer::GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const
{
	if (CurrentlyEditingComponent.IsValid() && CurrentlySelectedBone != NAME_None && CurrentlySelectedBone != "HandSocket")
	{
		//if (ViewportClient->GetWidgetCoordSystemSpace() == COORD_Local || ViewportClient->GetWidgetMode() == FWidget::WM_Rotate)
		//{

		if (CurrentlySelectedBone == "Visualizer")
		{
			FTransform newTrans = CurrentlyEditingComponent->HandRelativePlacement * CurrentlyEditingComponent->GetComponentTransform();
			OutMatrix = FRotationMatrix::Make(newTrans.GetRotation());
		}
		else
		{
			/*FQuat DeltaQuat = FQuat::Identity;
			for (FBPVRHandPoseBonePair & HandPair : CurrentlyEditingComponent->CustomPoseDeltas)
			{
				if (HandPair.BoneName == CurrentlySelectedBone)
				{
					//DeltaQuat = HandPair.DeltaPose;
				}
			}*/

			FTransform newTrans = CurrentlyEditingComponent->HandVisualizerComponent->GetBoneTransform(CurrentlySelectedBoneIdx);
			//newTrans.ConcatenateRotation(DeltaQuat);
			OutMatrix = FRotationMatrix::Make(newTrans.GetRotation());
		}

		return true;
		//}
	}

	return false;
}

bool FHandSocketVisualizer::IsVisualizingArchetype() const
{
	return (CurrentlyEditingComponent.IsValid() && CurrentlyEditingComponent->GetOwner() && FActorEditorUtils::IsAPreviewOrInactiveActor(CurrentlyEditingComponent->GetOwner()));
}

// Begin FComponentVisualizer interface
// virtual void OnRegister() override;
void FHandSocketVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
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
	}
}

bool FHandSocketVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
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

bool FHandSocketVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
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

			FTransform CompTransform = CurrentlyEditingComponent->GetComponentTransform();
			FQuat DeltaRotateMod = CompTransform.GetRotation().Inverse() * DeltaRotate.Quaternion();

			bool bFoundBone = false;
			for (FBPVRHandPoseBonePair& BonePair : CurrentlyEditingComponent->CustomPoseDeltas)
			{
				if (BonePair.BoneName == CurrentlySelectedBone)
				{
					bFoundBone = true;
					BonePair.DeltaPose *= DeltaRotateMod;
					break;
				}
			}

			if (!bFoundBone)
			{
				FBPVRHandPoseBonePair newBonePair;
				newBonePair.BoneName = CurrentlySelectedBone;
				newBonePair.DeltaPose *= DeltaRotateMod;
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

void FHandSocketVisualizer::EndEditing()
{
	CurrentlyEditingComponent = nullptr;
}

#undef LOCTEXT_NAMESPACE