#include "VRGestureComponent.h"

DECLARE_CYCLE_STAT(TEXT("TickGesture ~ TickingGesture"), STAT_TickGesture, STATGROUP_TickGesture);

UVRGestureComponent::UVRGestureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = false;

	maxSlope = 3;// INT_MAX;
	//globalThreshold = 10.0f;
	SameSampleTolerance = 0.1f;
	bGestureChanged = false;
	MirroringHand = EVRGestureMirrorMode::GES_NoMirror;
	bDrawSplinesCurved = true;
	bGetGestureInWorldSpace = true;
}

void UVRGestureComponent::BeginRecording(bool bRunDetection, bool bDrawGesture, bool bDrawAsSpline, int SamplingHTZ, int SampleBufferSize, float ClampingTolerance)
{
	RecordingBufferSize = SampleBufferSize;
	RecordingHTZ = SamplingHTZ;
	RecordingClampingTolerance = ClampingTolerance;
	bDrawRecordingGesture = bDrawGesture;
	bDrawRecordingGestureAsSpline = bDrawAsSpline;
	GestureLog.GestureSize.Init();

	// Reinit the drawing spline
	if (!bDrawAsSpline || !bDrawGesture)
		RecordingGestureDraw.Clear(); // Not drawing or not as a spline, remove the components if they exist
	else
	{
		RecordingGestureDraw.Reset(); // Otherwise just clear points and hide mesh components

		if (RecordingGestureDraw.SplineComponent == nullptr)
		{
			RecordingGestureDraw.SplineComponent = NewObject<USplineComponent>(GetAttachParent());
			RecordingGestureDraw.SplineComponent->RegisterComponentWithWorld(GetWorld());
			RecordingGestureDraw.SplineComponent->SetMobility(EComponentMobility::Movable);
			RecordingGestureDraw.SplineComponent->AttachToComponent(GetAttachParent(), FAttachmentTransformRules::KeepRelativeTransform);
			RecordingGestureDraw.SplineComponent->ClearSplinePoints(true);
		}
	}

	// Reset does the reserve already
	GestureLog.Samples.Reset(RecordingBufferSize);
	RecordingDelta = 0.0f;

	CurrentState = bRunDetection ? EVRGestureState::GES_Detecting : EVRGestureState::GES_Recording;

	if (TargetCharacter != nullptr)
	{
		OriginatingTransform = TargetCharacter->OffsetComponentToWorld;
	}
	else if (AVRBaseCharacter * own = Cast<AVRBaseCharacter>(GetOwner()))
	{
		TargetCharacter = own;
		OriginatingTransform = TargetCharacter->OffsetComponentToWorld;
	}
	else
		OriginatingTransform = this->GetComponentTransform();

	StartVector = OriginatingTransform.InverseTransformPosition(this->GetComponentLocation());
	this->SetComponentTickEnabled(true);
}

