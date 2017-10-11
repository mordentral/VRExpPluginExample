// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SVRRetainerWidget.h"
#include "Misc/App.h"
#include "UObject/Package.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"


//DECLARE_CYCLE_STAT(TEXT("Retainer Widget Tick"), STAT_SlateRetainerWidgetTick, STATGROUP_Slate);
//DECLARE_CYCLE_STAT(TEXT("Retainer Widget Paint"), STAT_SlateRetainerWidgetPaint, STATGROUP_Slate);

#if !UE_BUILD_SHIPPING

/** True if we should allow widgets to be cached in the UI at all. */
/*TAutoConsoleVariable<int32> EnableRetainedRendering(
	TEXT("Slate.EnableRetainedRendering"),
	true,
	TEXT("Whether to attempt to render things in SVRRetainerWidgets to render targets first."));
	*/
static bool IsRetainedRenderingEnabled()
{
	return true;// EnableRetainedRendering.GetValueOnGameThread() == 1;
}

//FOnRetainedModeChanged SVRRetainerWidget::OnRetainerModeChangedDelegate;
#else

static bool IsRetainedRenderingEnabled()
{
	return true;
}

#endif


//---------------------------------------------------

SVRRetainerWidget::SVRRetainerWidget()
	: CachedWindowToDesktopTransform(0, 0)
	//, DynamicEffect(nullptr)
{
}

SVRRetainerWidget::~SVRRetainerWidget()
{
	if (FSlateApplication::IsInitialized())
	{
#if !UE_BUILD_SHIPPING
		//OnRetainerModeChangedDelegate.RemoveAll(this);
#endif
	}
}

void SVRRetainerWidget::InitWidgetRenderer()
{
	// Use slate's gamma correction for dynamic materials on mobile, as HW sRGB writes are not supported.
	bool bUseGammaCorrection = /*(DynamicEffect != nullptr) ? GMaxRHIFeatureLevel <= ERHIFeatureLevel::ES3_1 : */true;

	if (!WidgetRenderer.IsValid() || WidgetRenderer->GetUseGammaCorrection() != bUseGammaCorrection)
	{
		WidgetRenderer = MakeShareable(new FWidgetRenderer(bUseGammaCorrection));
		WidgetRenderer->SetIsPrepassNeeded(false);
	}
}

void SVRRetainerWidget::Construct(const FArguments& InArgs)
{
	//STAT(MyStatId = FDynamicStats::CreateStatId<FStatGroup_STATGROUP_Slate>(InArgs._StatId);)

		RenderTarget = NewObject<UTextureRenderTarget2D>();

	// Use HW sRGB correction for RT, ensuring it is linearised on read by material shaders
	// since SVRRetainerWidget::OnPaint renders the RT with gamma correction.
	RenderTarget->SRGB = false;
	RenderTarget->TargetGamma = 1.0f;
	RenderTarget->ClearColor = FLinearColor::Transparent;

	SurfaceBrush.SetResourceObject(RenderTarget);

	Window = SNew(SVirtualWindow);
	Window->SetShouldResolveDeferred(false);
	HitTestGrid = MakeShareable(new FHittestGrid());
	InitWidgetRenderer();

	MyWidget = InArgs._Content.Widget;
	Phase = InArgs._Phase;
	PhaseCount = InArgs._PhaseCount;

	LastDrawTime = FApp::GetCurrentTime();
	LastTickedFrame = 0;

	bRenderingOffscreenDesire = true;
	bRenderingOffscreen = false;

	Window->SetContent(MyWidget.ToSharedRef());

	ChildSlot
		[
			Window.ToSharedRef()
		];

	if (FSlateApplication::IsInitialized())
	{
#if !UE_BUILD_SHIPPING
		//OnRetainerModeChangedDelegate.AddRaw(this, &SVRRetainerWidget::OnRetainerModeChanged);

		static bool bStaticInit = false;

		if (!bStaticInit)
		{
			bStaticInit = true;
			//EnableRetainedRendering.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&SVRRetainerWidget::OnRetainerModeCVarChanged));
		}
#endif
	}
}

