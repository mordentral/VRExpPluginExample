// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Layout/Visibility.h"
#include "Layout/SlateRect.h"
#include "Layout/Geometry.h"
#include "Input/Events.h"
#include "Layout/ArrangedWidget.h"
#include "Widgets/SWidget.h"
#include "Styling/SlateBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Layout/Children.h"
#include "UObject/GCObject.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/HittestGrid.h"
#include "Slate/WidgetRenderer.h"

class FArrangedChildren;
class UMaterialInstanceDynamic;
class UMaterialInterface;

//DECLARE_MULTICAST_DELEGATE(FOnRetainedModeChanged);

/**
* The SVRRetainerWidget renders children widgets to a render target first before
* later rendering that render target to the screen.  This allows both frequency
* and phase to be controlled so that the UI can actually render less often than the
* frequency of the main game render.  It also has the side benefit of allow materials
* to be applied to the render target after drawing the widgets to apply a simple post process.
*/
class VREXPANSIONPLUGIN_API SVRRetainerWidget : public SCompoundWidget, public FGCObject, public ICustomHitTestPath
{
public:
	SLATE_BEGIN_ARGS(SVRRetainerWidget)
	{
		_Visibility = EVisibility::Visible;
		_Phase = 0;
		_PhaseCount = 1;
	}
	SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(int32, Phase)
		SLATE_ARGUMENT(int32, PhaseCount)
		SLATE_ARGUMENT(FName, StatId)
		SLATE_END_ARGS()

		SVRRetainerWidget();
	~SVRRetainerWidget();

	void Construct(const FArguments& Args);

	void SetRetainedRendering(bool bRetainRendering);

	void SetContent(const TSharedRef< SWidget >& InContent);

	UMaterialInstanceDynamic* GetEffectMaterial() const;

	void SetEffectMaterial(UMaterialInterface* EffectMaterial);

	//void SetTextureParameter(FName TextureParameter);

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// FGCObject

	// SWidget
	virtual FChildren* GetChildren() override;
	virtual bool ComputeVolatility() const override;
	// SWidget

	virtual void PaintRetainedContent(float DeltaTime);

	// ICustomHitTestPath
	virtual TArray<FWidgetAndPointer> GetBubblePathAndVirtualCursors(const FGeometry& InGeometry, FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus) const override;
	virtual void ArrangeChildren(FArrangedChildren& ArrangedChildren) const override;
	virtual TSharedPtr<struct FVirtualPointerPosition> TranslateMouseCoordinateFor3DChild(const TSharedRef<SWidget>& ChildWidget, const FGeometry& ViewportGeometry, const FVector2D& ScreenSpaceMouseCoordinate, const FVector2D& LastScreenSpaceMouseCoordinate) const override;
	// ICustomHitTestPath

	FORCEINLINE const FGeometry& GetCachedAllottedGeometry() const
	{
		return CachedAllottedGeometry;
	}

	void SetWorld(UWorld* World);

	mutable class UTextureRenderTarget2D* RenderTarget;

protected:
	// BEGIN SLeafWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float Scale) const override;
	// END SLeafWidget interface

	void RefreshRenderingMode();
	bool ShouldBeRenderingOffscreen() const;
	bool IsAnythingVisibleToRender() const;
	//void OnRetainerModeChanged();
private:
#if !UE_BUILD_SHIPPING
	//static void OnRetainerModeCVarChanged(IConsoleVariable* CVar);
	//static FOnRetainedModeChanged OnRetainerModeChangedDelegate;
#endif

	mutable FGeometry CachedAllottedGeometry;
	mutable FVector2D CachedWindowToDesktopTransform;

	FSimpleSlot EmptyChildSlot;

	mutable FSlateBrush SurfaceBrush;

	void InitWidgetRenderer();
	mutable TSharedPtr<class FWidgetRenderer> WidgetRenderer;
	mutable TSharedPtr<SWidget> MyWidget;

	bool bRenderingOffscreenDesire;
	bool bRenderingOffscreen;

	int32 Phase;
	int32 PhaseCount;

	double LastDrawTime;
	int64 LastTickedFrame;

	TSharedPtr<SVirtualWindow> Window;
	TSharedPtr<FHittestGrid> HitTestGrid;
	TWeakObjectPtr<UWorld> OuterWorld;

	STAT(TStatId MyStatId;)

	//	FSlateBrush DynaicBrush;
	//UMaterialInstanceDynamic* DynamicEffect;
	//FName DynamicEffectTextureParameter;
};
