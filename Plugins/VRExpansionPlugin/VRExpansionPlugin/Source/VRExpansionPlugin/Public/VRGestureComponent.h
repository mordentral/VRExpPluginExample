// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "VRBPDatatypes.h"
#include "Algo/Reverse.h"
#include "VRGestureComponent.generated.h"


USTRUCT(BlueprintType, Category = "VRGestures")
struct VREXPANSIONPLUGIN_API FVRGesture
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	TArray<FVector> Samples;
};


/**
* Items Database DataAsset, here we can save all of our game items
*/
UCLASS(BlueprintType, Category = "VRGestures")
class VREXPANSIONPLUGIN_API UGesturesDatabase : public UDataAsset
{
	GENERATED_BODY()
public:

		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		TArray <FVRGesture> gestures;
};

/**
* A scene component that can sample its positions to record / track VR gestures
*
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGestureComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	// Size of obeservations vectors.
	//int dim; // Not needed, this is just dimensionality
	// Can be used for arrays of samples (IE: multiple points), could add back in eventually

	// Known sequences
	TArray<FVRGesture> Gestures;
	//ArrayList sequences;

	// Labels of those known sequences
	//ArrayList labels;

	// Maximum DTW distance between an example and a sequence being classified.
	float globalThreshold;
	//double globalThreshold;

	// Maximum distance between the last observations of each sequence.
	float firstThreshold;
	//double firstThreshold;

	// Maximum vertical or horizontal steps in a row.
	int maxSlope;

	// Recognize gesture in the given sequence.
	// It will always assume that the gesture ends on the last observation of that sequence.
	// If the distance between the last observations of each sequence is too great, or if the overall DTW distance between the two sequence is too great, no gesture will be recognized.
	int recognize(FVRGesture inputGesture)
	{
		float minDist = MAX_FLT;
		//double minDist = double.PositiveInfinity;

		int OutGestureIndex = -1;

		for (int i = 0; i < Gestures.Num(); i++)
		{
			FVRGesture exampleGesture = Gestures[i];
			//ArrayList example = (ArrayList)sequences[i];
			
			if (FVector::Dist(inputGesture.Samples[i - 1], exampleGesture.Samples[exampleGesture.Samples.Num() - 1]) < firstThreshold)
			{
				float d = dtw(inputGesture, exampleGesture) / (exampleGesture.Samples.Num());
				if (d < minDist)
				{
					minDist = d;
					OutGestureIndex = i;
				}
			}
		}

		return (minDist < globalThreshold ? OutGestureIndex : -1);
	}
#ifndef FINDEX
#define FINDEX(array, x, y) array[y*RowCount+x]
#endif

	// Compute the min DTW distance between seq2 and all possible endings of seq1.
	float dtw(FVRGesture seq1, FVRGesture seq2)
	{

		// #TODO: Skip copying the array and reversing it in the future, we only ever use the reversed value.
		// So pre-reverse it and keep it stored like that on init. When we do the initial sample we can check off of the first index instead of last then
		
		// Should also be able to get SizeSquared for values and compared to squared thresholds instead of doing the full SQRT calc.

		// Getting number of average samples recorded over of a gesture (top down) may be able to achieve a basic % completed check

		// Init
		TArray<FVector> seq1Reversed = seq1.Samples;
		Algo::Reverse(seq1Reversed);

		TArray<FVector> seq2Reversed = seq2.Samples;
		Algo::Reverse(seq2Reversed);

		int ColumnCount = seq1Reversed.Num() + 1;
		int RowCount = seq2Reversed.Num() + 1;

		TArray<float> LookupTable;
		LookupTable.AddZeroed(ColumnCount * RowCount);

		TArray<int> SlopeI;
		SlopeI.AddZeroed(ColumnCount * RowCount);
		TArray<int> SlopeJ;
		SlopeJ.AddZeroed(ColumnCount * RowCount);

		for (int i = 0; i < (ColumnCount * RowCount); i++)
		{
			LookupTable[i] = MAX_FLT;
			//tab[i* j] = double.PositiveInfinity;
			//slopeI[i, j] = 0;
			//slopeJ[i, j] = 0;
		}
		// Don't need to do this, it is already handled by add zeroed
		//tab[0, 0] = 0;

		// Dynamic computation of the DTW matrix.
		for (int i = 1; i < ColumnCount; i++)
		{
			for (int j = 1; j < RowCount; j++)
			{
				if (FINDEX(LookupTable, i, j - 1) < FINDEX(LookupTable, i - 1, j - 1) && FINDEX(LookupTable, i, j - 1) < FINDEX(LookupTable, i - 1, j) && FINDEX(SlopeI, i, j - 1) < maxSlope)
				{
					FINDEX(LookupTable, i, j) = FVector::Dist(seq1Reversed[i - 1], seq2Reversed[j - 1]) + FINDEX(LookupTable, i, j - 1);
					FINDEX(SlopeI, i, j) = FINDEX(SlopeJ, i, j - 1) + 1;
					FINDEX(SlopeJ, i, j) = 0;
				}
				else if (FINDEX(LookupTable, i - 1, j) < FINDEX(LookupTable, i - 1, j - 1) && FINDEX(LookupTable, i - 1, j) < FINDEX(LookupTable, i, j - 1) && FINDEX(SlopeJ, i - 1, j) < maxSlope)
				{
					FINDEX(LookupTable, i, j) = FVector::Dist(seq1Reversed[i - 1], seq2Reversed[j - 1]) + FINDEX(LookupTable, i - 1, j);
					FINDEX(SlopeI, i, j) = 0;
					FINDEX(SlopeJ, i, j) = FINDEX(SlopeJ, i - 1, j) + 1;
				}
				else
				{
					FINDEX(LookupTable, i, j) = FVector::Dist(seq1Reversed[i - 1], seq2Reversed[j - 1]) + FINDEX(LookupTable, i - 1, j - 1);
					FINDEX(SlopeI, i, j) = 0;
					FINDEX(SlopeJ, i, j) = 0;
				}
			}
		}

		// Find best between seq2 and an ending (postfix) of seq1.
		float bestMatch = FLT_MAX;

		for (int i = 1; i < seq1Reversed.Num() + 1; i++)
		{
			if (FINDEX(LookupTable, i, seq2Reversed.Num()) < bestMatch)
				bestMatch = FINDEX(LookupTable, i, seq2Reversed.Num());
		}

		return bestMatch;
	}

#ifdef FINDEX
#undef FINDEX
#endif


};