void UVRGestureComponent::CaptureGestureFrame()
{

	FVector NewSample = OriginatingTransform.InverseTransformPosition(this->GetComponentLocation()) - StartVector;

	if (RecordingClampingTolerance > 0.0f)
	{
		NewSample.X = FMath::GridSnap(NewSample.X, RecordingClampingTolerance);
		NewSample.Y = FMath::GridSnap(NewSample.Y, RecordingClampingTolerance);
		NewSample.Z = FMath::GridSnap(NewSample.Z, RecordingClampingTolerance);
	}

	// Add in newest sample at beginning (reverse order)
	if (NewSample != FVector::ZeroVector && (GestureLog.Samples.Num() < 1 || !GestureLog.Samples[0].Equals(NewSample, SameSampleTolerance)))
	{
		bool bClearLatestSpline = false;
		// Pop off oldest sample
		if (GestureLog.Samples.Num() >= RecordingBufferSize)
		{
			GestureLog.Samples.Pop(false);
			bClearLatestSpline = true;
		}
		
		GestureLog.GestureSize.Max.X = FMath::Max(NewSample.X, GestureLog.GestureSize.Max.X);
		GestureLog.GestureSize.Max.Y = FMath::Max(NewSample.Y, GestureLog.GestureSize.Max.Y);
		GestureLog.GestureSize.Max.Z = FMath::Max(NewSample.Z, GestureLog.GestureSize.Max.Z);

		GestureLog.GestureSize.Min.X = FMath::Min(NewSample.X, GestureLog.GestureSize.Min.X);
		GestureLog.GestureSize.Min.Y = FMath::Min(NewSample.Y, GestureLog.GestureSize.Min.Y);
		GestureLog.GestureSize.Min.Z = FMath::Min(NewSample.Z, GestureLog.GestureSize.Min.Z);


		if (bDrawRecordingGesture && bDrawRecordingGestureAsSpline && SplineMesh != nullptr && SplineMaterial != nullptr)
		{

			if (bClearLatestSpline)
				RecordingGestureDraw.ClearLastPoint();

			FSplinePoint newPoint;
			RecordingGestureDraw.SplineComponent->AddSplinePoint(NewSample, ESplineCoordinateSpace::Local, false);
			int SplineIndex = RecordingGestureDraw.SplineComponent->GetNumberOfSplinePoints() - 1;
			RecordingGestureDraw.SplineComponent->SetSplinePointType(SplineIndex, bDrawSplinesCurved ? ESplinePointType::Curve : ESplinePointType::Linear, true);

			bool bFoundEmptyMesh = false;
			USplineMeshComponent * MeshComp = nullptr;
			int MeshIndex = 0;

			for (int i = 0; i < RecordingGestureDraw.SplineMeshes.Num(); i++)
			{
				MeshIndex = i;
				MeshComp = RecordingGestureDraw.SplineMeshes[i];
				if (MeshComp == nullptr)
				{
					RecordingGestureDraw.SplineMeshes[i] = NewObject<USplineMeshComponent>(RecordingGestureDraw.SplineComponent);
					MeshComp = RecordingGestureDraw.SplineMeshes[i];

					MeshComp->RegisterComponentWithWorld(GetWorld());
					MeshComp->SetMobility(EComponentMobility::Movable);
					MeshComp->SetStaticMesh(SplineMesh);
					MeshComp->SetMaterial(0, SplineMaterial);
					bFoundEmptyMesh = true;
					break;
				}
				else if (!MeshComp->IsVisible())
				{
					bFoundEmptyMesh = true;
					break;
				}
			}

			if (!bFoundEmptyMesh)
			{
				USplineMeshComponent * newSplineMesh = NewObject<USplineMeshComponent>(RecordingGestureDraw.SplineComponent);
				MeshComp = newSplineMesh;
				MeshComp->RegisterComponentWithWorld(GetWorld());
				MeshComp->SetMobility(EComponentMobility::Movable);
				RecordingGestureDraw.SplineMeshes.Add(MeshComp);
				MeshIndex = RecordingGestureDraw.SplineMeshes.Num() - 1;
				MeshComp->SetStaticMesh(SplineMesh);
				MeshComp->SetMaterial(0, SplineMaterial);
				if (!bGetGestureInWorldSpace && TargetCharacter)
					MeshComp->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			}

			if (MeshComp != nullptr)
			{
				// Fill in last mesh component tangent and end pos
				if (RecordingGestureDraw.LastIndexSet != MeshIndex && RecordingGestureDraw.SplineMeshes[RecordingGestureDraw.LastIndexSet] != nullptr)
				{
					RecordingGestureDraw.SplineMeshes[RecordingGestureDraw.LastIndexSet]->SetEndPosition(NewSample, false);
					RecordingGestureDraw.SplineMeshes[RecordingGestureDraw.LastIndexSet]->SetEndTangent(RecordingGestureDraw.SplineComponent->GetTangentAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local), true);
				}

				MeshComp->SetStartAndEnd(NewSample,
					RecordingGestureDraw.SplineComponent->GetTangentAtSplinePoint(SplineIndex, ESplineCoordinateSpace::Local),
					NewSample,
					FVector::ZeroVector,
					true);

				if (bGetGestureInWorldSpace)
					MeshComp->SetWorldLocationAndRotation(OriginatingTransform.TransformPosition(StartVector), OriginatingTransform.GetRotation());
				else
					MeshComp->SetRelativeLocationAndRotation(/*OriginatingTransform.TransformPosition(*/StartVector/*)*/, FQuat::Identity/*OriginatingTransform.GetRotation()*/);

				RecordingGestureDraw.LastIndexSet = MeshIndex;
				MeshComp->SetVisibility(true);
			}
		
		}

		GestureLog.Samples.Insert(NewSample, 0);
		bGestureChanged = true;
	}
}

void UVRGestureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	SCOPE_CYCLE_COUNTER(STAT_TickGesture);

	switch (CurrentState)
	{
	case EVRGestureState::GES_Detecting:
	{
		RecordingDelta += DeltaTime;

		if (RecordingDelta >= 1.0f / RecordingHTZ)
		{
			CaptureGestureFrame();
			RecognizeGesture(GestureLog);
			bGestureChanged = false;
			RecordingDelta = 0.0f;
		}
	}break;

	case EVRGestureState::GES_Recording:
	{
		RecordingDelta += DeltaTime;


		if (RecordingDelta >= 1.0f / RecordingHTZ)
		{
			CaptureGestureFrame();
			RecordingDelta = 0.0f;
		}
	}break;

	case EVRGestureState::GES_None:
	default: {}break;
	}

	if (bDrawRecordingGesture)
	{
		if (!bDrawRecordingGestureAsSpline)
		{
			DrawDebugGesture(this, FTransform(StartVector) * OriginatingTransform, GestureLog, FColor::White);
		}
		else
		{

		}
	}
}

void UVRGestureComponent::RecognizeGesture(FVRGesture inputGesture)
{
	if (!GesturesDB || inputGesture.Samples.Num() < 1 || !bGestureChanged)
		return;

	float minDist = MAX_FLT;

	int OutGestureIndex = -1;
	bool bMirrorGesture = false;

	for (int i = 0; i < GesturesDB->Gestures.Num(); i++)
	{
		FVRGesture exampleGesture = GesturesDB->Gestures[i];

		if (!exampleGesture.GestureSettings.bEnabled || exampleGesture.Samples.Num() < 1 || inputGesture.Samples.Num() < exampleGesture.GestureSettings.Minimum_Gesture_Length)
			continue;

		bMirrorGesture = (MirroringHand != EVRGestureMirrorMode::GES_NoMirror && MirroringHand != EVRGestureMirrorMode::GES_MirrorBoth && MirroringHand == exampleGesture.GestureSettings.MirrorMode);

		FVector Size = inputGesture.GestureSize.GetSize();
		float Scaler = GesturesDB->TargetGestureScale / Size.GetMax();

		if (GetGestureDistance(inputGesture.Samples[0] * Scaler, exampleGesture.Samples[0], bMirrorGesture) < FMath::Square(exampleGesture.GestureSettings.firstThreshold))
		{
			float d = dtw(inputGesture, exampleGesture, bMirrorGesture, Scaler) / (exampleGesture.Samples.Num());
			if (d < minDist && d < FMath::Square(exampleGesture.GestureSettings.FullThreshold))
			{
				minDist = d;
				OutGestureIndex = i;
			}
		}
		else if (exampleGesture.GestureSettings.MirrorMode == EVRGestureMirrorMode::GES_MirrorBoth)
		{
			bMirrorGesture = true;
			if (GetGestureDistance(inputGesture.Samples[0] * Scaler, exampleGesture.Samples[0], bMirrorGesture) < FMath::Square(exampleGesture.GestureSettings.firstThreshold))
			{
				float d = dtw(inputGesture, exampleGesture, bMirrorGesture, Scaler) / (exampleGesture.Samples.Num());
				if (d < minDist && d < FMath::Square(exampleGesture.GestureSettings.FullThreshold))
				{
					minDist = d;
					OutGestureIndex = i;
				}
			}
		}

		/*if (exampleGesture.MirrorMode == EVRGestureMirrorMode::GES_MirrorBoth)
		{
			bMirrorGesture = true;

			if (GetGestureDistance(inputGesture.Samples[0], exampleGesture.Samples[0], bMirrorGesture) < FMath::Square(exampleGesture.GestureSettings.firstThreshold))
			{
				float d = dtw(inputGesture, exampleGesture, bMirrorGesture) / (exampleGesture.Samples.Num());
				if (d < minDist && d < FMath::Square(exampleGesture.GestureSettings.FullThreshold))
				{
					minDist = d;
					OutGestureIndex = i;
				}
			}
		}*/
	}

	if (/*minDist < FMath::Square(globalThreshold) && */OutGestureIndex != -1)
	{
		OnGestureDetected(GesturesDB->Gestures[OutGestureIndex].GestureType, /*minDist,*/ GesturesDB->Gestures[OutGestureIndex].Name, OutGestureIndex, GesturesDB);
		OnGestureDetected_Bind.Broadcast(GesturesDB->Gestures[OutGestureIndex].GestureType, /*minDist,*/ GesturesDB->Gestures[OutGestureIndex].Name, OutGestureIndex, GesturesDB);
		ClearRecording(); // Clear the recording out, we don't want to detect this gesture again with the same data
		RecordingGestureDraw.Reset();
	}
}

