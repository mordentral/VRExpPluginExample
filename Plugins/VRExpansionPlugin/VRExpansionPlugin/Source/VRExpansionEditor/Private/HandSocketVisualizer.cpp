// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HandSocketVisualizer.h"

#include "AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"
//#include "Persona.h"
#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "AnimationUtils.h"

IMPLEMENT_HIT_PROXY(HHandSocketVisProxy, HComponentVisProxy);
#define LOCTEXT_NAMESPACE "HandSocketVisualizer"


FText SCreateHandAnimationDlg::LastUsedAssetPath;

static bool PromptUserForAssetPath(FString& AssetPath, FString& AssetName)
{
	TSharedRef<SCreateHandAnimationDlg> NewAnimDlg = SNew(SCreateHandAnimationDlg);
	if (NewAnimDlg->ShowModal() != EAppReturnType::Cancel)
	{
		AssetPath = NewAnimDlg->GetFullAssetPath();
		AssetName = NewAnimDlg->GetAssetName();
		return true;
	}

	return false;
}

bool FHandSocketVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click)
{
	bool bEditing = false;
	if (VisProxy && VisProxy->Component.IsValid())
	{
		FString AssetPath;
		FString AssetName;
		PromptUserForAssetPath(AssetPath, AssetName);
		SaveAnimationAsset(AssetPath, AssetName);

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
/*
bool FAnimationRecorder::Record(USkeletalMeshComponent* Component, FTransform const& ComponentToWorld, const TArray<FTransform>& SpacesBases, const FBlendedHeapCurve& AnimationCurves, int32 FrameToAdd)
{
	if (ensure(AnimationObject))
	{
		USkeletalMesh* SkeletalMesh = Component->MasterPoseComponent != nullptr ? Component->MasterPoseComponent->SkeletalMesh : Component->SkeletalMesh;

		if (FrameToAdd == 0)
		{
			// Find the root bone & store its transform
			SkeletonRootIndex = INDEX_NONE;
			USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
			for (int32 TrackIndex = 0; TrackIndex < AnimationObject->GetRawAnimationData().Num(); ++TrackIndex)
			{
				// verify if this bone exists in skeleton
				int32 BoneTreeIndex = AnimationObject->GetSkeletonIndexFromRawDataTrackIndex(TrackIndex);
				if (BoneTreeIndex != INDEX_NONE)
				{
					int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, BoneTreeIndex);
					int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
					FTransform LocalTransform = SpacesBases[BoneIndex];
					if (ParentIndex == INDEX_NONE)
					{
						if (bRemoveRootTransform && AnimationObject->GetRawAnimationData().Num() > 1)
						{
							// Store initial root transform.
							// We remove the initial transform of the root bone and transform root's children
							// to remove any offset. We need to do this for sequence recording in particular
							// as we use root motion to build transform tracks that properly sync with
							// animation keyframes. If we have a transformed root bone then the assumptions
							// we make about root motion use are incorrect.
							// NEW. But we don't do this if there is just one root bone. This has come up with recording
							// single bone props and cameras.
							InitialRootTransform = LocalTransform;
							InvInitialRootTransform = LocalTransform.Inverse();
						}
						else
						{
							InitialRootTransform = InvInitialRootTransform = FTransform::Identity;
						}
						SkeletonRootIndex = BoneIndex;
						break;
					}
				}
			}
		}

		FSerializedAnimation  SerializedAnimation;
		USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
		for (int32 TrackIndex = 0; TrackIndex < AnimationObject->GetRawAnimationData().Num(); ++TrackIndex)
		{
			// verify if this bone exists in skeleton
			int32 BoneTreeIndex = AnimationObject->GetSkeletonIndexFromRawDataTrackIndex(TrackIndex);
			if (BoneTreeIndex != INDEX_NONE)
			{
				int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, BoneTreeIndex);
				int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				FTransform LocalTransform = SpacesBases[BoneIndex];
				if ( ParentIndex != INDEX_NONE )
				{
					LocalTransform.SetToRelativeTransform(SpacesBases[ParentIndex]);
				}
				// if record local to world, we'd like to consider component to world to be in root
				else
				{
					if (bRecordLocalToWorld)
					{
						LocalTransform *= ComponentToWorld;
					}
				}

				FRawAnimSequenceTrack& RawTrack = AnimationObject->GetRawAnimationTrack(TrackIndex);
				if (bRecordTransforms)
				{
					RawTrack.PosKeys.Add(LocalTransform.GetTranslation());
					RawTrack.RotKeys.Add(LocalTransform.GetRotation());
					RawTrack.ScaleKeys.Add(LocalTransform.GetScale3D());
					if (AnimationSerializer)
					{
						SerializedAnimation.AddTransform(TrackIndex, LocalTransform);
					}
				}
				// verification
				if (FrameToAdd != RawTrack.PosKeys.Num()-1)
				{
					UE_LOG(LogAnimation, Warning, TEXT("Mismatch in animation frames. Trying to record frame: %d, but only: %d frame(s) exist. Changing skeleton while recording is not supported."), FrameToAdd, RawTrack.PosKeys.Num());
					return false;
				}
			}
		}

		TOptional<FQualifiedFrameTime> CurrentTime = FApp::GetCurrentFrameTime();
		RecordedTimes.Add(CurrentTime.IsSet() ? CurrentTime.GetValue() : FQualifiedFrameTime());

		if (AnimationSerializer)
		{
			AnimationSerializer->WriteFrameData(AnimationSerializer->FramesWritten, SerializedAnimation);
		}
		// each RecordedCurves contains all elements
		if (bRecordCurves && AnimationCurves.CurveWeights.Num() > 0)
		{
			RecordedCurves.Emplace(AnimationCurves.CurveWeights, AnimationCurves.ValidCurveWeights);
			if (UIDToArrayIndexLUT == nullptr)
			{
				UIDToArrayIndexLUT = AnimationCurves.UIDToArrayIndexLUT;
			}
			else
			{
				ensureAlways(UIDToArrayIndexLUT->Num() == AnimationCurves.UIDToArrayIndexLUT->Num());
				if (UIDToArrayIndexLUT != AnimationCurves.UIDToArrayIndexLUT)
				{
					UIDToArrayIndexLUT = AnimationCurves.UIDToArrayIndexLUT;
				}
			}
		}

		LastFrame = FrameToAdd;
	}

	return true;
}

*/
bool FHandSocketVisualizer::SaveAnimationAsset(const FString& InAssetPath, const FString& InAssetName)
{
	// Replace when this moves to custom display
	if (!CurrentlyEditingComponent.IsValid())
		return false;

	if (!CurrentlyEditingComponent->HandVisualizerComponent || !CurrentlyEditingComponent->HandVisualizerComponent->SkeletalMesh || !CurrentlyEditingComponent->HandVisualizerComponent->SkeletalMesh->Skeleton)
	{
		return false;
	}

	// create the asset
	FText InvalidPathReason;
	bool const bValidPackageName = FPackageName::IsValidLongPackageName(InAssetPath, false, &InvalidPathReason);
	if (bValidPackageName == false)
	{
		UE_LOG(LogAnimation, Log, TEXT("%s is an invalid asset path, prompting user for new asset path. Reason: %s"), *InAssetPath, *InvalidPathReason.ToString());
	}

	FString ValidatedAssetPath = InAssetPath;
	FString ValidatedAssetName = InAssetName;

	UObject* Parent = bValidPackageName ? CreatePackage(*ValidatedAssetPath) : nullptr;
	if (Parent == nullptr)
	{
		// bad or no path passed in, do the popup
		if (PromptUserForAssetPath(ValidatedAssetPath, ValidatedAssetName) == false)
		{
			return false;
		}

		Parent = CreatePackage(*ValidatedAssetPath);
	}

	UObject* const Object = LoadObject<UObject>(Parent, *ValidatedAssetName, nullptr, LOAD_Quiet, nullptr);
	// if object with same name exists, warn user
	if (Object)
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "Error_AssetExist", "Asset with same name exists. Can't overwrite another asset"));
		return false;		// failed
	}

	// If not, create new one now.
	UAnimSequence* const NewSeq = NewObject<UAnimSequence>(Parent, *ValidatedAssetName, RF_Public | RF_Standalone);
	if (NewSeq)
	{
		// set skeleton
		NewSeq->SetSkeleton(CurrentlyEditingComponent->HandVisualizerComponent->SkeletalMesh->Skeleton);
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(NewSeq);
		//StartRecord(Component, NewSeq);

		//return true;

		UAnimSequence* AnimationObject = NewSeq;

		//TimePassed = 0.f;

		AnimationObject->RecycleAnimSequence();
		AnimationObject->BoneCompressionSettings = FAnimationUtils::GetDefaultAnimationBoneCompressionSettings();

		//FAnimationRecorder::GetBoneTransforms(Component, PreviousSpacesBases);
		//PreviousAnimCurves = Component->GetAnimationCurves();
		//PreviousComponentToWorld = Component->GetComponentTransform();

		//LastFrame = 0;
		AnimationObject->SequenceLength = 0.f;
		AnimationObject->SetRawNumberOfFrame(0);

		//RecordedCurves.Reset();
		//RecordedTimes.Empty();
		//UIDToArrayIndexLUT = nullptr;

		/*USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
		// add all frames
		for (int32 BoneIndex = 0; BoneIndex < PreviousSpacesBases.Num(); ++BoneIndex)
		{
			// verify if this bone exists in skeleton
			const int32 BoneTreeIndex = AnimSkeleton->GetSkeletonBoneIndexFromMeshBoneIndex(Component->MasterPoseComponent != nullptr ? Component->MasterPoseComponent->SkeletalMesh : Component->SkeletalMesh, BoneIndex);
			if (BoneTreeIndex != INDEX_NONE)
			{
				// add tracks for the bone existing
				FName BoneTreeName = AnimSkeleton->GetReferenceSkeleton().GetBoneName(BoneTreeIndex);
				AnimationObject->AddNewRawTrack(BoneTreeName);
			}
		}*/
		USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
		for (FBPVRHandPoseBonePair& BonePair : CurrentlyEditingComponent->CustomPoseDeltas)
		{
			AnimationObject->AddNewRawTrack(BonePair.BoneName);
		}
		
		
		AnimationObject->RetargetSource = CurrentlyEditingComponent->HandVisualizerComponent->SkeletalMesh ? AnimSkeleton->GetRetargetSourceForMesh(CurrentlyEditingComponent->HandVisualizerComponent->SkeletalMesh) : NAME_None;


		/// SAVE POSE
		/// 
					// Find the root bone & store its transform
		/*SkeletonRootIndex = INDEX_NONE;
		USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
		for (int32 TrackIndex = 0; TrackIndex < AnimationObject->GetRawAnimationData().Num(); ++TrackIndex)
		{
			// verify if this bone exists in skeleton
			int32 BoneTreeIndex = AnimationObject->GetSkeletonIndexFromRawDataTrackIndex(TrackIndex);
			if (BoneTreeIndex != INDEX_NONE)
			{
				int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, BoneTreeIndex);
				int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				FTransform LocalTransform = SpacesBases[BoneIndex];
				if (ParentIndex == INDEX_NONE)
				{
					if (bRemoveRootTransform && AnimationObject->GetRawAnimationData().Num() > 1)
					{
						// Store initial root transform.
						// We remove the initial transform of the root bone and transform root's children
						// to remove any offset. We need to do this for sequence recording in particular
						// as we use root motion to build transform tracks that properly sync with
						// animation keyframes. If we have a transformed root bone then the assumptions
						// we make about root motion use are incorrect.
						// NEW. But we don't do this if there is just one root bone. This has come up with recording
						// single bone props and cameras.
						InitialRootTransform = LocalTransform;
						InvInitialRootTransform = LocalTransform.Inverse();
					}
					else
					{
						InitialRootTransform = InvInitialRootTransform = FTransform::Identity;
					}
					SkeletonRootIndex = BoneIndex;
					break;
				}
			}
		}*/
		
		
		/// END SAVE POSE 
		/// 
		/// 
		/// 



		// init notifies
		AnimationObject->InitializeNotifyTrack();

		AnimationObject->PostProcessSequence();

		AnimationObject->MarkPackageDirty();

		//if (bAutoSaveAsset)
		{
			UPackage* const Package = AnimationObject->GetOutermost();
			FString const PackageName = Package->GetName();
			FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

			double StartTime = FPlatformTime::Seconds();

			UPackage::SavePackage(Package, NULL, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);

			double ElapsedTime = FPlatformTime::Seconds() - StartTime;
			UE_LOG(LogAnimation, Log, TEXT("Animation Recorder saved %s in %0.2f seconds"), *PackageName, ElapsedTime);
		}

		return true;
	}

	return false;
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


