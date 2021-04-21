// Copyright Epic Games, Inc. All Rights Reserved.

#include "HandSocketComponentDetails.h"
#include "HandSocketVisualizer.h"
#include "PropertyEditing.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"

#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "AnimationUtils.h"
#include "AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "HandSocketComponentDetails"

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

TWeakObjectPtr<UAnimSequence> FHandSocketComponentDetails::SaveAnimationAsset(const FString& InAssetPath, const FString& InAssetName)
{

	TWeakObjectPtr<UAnimSequence> FinalAnimation;

	// Replace when this moves to custom display
	if (!HandSocketComponent.IsValid())
		return false;

	/*if (!HandSocketComponent->HandVisualizerComponent)// || !HandSocketComponent->HandVisualizerComponent->SkeletalMesh || !HandSocketComponent->HandVisualizerComponent->SkeletalMesh->Skeleton)
	{
		return false;
	}*/

	if (!HandSocketComponent->HandTargetAnimation)
	{
		return FinalAnimation;
	}

	UAnimSequence* BaseAnimation = HandSocketComponent->HandTargetAnimation;

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
			return FinalAnimation;
		}

		Parent = CreatePackage(*ValidatedAssetPath);
	}

	UObject* const Object = LoadObject<UObject>(Parent, *ValidatedAssetName, nullptr, LOAD_Quiet, nullptr);
	// if object with same name exists, warn user
	if (Object)
	{
		EAppReturnType::Type ReturnValue = FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "Error_AssetExist", "Asset with same name exists. Do you wish to overwrite it?"));
		if (ReturnValue == EAppReturnType::No)
		{
			return FinalAnimation; // failed
		}
	}

	// If not, create new one now.
	UAnimSequence* const NewSeq = NewObject<UAnimSequence>(Parent, *ValidatedAssetName, RF_Public | RF_Standalone);
	if (NewSeq)
	{
		// set skeleton
		NewSeq->SetSkeleton(BaseAnimation->GetSkeleton());
		//NewSeq->SetSkeleton(HandSocketComponent->HandVisualizerComponent->SkeletalMesh->Skeleton);
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(NewSeq);
		//StartRecord(Component, NewSeq);

		//return true;
		UAnimSequence* AnimationObject = NewSeq;

		AnimationObject->RecycleAnimSequence();
		AnimationObject->BoneCompressionSettings = BaseAnimation->BoneCompressionSettings;//FAnimationUtils::GetDefaultAnimationBoneCompressionSettings();

		AnimationObject->SequenceLength = 4.f;
		AnimationObject->SetRawNumberOfFrame(1);

		for (FName TrackName : BaseAnimation->GetAnimationTrackNames())
		{
			AnimationObject->AddNewRawTrack(TrackName);
		}

		/*USkeleton* AnimSkeleton = AnimationObject->GetSkeleton();
		for (int i = 0; i < HandSocketComponent->HandVisualizerComponent->GetNumBones(); i++)
		{
			AnimationObject->AddNewRawTrack(HandSocketComponent->HandVisualizerComponent->GetBoneName(i));
		}*/



		AnimationObject->RetargetSource = BaseAnimation->RetargetSource;//HandSocketComponent->HandVisualizerComponent->SkeletalMesh ? AnimSkeleton->GetRetargetSourceForMesh(HandSocketComponent->HandVisualizerComponent->SkeletalMesh) : NAME_None;


		/// SAVE POSE
		for (int32 TrackIndex = 0; TrackIndex < BaseAnimation->GetRawAnimationData().Num(); ++TrackIndex)
		{
			FRawAnimSequenceTrack& RawTrack = BaseAnimation->GetRawAnimationTrack(TrackIndex);

			bool bHadLoc = false;
			bool bHadRot = false;
			bool bHadScale = false;
			FVector Loc = FVector::ZeroVector;
			FQuat Rot = FQuat::Identity;
			FVector Scale = FVector::ZeroVector;

			if (RawTrack.PosKeys.Num())
			{
				Loc = RawTrack.PosKeys[0];
				bHadLoc = true;
			}

			if (RawTrack.RotKeys.Num())
			{
				Rot = RawTrack.RotKeys[0];
				bHadRot = true;
			}

			if (RawTrack.ScaleKeys.Num())
			{
				Scale = RawTrack.ScaleKeys[0];
				bHadScale = true;
			}

			FTransform FinalTrans(Rot, Loc, Scale);

			FName TrackName = (BaseAnimation->GetAnimationTrackNames())[TrackIndex];

			FQuat DeltaQuat = FQuat::Identity;
			for (FBPVRHandPoseBonePair& HandPair : HandSocketComponent->CustomPoseDeltas)
			{
				if (HandPair.BoneName == TrackName)
				{
					DeltaQuat = HandPair.DeltaPose;
					bHadRot = true;
					break;
				}
			}

			FinalTrans.ConcatenateRotation(DeltaQuat);
			FinalTrans.NormalizeRotation();

			FRawAnimSequenceTrack& RawNewTrack = AnimationObject->GetRawAnimationTrack(TrackIndex);
			if(bHadLoc)
				RawNewTrack.PosKeys.Add(FinalTrans.GetTranslation());
			if(bHadRot)
				RawNewTrack.RotKeys.Add(FinalTrans.GetRotation());
			if(bHadScale)
				RawNewTrack.ScaleKeys.Add(FinalTrans.GetScale3D());
		}

		/*USkeletalMesh* SkeletalMesh = HandSocketComponent->HandVisualizerComponent->SkeletalMesh;
		for (int32 TrackIndex = 0; TrackIndex < AnimationObject->GetRawAnimationData().Num(); ++TrackIndex)
		{
			// verify if this bone exists in skeleton
			int32 BoneTreeIndex = AnimationObject->GetSkeletonIndexFromRawDataTrackIndex(TrackIndex);
			if (BoneTreeIndex != INDEX_NONE)
			{
				int32 BoneIndex = AnimSkeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkeletalMesh, BoneTreeIndex);
				int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
				FTransform LocalTransform = FTransform::Identity;
				LocalTransform.AddToTranslation(FVector(10.f, 10.f, 0.f));

				FName BoneName = HandSocketComponent->HandVisualizerComponent->GetBoneName(BoneIndex);

				FName ParentBone = HandSocketComponent->HandVisualizerComponent->GetParentBone(BoneName);
				FTransform ParentTrans = FTransform::Identity;
				if (ParentBone != NAME_None)
				{
					ParentTrans = HandSocketComponent->HandVisualizerComponent->GetBoneTransformByName(ParentBone, EBoneSpaces::ComponentSpace);
				}

				LocalTransform = HandSocketComponent->HandVisualizerComponent->GetBoneTransformByName(BoneName, EBoneSpaces::ComponentSpace);

				if (ParentBone != NAME_None)
				{
					LocalTransform.SetToRelativeTransform(ParentTrans);
				}

				FRawAnimSequenceTrack& RawTrack = AnimationObject->GetRawAnimationTrack(TrackIndex);
				//if (bRecordTransforms)
				{
					RawTrack.PosKeys.Add(LocalTransform.GetTranslation());
					RawTrack.RotKeys.Add(LocalTransform.GetRotation());
					RawTrack.ScaleKeys.Add(LocalTransform.GetScale3D());
				}
			}
		}*/

		//TOptional<FQualifiedFrameTime> CurrentTime = FApp::GetCurrentFrameTime();
		//RecordedTimes.Add(CurrentTime.IsSet() ? CurrentTime.GetValue() : FQualifiedFrameTime());


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

		FinalAnimation = AnimationObject;
		return FinalAnimation;
	}

	return FinalAnimation;
}