float UVRGestureComponent::dtw(FVRGesture seq1, FVRGesture seq2, bool bMirrorGesture, float Scaler)
{

	// #TODO: Skip copying the array and reversing it in the future, we only ever use the reversed value.
	// So pre-reverse it and keep it stored like that on init. When we do the initial sample we can check off of the first index instead of last then

	// Should also be able to get SizeSquared for values and compared to squared thresholds instead of doing the full SQRT calc.

	// Getting number of average samples recorded over of a gesture (top down) may be able to achieve a basic % completed check
	// to see how far into detecting a gesture we are, this would require ignoring the last position threshold though....

	int RowCount = seq1.Samples.Num() + 1;
	int ColumnCount = seq2.Samples.Num() + 1;

	TArray<float> LookupTable;
	LookupTable.AddZeroed(ColumnCount * RowCount);

	TArray<int> SlopeI;
	SlopeI.AddZeroed(ColumnCount * RowCount);
	TArray<int> SlopeJ;
	SlopeJ.AddZeroed(ColumnCount * RowCount);

	for (int i = 1; i < (ColumnCount * RowCount); i++)
	{
		LookupTable[i] = MAX_FLT;
	}
	// Don't need to do this, it is already handled by add zeroed
	//tab[0, 0] = 0;

	int icol = 0, icolneg = 0;

	// Dynamic computation of the DTW matrix.
	for (int i = 1; i < RowCount; i++)
	{
		for (int j = 1; j < ColumnCount; j++)
		{
			icol = i * ColumnCount;
			icolneg = icol - ColumnCount;// (i - 1) * ColumnCount;

			if (
				LookupTable[icol + (j - 1)] < LookupTable[icolneg + (j - 1)] &&
				LookupTable[icol + (j - 1)] < LookupTable[icolneg + j] &&
				SlopeI[icol + (j - 1)] < maxSlope)
			{
				LookupTable[icol + j] = GetGestureDistance(seq1.Samples[i - 1] * Scaler, seq2.Samples[j - 1], bMirrorGesture) + LookupTable[icol + j - 1];
				SlopeI[icol + j] = SlopeJ[icol + j - 1] + 1;
				SlopeJ[icol + j] = 0;
			}
			else if (
				LookupTable[icolneg + j] < LookupTable[icolneg + j - 1] &&
				LookupTable[icolneg + j] < LookupTable[icol + j - 1] &&
				SlopeJ[icolneg + j] < maxSlope)
			{
				LookupTable[icol + j] = GetGestureDistance(seq1.Samples[i - 1] * Scaler, seq2.Samples[j - 1], bMirrorGesture) + LookupTable[icolneg + j];
				SlopeI[icol + j] = 0;
				SlopeJ[icol + j] = SlopeJ[icolneg + j] + 1;
			}
			else
			{
				LookupTable[icol + j] = GetGestureDistance(seq1.Samples[i - 1] * Scaler, seq2.Samples[j - 1], bMirrorGesture) + LookupTable[icolneg + j - 1];
				SlopeI[icol + j] = 0;
				SlopeJ[icol + j] = 0;
			}
		}
	}

	// Find best between seq2 and an ending (postfix) of seq1.
	float bestMatch = FLT_MAX;

	for (int i = 1; i < seq1.Samples.Num() + 1/* - seq2.Minimum_Gesture_Length*/; i++)
	{
		if (LookupTable[(i*ColumnCount) + seq2.Samples.Num()] < bestMatch)
		bestMatch = LookupTable[(i*ColumnCount) + seq2.Samples.Num()];
	}

	return bestMatch;
}