void SCreateHandAnimationDlg::Construct(const FArguments& InArgs)
{
	AssetPath = FText::FromString(FPackageName::GetLongPackagePath(InArgs._DefaultAssetPath.ToString()));
	AssetName = FText::FromString(FPackageName::GetLongPackageAssetName(InArgs._DefaultAssetPath.ToString()));

	if (AssetPath.IsEmpty())
	{
		AssetPath = LastUsedAssetPath;
		// still empty?
		if (AssetPath.IsEmpty())
		{
			AssetPath = FText::FromString(TEXT("/Game"));
		}
	}
	else
	{
		LastUsedAssetPath = AssetPath;
	}

	if (AssetName.IsEmpty())
	{
		// find default name for them
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString OutPackageName, OutAssetName;
		FString PackageName = AssetPath.ToString() + TEXT("/NewAnimation");

		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), OutPackageName, OutAssetName);
		AssetName = FText::FromString(OutAssetName);
	}

	FPathPickerConfig PathPickerConfig;
	PathPickerConfig.DefaultPath = AssetPath.ToString();
	PathPickerConfig.OnPathSelected = FOnPathSelected::CreateSP(this, &SCreateHandAnimationDlg::OnPathChange);
	PathPickerConfig.bAddDefaultPath = true;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	SWindow::Construct(SWindow::FArguments()
		.Title(LOCTEXT("SCreateHandAnimationDlg_Title", "Create New Animation Object"))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		//.SizingRule( ESizingRule::Autosized )
		.ClientSize(FVector2D(450, 450))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot() // Add user input block
		.Padding(2)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("SelectPath", "Select Path to create animation"))
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
		]

	+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(3)
		[
			ContentBrowserModule.Get().CreatePathPicker(PathPickerConfig)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(3)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 10, 0)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AnimationName", "Animation Name"))
		]

	+ SHorizontalBox::Slot()
		[
			SNew(SEditableTextBox)
			.Text(AssetName)
		.OnTextCommitted(this, &SCreateHandAnimationDlg::OnNameChange)
		.MinDesiredWidth(250)
		]
		]
		]
		]

	+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(5)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FEditorStyle::GetMargin("StandardDialog.SlotPadding"))
		.MinDesiredSlotWidth(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
		.MinDesiredSlotHeight(FEditorStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.Text(LOCTEXT("OK", "OK"))
		.OnClicked(this, &SCreateHandAnimationDlg::OnButtonClick, EAppReturnType::Ok)
		]
	+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
		.ContentPadding(FEditorStyle::GetMargin("StandardDialog.ContentPadding"))
		.Text(LOCTEXT("Cancel", "Cancel"))
		.OnClicked(this, &SCreateHandAnimationDlg::OnButtonClick, EAppReturnType::Cancel)
		]
		]
		]);
}