TSharedRef< IDetailCustomization > FHandSocketComponentDetails::MakeInstance()
{
    return MakeShareable(new FHandSocketComponentDetails);
}

void FHandSocketComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetSelectedObjects();
	for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			UHandSocketComponent* CurrentHandSocket = Cast<UHandSocketComponent>(CurrentObject.Get());
			if (CurrentHandSocket != NULL)
			{
				HandSocketComponent = CurrentHandSocket;
				break;
			}
		}
	}

	DetailBuilder.EditCategory("Hand Animation")
		.AddCustomRow(NSLOCTEXT("HandSocketDetails", "UpdateHandSocket", "Save Current Pose"))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(NSLOCTEXT("HandSocketDetails", "UpdateHandSocket", "Save Current Pose"))
		]
	.ValueContent()
		.MaxDesiredWidth(125.f)
		.MinDesiredWidth(125.f)
		[
			SNew(SButton)
			.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(this, &FHandSocketComponentDetails::OnUpdateSavePose)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(NSLOCTEXT("HandSocketDetails", "UpdateHandSocket", "Save"))
		]
		];
}

FReply FHandSocketComponentDetails::OnUpdateSavePose()
{
	if (HandSocketComponent.IsValid() && HandSocketComponent->CustomPoseDeltas.Num() > 0)
	{
		if (HandSocketComponent->HandTargetAnimation)
		{
			// Save Animation Pose here
			FString AssetPath;
			FString AssetName;
			PromptUserForAssetPath(AssetPath, AssetName);
			TWeakObjectPtr<UAnimSequence> NewAnim = SaveAnimationAsset(AssetPath, AssetName);

			// Finally remove the deltas
			if (NewAnim.IsValid())
			{
				HandSocketComponent->HandTargetAnimation = NewAnim.Get();
				HandSocketComponent->CustomPoseDeltas.Empty();
				HandSocketComponent->bUseCustomPoseDeltas = false;
				FComponentVisualizer::NotifyPropertyModified(HandSocketComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, HandTargetAnimation)));
				FComponentVisualizer::NotifyPropertyModified(HandSocketComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, bUseCustomPoseDeltas)));
				FComponentVisualizer::NotifyPropertyModified(HandSocketComponent.Get(), FindFProperty<FProperty>(UHandSocketComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UHandSocketComponent, CustomPoseDeltas)));
			}
		}
	}

	return FReply::Handled();
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
