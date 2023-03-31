// Copyright Epic Games, Inc. All Rights Reserved.

#include "VRGlobalSettingsDetails.h"
#include "VRGlobalSettings.h"
//#include "PropertyEditing.h"
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
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Styling/CoreStyle.h"

#include "Animation/AnimData/AnimDataModel.h"

#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"

#define LOCTEXT_NAMESPACE "VRGlobalSettingsDetails"


TSharedRef< IDetailCustomization > FVRGlobalSettingsDetails::MakeInstance()
{
    return MakeShareable(new FVRGlobalSettingsDetails);
}

void FVRGlobalSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	//DetailBuilder.HideCategory(FName("ComponentTick"));
	//DetailBuilder.HideCategory(FName("GameplayTags"));


	//TSharedPtr<IPropertyHandle> LockedLocationProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UHandSocketComponent, bDecoupleMeshPlacement));

	//FSimpleDelegate OnLockedStateChangedDelegate = FSimpleDelegate::CreateSP(this, &FVRGlobalSettingsDetails::OnLockedStateUpdated, &DetailBuilder);
	//LockedLocationProperty->SetOnPropertyValueChanged(OnLockedStateChangedDelegate);

	DetailBuilder.EditCategory("Utilities")
		.AddCustomRow(NSLOCTEXT("VRGlobalSettingsDetails", "Tools", "Fix Invalid 5.2 Animation Assets"))
		.NameContent()
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(NSLOCTEXT("VRGlobalSettingsDetails", "Tools", "Fix Invalid 5.2 Animation Assets"))
		]
	.ValueContent()
		.MaxDesiredWidth(125.f)
		.MinDesiredWidth(125.f)
		[
			SNew(SButton)
			.ContentPadding(2)
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		.OnClicked(this, &FVRGlobalSettingsDetails::OnCorrectInvalidAnimationAssets)
		[
			SNew(STextBlock)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		.Text(NSLOCTEXT("VRGlobalSettingsDetails", "Tools", "Fix Animation Assets"))
		]
		];
}

FReply FVRGlobalSettingsDetails::OnCorrectInvalidAnimationAssets()
{

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE
