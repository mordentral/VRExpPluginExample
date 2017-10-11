// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VRRetainerBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

#include "SVRRetainerWidget.h"

#define LOCTEXT_NAMESPACE "UMG"

static FName DefaultTextureParameterName("Texture");

/////////////////////////////////////////////////////
// UVRRetainerBox

UVRRetainerBox::UVRRetainerBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Visibility = ESlateVisibility::Visible;
	Phase = 0;
	PhaseCount = 1;
//	TextureParameter = DefaultTextureParameterName;
}

UTextureRenderTarget2D * UVRRetainerBox::GetRenderTarget()
{
	return MyRetainerWidget->RenderTarget;
}

void UVRRetainerBox::SetRenderTarget(UTextureRenderTarget2D * NewRenderTarget)
{
	MyRetainerWidget->RenderTarget = NewRenderTarget;
}

/*UMaterialInstanceDynamic* UVRRetainerBox::GetEffectMaterial() const
{
	if ( MyRetainerWidget.IsValid() )
	{
		return MyRetainerWidget->GetEffectMaterial();
	}

	return nullptr;
}*/

/*void UVRRetainerBox::SetEffectMaterial(UMaterialInterface* InEffectMaterial)
{
	EffectMaterial = InEffectMaterial;
	if ( MyRetainerWidget.IsValid() )
	{
		MyRetainerWidget->SetEffectMaterial(EffectMaterial);
	}
}*/

/*void UVRRetainerBox::SetTextureParameter(FName InTextureParameter)
{
	TextureParameter = InTextureParameter;
	if ( MyRetainerWidget.IsValid() )
	{
		MyRetainerWidget->SetTextureParameter(TextureParameter);
	}
}*/

void UVRRetainerBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyRetainerWidget.Reset();
}

TSharedRef<SWidget> UVRRetainerBox::RebuildWidget()
{
	MyRetainerWidget =
		SNew(SVRRetainerWidget)
		.Phase(Phase)
		.PhaseCount(PhaseCount)
#if STATS
		.StatId( FName( *FString::Printf(TEXT("%s [%s]"), *GetFName().ToString(), *GetClass()->GetName() ) ) )
#endif//STATS
		;

	if (TargetRenderTarget != nullptr)
		MyRetainerWidget->RenderTarget = TargetRenderTarget;

	MyRetainerWidget->SetRetainedRendering(IsDesignTime() ? false : true);
	
	if ( GetChildrenCount() > 0 )
	{
		MyRetainerWidget->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}

	return MyRetainerWidget.ToSharedRef();
	
	//return BuildDesignTimeWidget(MyRetainerWidget.ToSharedRef());
}

void UVRRetainerBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	//MyRetainerWidget->SetEffectMaterial(EffectMaterial);
	//MyRetainerWidget->SetTextureParameter(TextureParameter);
	MyRetainerWidget->SetWorld(GetWorld());
}

void UVRRetainerBox::OnSlotAdded(UPanelSlot* InSlot)
{
	// Add the child to the live slot if it already exists
	if ( MyRetainerWidget.IsValid() )
	{
		MyRetainerWidget->SetContent(InSlot->Content ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UVRRetainerBox::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyRetainerWidget.IsValid() )
	{
		MyRetainerWidget->SetContent(SNullWidget::NullWidget);
	}
}

#if WITH_EDITOR

const FText UVRRetainerBox::GetPaletteCategory()
{
	return LOCTEXT("Optimization", "Optimization");
}

#endif

const FGeometry& UVRRetainerBox::GetCachedAllottedGeometry() const
{
	if (MyRetainerWidget.IsValid())
	{
		return MyRetainerWidget->GetCachedAllottedGeometry();
	}

	static const FGeometry TempGeo;
	return TempGeo;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