void SCreateHandAnimationDlg::OnNameChange(const FText& NewName, ETextCommit::Type CommitInfo)
{
	AssetName = NewName;
}

void SCreateHandAnimationDlg::OnPathChange(const FString& NewPath)
{
	AssetPath = FText::FromString(NewPath);
	LastUsedAssetPath = AssetPath;
}

FReply SCreateHandAnimationDlg::OnButtonClick(EAppReturnType::Type ButtonID)
{
	UserResponse = ButtonID;

	if (ButtonID != EAppReturnType::Cancel)
	{
		if (!ValidatePackage())
		{
			// reject the request
			return FReply::Handled();
		}
	}

	RequestDestroyWindow();

	return FReply::Handled();
}

/** Ensures supplied package name information is valid */
bool SCreateHandAnimationDlg::ValidatePackage()
{
	FText Reason;
	FString FullPath = GetFullAssetPath();

	if (!FPackageName::IsValidLongPackageName(FullPath, false, &Reason)
		|| !FName(*AssetName.ToString()).IsValidObjectName(Reason))
	{
		FMessageDialog::Open(EAppMsgType::Ok, Reason);
		return false;
	}

	return true;
}

EAppReturnType::Type SCreateHandAnimationDlg::ShowModal()
{
	GEditor->EditorAddModalWindow(SharedThis(this));
	return UserResponse;
}

FString SCreateHandAnimationDlg::GetAssetPath()
{
	return AssetPath.ToString();
}

FString SCreateHandAnimationDlg::GetAssetName()
{
	return AssetName.ToString();
}

FString SCreateHandAnimationDlg::GetFullAssetPath()
{
	return AssetPath.ToString() + "/" + AssetName.ToString();
}

#undef LOCTEXT_NAMESPACE