bool SVRRetainerWidget::ShouldBeRenderingOffscreen() const
{
	return bRenderingOffscreenDesire && IsRetainedRenderingEnabled();
}

bool SVRRetainerWidget::IsAnythingVisibleToRender() const
{
	return GetVisibility().IsVisible() && MyWidget.IsValid() && MyWidget->GetVisibility().IsVisible();
}

/*void SVRRetainerWidget::OnRetainerModeChanged()
{
	RefreshRenderingMode();
	Invalidate(EInvalidateWidget::Layout);
}*/

#if !UE_BUILD_SHIPPING

/*void SVRRetainerWidget::OnRetainerModeCVarChanged(IConsoleVariable* CVar)
{
	OnRetainerModeChangedDelegate.Broadcast();
}*/

#endif

void SVRRetainerWidget::SetRetainedRendering(bool bRetainRendering)
{
	bRenderingOffscreenDesire = bRetainRendering;
}

void SVRRetainerWidget::RefreshRenderingMode()
{
	const bool bShouldBeRenderingOffscreen = ShouldBeRenderingOffscreen();

	if (bRenderingOffscreen != bShouldBeRenderingOffscreen)
	{
		bRenderingOffscreen = bShouldBeRenderingOffscreen;

		Window->SetContent(MyWidget.ToSharedRef());
	}
}

void SVRRetainerWidget::SetContent(const TSharedRef< SWidget >& InContent)
{
	MyWidget = InContent;
	Window->SetContent(InContent);
}

/*UMaterialInstanceDynamic* SVRRetainerWidget::GetEffectMaterial() const
{
	return DynamicEffect;
}*/

/*void SVRRetainerWidget::SetEffectMaterial(UMaterialInterface* EffectMaterial)
{
	if (EffectMaterial)
	{
		DynamicEffect = Cast<UMaterialInstanceDynamic>(EffectMaterial);
		if (!DynamicEffect)
		{
			DynamicEffect = UMaterialInstanceDynamic::Create(EffectMaterial, GetTransientPackage());
		}

		SurfaceBrush.SetResourceObject(DynamicEffect);
	}
	else
	{
		DynamicEffect = nullptr;
		SurfaceBrush.SetResourceObject(RenderTarget);
	}
	InitWidgetRenderer();
}*/

/*void SVRRetainerWidget::SetTextureParameter(FName TextureParameter)
{
	DynamicEffectTextureParameter = TextureParameter;
}*/

void SVRRetainerWidget::SetWorld(UWorld* World)
{
	OuterWorld = World;
}

void SVRRetainerWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(RenderTarget);
	//Collector.AddReferencedObject(DynamicEffect);
}

FChildren* SVRRetainerWidget::GetChildren()
{
	if (bRenderingOffscreen)
	{
		return &EmptyChildSlot;
	}
	else
	{
		return SCompoundWidget::GetChildren();
	}
}

bool SVRRetainerWidget::ComputeVolatility() const
{
	return true;
}

