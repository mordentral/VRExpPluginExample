#include "VRGestureComponent.h"


UVRGestureComponent::UVRGestureComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = false;

	maxSlope = 3;// INT_MAX;
	globalThreshold = 10.0f;
	SameSampleTolerance = 0.1f;
}

void UVRGestureComponent::CaptureGestureFrame()
{

	FVector NewSample = this->GetComponentLocation() - StartVector;

	if (RecordingClampingTolerance > 0.0f)
	{
		NewSample.X = FMath::GridSnap(NewSample.X, RecordingClampingTolerance);
		NewSample.Y = FMath::GridSnap(NewSample.Y, RecordingClampingTolerance);
		NewSample.Z = FMath::GridSnap(NewSample.Z, RecordingClampingTolerance);
	}

	// Add in newest sample at beginning (reverse order)
	if (NewSample != FVector::ZeroVector && (GestureLog.Samples.Num() < 1 || !GestureLog.Samples[0].Equals(NewSample, SameSampleTolerance)))
	{
		// Pop off oldest sample
		if (GestureLog.Samples.Num() >= RecordingBufferSize)
			GestureLog.Samples.Pop(false);

		GestureLog.Samples.Insert(NewSample, 0);
	}
}

void UVRGestureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (CurrentState)
	{
	case EVRGestureState::GES_Detecting:
	{
		RecordingDelta += DeltaTime;

		if (RecordingDelta >= 1.0f / RecordingHTZ)
		{
			CaptureGestureFrame();
			RecognizeGesture(GestureLog);
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
}

void UVRGestureComponent::RecognizeGesture(FVRGesture inputGesture)
{
	if (!GesturesDB || inputGesture.Samples.Num() < 1)
		return;

	float minDist = MAX_FLT;

	int OutGestureIndex = -1;

	for (int i = 0; i < GesturesDB->Gestures.Num(); i++)
	{
		FVRGesture exampleGesture = GesturesDB->Gestures[i];
		if (!exampleGesture.bEnabled || exampleGesture.Samples.Num() < 1)
			continue;

		if (FVector::DistSquared(inputGesture.Samples[0], exampleGesture.Samples[0]) < FMath::Square(exampleGesture.firstThreshold))
		{
			float d = dtw(inputGesture, exampleGesture) / (exampleGesture.Samples.Num());
			if (d < minDist)
			{
				minDist = d;
				OutGestureIndex = i;
			}
		}
	}

	if (minDist < FMath::Square(globalThreshold) && OutGestureIndex != -1)
	{
		OnGestureDetected(minDist, OutGestureIndex, GesturesDB);
		OnGestureDetected_Bind.Broadcast(minDist, OutGestureIndex, GesturesDB);
		ClearRecording(); // Clear the recording out, we don't want to detect this gesture again with the same data
	}
}

float UVRGestureComponent::dtw(FVRGesture seq1, FVRGesture seq2)
{

	// #TODO: Skip copying the array and reversing it in the future, we only ever use the reversed value.
	// So pre-reverse it and keep it stored like that on init. When we do the initial sample we can check off of the first index instead of last then

	// Should also be able to get SizeSquared for values and compared to squared thresholds instead of doing the full SQRT calc.

	// Getting number of average samples recorded over of a gesture (top down) may be able to achieve a basic % completed check

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
				LookupTable[icol + j] = FVector::DistSquared(seq1.Samples[i - 1], seq2.Samples[j - 1]) + LookupTable[icol + j - 1];
				SlopeI[icol + j] = SlopeJ[icol + j - 1] + 1;
				SlopeJ[icol + j] = 0;
			}
			else if (
				LookupTable[icolneg + j] < LookupTable[icolneg + j - 1] &&
				LookupTable[icolneg + j] < LookupTable[icol + j - 1] &&
				SlopeJ[icolneg + j] < maxSlope)
			{
				LookupTable[icol + j] = FVector::DistSquared(seq1.Samples[i - 1], seq2.Samples[j - 1]) + LookupTable[icolneg + j];
				SlopeI[icol + j] = 0;
				SlopeJ[icol + j] = SlopeJ[icolneg + j] + 1;
			}
			else
			{
				LookupTable[icol + j] = FVector::DistSquared(seq1.Samples[i - 1], seq2.Samples[j - 1]) + LookupTable[icolneg + j - 1];
				SlopeI[icol + j] = 0;
				SlopeJ[icol + j] = 0;
			}
		}
	}

	// Find best between seq2 and an ending (postfix) of seq1.
	float bestMatch = FLT_MAX;

	for (int i = 1; i < seq1.Samples.Num() + 1 - seq2.Minimum_Gesture_Length; i++)
	{
		if (LookupTable[(i*ColumnCount) + seq2.Samples.Num()] < bestMatch)
		bestMatch = LookupTable[(i*ColumnCount) + seq2.Samples.Num()];
	}

	return bestMatch;
}