void SVRRetainerWidget::PaintRetainedContent(float DeltaTime)
{
	//STAT(FScopeCycleCounter TickCycleCounter(MyStatId);)

		// we should not be added to tick if we're not rendering
		checkSlow(FApp::CanEverRender());

	const bool bShouldRenderAnything = IsAnythingVisibleToRender();
	if (bRenderingOffscreen && bShouldRenderAnything)
	{

		// In order to get material parameter collections to function properly, we need the current world's Scene
		// properly propagated through to any widgets that depend on that functionality. The SceneViewport and RetainerWidget the 
		// only location where this information exists in Slate, so we push the current scene onto the current
		// Slate application so that we can leverage it in later calls.
		UWorld* TickWorld = OuterWorld.Get();
		if (TickWorld && TickWorld->Scene && IsInGameThread())
		{
			FSlateApplication::Get().GetRenderer()->RegisterCurrentScene(TickWorld->Scene);
		}
		else if (IsInGameThread())
		{
			FSlateApplication::Get().GetRenderer()->RegisterCurrentScene(nullptr);
		}

		//SCOPE_CYCLE_COUNTER(STAT_SlateRetainerWidgetTick);
		if (LastTickedFrame != GFrameCounter && (GFrameCounter % PhaseCount) == Phase)
		{
			LastTickedFrame = GFrameCounter;
			const double TimeSinceLastDraw = FApp::GetCurrentTime() - LastDrawTime;

			//const FSlateRenderTransform& RenderTransform = CachedAllottedGeometry.GetAccumulatedRenderTransform();

			FPaintGeometry PaintGeometry = CachedAllottedGeometry.ToPaintGeometry();
			FVector2D RenderSize = PaintGeometry.GetLocalSize() * PaintGeometry.GetAccumulatedRenderTransform().GetMatrix().GetScale().GetVector();

			const uint32 RenderTargetWidth = FMath::RoundToInt(RenderSize.X);
			const uint32 RenderTargetHeight = FMath::RoundToInt(RenderSize.Y);

			const FVector2D ViewOffset = PaintGeometry.DrawPosition.RoundToVector();

			// Keep the visibilities the same, the proxy window should maintain the same visible/non-visible hit-testing of the retainer.
			Window->SetVisibility(GetVisibility());

			// Need to prepass.
			Window->SlatePrepass(CachedAllottedGeometry.Scale);

			if (RenderTargetWidth != 0 && RenderTargetHeight != 0)
			{
				if (MyWidget->GetVisibility().IsVisible())
				{
					bool bDynamicMaterialInUse = false;// (DynamicEffect != nullptr);
					if (RenderTarget->GetSurfaceWidth() != RenderTargetWidth ||
						RenderTarget->GetSurfaceHeight() != RenderTargetHeight ||
						RenderTarget->SRGB != bDynamicMaterialInUse ||
						RenderTarget->GetFormat() != PF_B8G8R8A8
						)
					{
						const bool bForceLinearGamma = false;
						RenderTarget->TargetGamma = bDynamicMaterialInUse ? 2.2f : 1.0f;
						RenderTarget->SRGB = bDynamicMaterialInUse;
						RenderTarget->InitCustomFormat(RenderTargetWidth, RenderTargetHeight, PF_B8G8R8A8, bForceLinearGamma);
						RenderTarget->UpdateResource();
					}

					const float Scale = CachedAllottedGeometry.Scale;

					const FVector2D DrawSize = FVector2D(RenderTargetWidth, RenderTargetHeight);
					const FGeometry WindowGeometry = FGeometry::MakeRoot(DrawSize * (1 / Scale), FSlateLayoutTransform(Scale, PaintGeometry.DrawPosition));

					// Update the surface brush to match the latest size.
					SurfaceBrush.ImageSize = DrawSize;

					WidgetRenderer->ViewOffset = -ViewOffset;

					WidgetRenderer->DrawWindow(
						RenderTarget,
						HitTestGrid.ToSharedRef(),
						Window.ToSharedRef(),
						WindowGeometry,
						WindowGeometry.GetLayoutBoundingRect(),
						TimeSinceLastDraw);
				}
			}

			LastDrawTime = FApp::GetCurrentTime();
		}
	}
}

int32 SVRRetainerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	//STAT(FScopeCycleCounter PaintCycleCounter(MyStatId);)

		SVRRetainerWidget* MutableThis = const_cast<SVRRetainerWidget*>(this);

	MutableThis->RefreshRenderingMode();

	const bool bShouldRenderAnything = IsAnythingVisibleToRender();
	if (bRenderingOffscreen && bShouldRenderAnything)
	{
		//SCOPE_CYCLE_COUNTER(STAT_SlateRetainerWidgetPaint);
		CachedAllottedGeometry = AllottedGeometry;
		CachedWindowToDesktopTransform = Args.GetWindowToDesktopTransform();

		{
			MutableThis->PaintRetainedContent(FApp::GetDeltaTime());
		}

		if (RenderTarget->GetSurfaceWidth() >= 1 && RenderTarget->GetSurfaceHeight() >= 1)
		{
			//const FLinearColor ComputedColorAndOpacity(InWidgetStyle.GetColorAndOpacityTint() * ColorAndOpacity.Get() * SurfaceBrush.GetTint(InWidgetStyle));
			// Retainer widget uses premultiplied alpha, so premultiply the color by the alpha to respect opacity.
			//const FLinearColor PremultipliedColorAndOpacity(ComputedColorAndOpacity*ComputedColorAndOpacity.A);

			/*const bool bDynamicMaterialInUse = (DynamicEffect != nullptr);
			if (bDynamicMaterialInUse)
			{
				DynamicEffect->SetTextureParameterValue(DynamicEffectTextureParameter, RenderTarget);
			}*/

			/*FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(),
				&SurfaceBrush,
				ESlateDrawEffect::PreMultipliedAlpha | ESlateDrawEffect::NoGamma,
				FLinearColor(PremultipliedColorAndOpacity.R, PremultipliedColorAndOpacity.G, PremultipliedColorAndOpacity.B, PremultipliedColorAndOpacity.A)
			);*/

			TSharedRef<SVRRetainerWidget> SharedMutableThis = SharedThis(MutableThis);
			Args.InsertCustomHitTestPath(SharedMutableThis, Args.GetLastHitTestIndex());

			// Any deferred painted elements of the retainer should be drawn directly by the main renderer, not rendered into the render target,
			// as most of those sorts of things will break the rendering rect, things like tooltips, and popup menus.
			
			// #TODO- not supporting these for VR
			/*for (auto& DeferredPaint : WidgetRenderer->DeferredPaints)
			{
				OutDrawElements.QueueDeferredPainting(DeferredPaint->Copy(Args));
			}*/
		}

		return LayerId;
	}
	else if (bShouldRenderAnything)
	{
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	return LayerId;
}

FVector2D SVRRetainerWidget::ComputeDesiredSize(float LayoutScaleMuliplier) const
{
	if (bRenderingOffscreen)
	{
		return MyWidget->GetDesiredSize();
	}
	else
	{
		return SCompoundWidget::ComputeDesiredSize(LayoutScaleMuliplier);
	}
}

TArray<FWidgetAndPointer> SVRRetainerWidget::GetBubblePathAndVirtualCursors(const FGeometry& InGeometry, FVector2D DesktopSpaceCoordinate, bool bIgnoreEnabledStatus) const
{
	const FVector2D LocalPosition = DesktopSpaceCoordinate - CachedWindowToDesktopTransform;
	const FVector2D LastLocalPosition = DesktopSpaceCoordinate - CachedWindowToDesktopTransform;

	TSharedRef<FVirtualPointerPosition> VirtualMouseCoordinate = MakeShareable(new FVirtualPointerPosition(LocalPosition, LastLocalPosition));

	// TODO Where should this come from?
	const float CursorRadius = 0;

	TArray<FWidgetAndPointer> ArrangedWidgets =
		HitTestGrid->GetBubblePath(LocalPosition, CursorRadius, bIgnoreEnabledStatus);

	for (FWidgetAndPointer& ArrangedWidget : ArrangedWidgets)
	{
		ArrangedWidget.PointerPosition = VirtualMouseCoordinate;
	}

	return ArrangedWidgets;
}

void SVRRetainerWidget::ArrangeChildren(FArrangedChildren& ArrangedChildren) const
{
	ArrangedChildren.AddWidget(FArrangedWidget(MyWidget.ToSharedRef(), CachedAllottedGeometry));
}

TSharedPtr<struct FVirtualPointerPosition> SVRRetainerWidget::TranslateMouseCoordinateFor3DChild(const TSharedRef<SWidget>& ChildWidget, const FGeometry& InGeometry, const FVector2D& ScreenSpaceMouseCoordinate, const FVector2D& LastScreenSpaceMouseCoordinate) const
{
	return MakeShareable(new FVirtualPointerPosition(ScreenSpaceMouseCoordinate, LastScreenSpaceMouseCoordinate));
}
