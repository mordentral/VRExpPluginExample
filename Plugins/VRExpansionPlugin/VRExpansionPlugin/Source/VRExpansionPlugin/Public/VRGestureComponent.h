

#pragma once

#include "CoreMinimal.h"
#include "VRBPDatatypes.h"
#include "Algo/Reverse.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "VRBaseCharacter.h"
#include "DrawDebugHelpers.h"
#include "Components/LineBatchComponent.h"
#include "VRGestureComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("TICKGesture"), STATGROUP_TickGesture, STATCAT_Advanced);


UENUM(Blueprintable)
enum class EVRGestureState : uint8
{
	GES_None,
	GES_Recording,
	GES_Detecting
};


UENUM(Blueprintable)
enum class EVRGestureMirrorMode : uint8
{
	GES_NoMirror,
	GES_MirrorLeft,
	GES_MirrorRight,
	GES_MirrorBoth
};


USTRUCT(BlueprintType, Category = "VRGestures")
struct VREXPANSIONPLUGIN_API FVRGesture
{
	GENERATED_BODY()
public:

	// Name of the recorded gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture")
	FString Name;

	// Enum uint8 for end user use
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture")
	uint8 GestureType;

	// Samples in the recorded gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	TArray<FVector> Samples;

	// Minimum length to start recognizing this gesture at
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	int Minimum_Gesture_Length;

	// Maximum distance between the last observations before throwing out this gesture, raise this to make it easier to start checking this gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	float firstThreshold;

	// Full threshold before detecting the gesture, raise this to lower accuracy but make it easier to detect this gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	float FullThreshold;

	// If set to left/right, will mirror the detected gesture if the gesture component is set to match that value
	// If set to Both mode, the gesture will be checked both normal and mirrored and the best match will be chosen
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	EVRGestureMirrorMode MirrorMode;

	// If enabled this gesture will be checked when inside a DB
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGesture|Advanced")
	bool bEnabled;

	FVRGesture()
	{
		Minimum_Gesture_Length = 1;
		firstThreshold = 5.0f;
		FullThreshold = 5.0f;
		MirrorMode = EVRGestureMirrorMode::GES_NoMirror;
		bEnabled = true;
	}
};

/**
* Items Database DataAsset, here we can save all of our game items
*/
UCLASS(BlueprintType, Category = "VRGestures")
class VREXPANSIONPLUGIN_API UGesturesDatabase : public UDataAsset
{
	GENERATED_BODY()
public:

	// Gestures in this database
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	TArray <FVRGesture> Gestures;
};


/** Delegate for notification when the lever state changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FVRGestureDetectedSignature, uint8, GestureType, FString, DetectedGestureName, int, DetectedGestureIndex, UGesturesDatabase *, GestureDataBase);

/**
* A scene component that can sample its positions to record / track VR gestures
* Core code is from https://social.msdn.microsoft.com/Forums/en-US/4a428391-82df-445a-a867-557f284bd4b1/dynamic-time-warping-to-recognize-gestures?forum=kinectsdk
* I would also like to acknowledge RuneBerg as he appears to have used the same core codebase and I discovered that halfway through implementing this
* If this algorithm should not prove stable enough I will likely look into using a more complex and faster one in the future, I have several modifications
* to the base DTW algorithm noted from a few research papers. I only implemented this one first as it was a single header file and the quickest to implement.
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGestureComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:

	// Size of obeservations vectors.
	//int dim; // Not needed, this is just dimensionality
	// Can be used for arrays of samples (IE: multiple points), could add back in eventually
	// if I decide to support three point tracked gestures or something at some point, but its a waste for single point.

	UFUNCTION(BlueprintImplementableEvent, Category = "BaseVRCharacter")
		void OnGestureDetected(uint8 GestureType, FString &DetectedGestureName, int & DetectedGestureIndex, UGesturesDatabase * GestureDatabase);

	// Call to use an object
	UPROPERTY(BlueprintAssignable, Category = "VRGestures")
		FVRGestureDetectedSignature OnGestureDetected_Bind;

	// Known sequences
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	UGesturesDatabase *GesturesDB;

	// Maximum DTW distance between an example and a sequence being classified.
//	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	//float globalThreshold;

	// Tolerance within we throw out duplicate samples
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		float SameSampleTolerance;

	// If a gesture is set to match this value then detection will mirror the gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		EVRGestureMirrorMode MirroringHand;

	// Tolerance within we throw out duplicate samples
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		AVRBaseCharacter * TargetCharacter;

	// HTZ to run recording at for detection and saving
	int RecordingHTZ;

	// Number of samples to keep in memory during detection
	int RecordingBufferSize;

	float RecordingClampingTolerance;
	bool bDrawRecordingGesture;
	bool bDrawRecordingGestureAsSpline;
	bool bGestureChanged;


	// Maximum vertical or horizontal steps in a row in the lookup table before throwing out a gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	int maxSlope;

	UPROPERTY(BlueprintReadOnly, Category = "VRGestures")
	EVRGestureState CurrentState;

	inline float GetGestureDistance(FVector Seq1, FVector Seq2, bool bMirrorGesture = false)
	{
		if (bMirrorGesture)
		{
			return FVector::DistSquared(Seq1, FVector(Seq2.X, -Seq2.Y, Seq2.Z));
		}

		return FVector::DistSquared(Seq1, Seq2);
	}

	UFUNCTION(BlueprintCallable, Category = "VRGestures",meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	void DrawDebugGesture(UObject* WorldContextObject, FTransform StartTransform, FVRGesture GestureToDraw, FColor const& Color, bool bPersistentLines = false, uint8 DepthPriority = 0, float LifeTime = -1.f, float Thickness = 0.f)
	{
#if ENABLE_DRAW_DEBUG
		UWorld* InWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		
		if (InWorld != nullptr)
		{
			// no debug line drawing on dedicated server
			if (GEngine->GetNetMode(InWorld) != NM_DedicatedServer && GestureToDraw.Samples.Num() > 1)
			{
				bool bMirrorGesture = (MirroringHand != EVRGestureMirrorMode::GES_NoMirror && MirroringHand == GestureToDraw.MirrorMode);
				FVector MirrorVector = FVector(1.f, -1.f, 1.f); // Only mirroring on Y axis to flip Left/Right

				// this means foreground lines can't be persistent 
				ULineBatchComponent* const LineBatcher = (InWorld ? ((DepthPriority == SDPG_Foreground) ? InWorld->ForegroundLineBatcher : ((bPersistentLines || (LifeTime > 0.f)) ? InWorld->PersistentLineBatcher : InWorld->LineBatcher)) : NULL);

				if (LineBatcher != NULL)
				{
					float const LineLifeTime = (LifeTime > 0.f) ? LifeTime : LineBatcher->DefaultLifeTime;

					TArray<FBatchedLine> Lines;
					FBatchedLine Line;
					Line.Color = Color;
					Line.Thickness = Thickness;
					Line.RemainingLifeTime = LineLifeTime;
					Line.DepthPriority = DepthPriority;

					FVector FirstLoc = bMirrorGesture ? GestureToDraw.Samples[GestureToDraw.Samples.Num() - 1] * MirrorVector : GestureToDraw.Samples[GestureToDraw.Samples.Num() - 1];

					for (int i = GestureToDraw.Samples.Num() - 2; i >= 0; --i)
					{
						Line.Start = bMirrorGesture ? GestureToDraw.Samples[i] * MirrorVector : GestureToDraw.Samples[i];
						
						Line.End = FirstLoc;
						FirstLoc = Line.Start;

						Line.End = StartTransform.TransformPosition(Line.End);
						Line.Start = StartTransform.TransformPosition(Line.Start);

						Lines.Add(Line);
					}

					/*Line.Start = FirstLoc;
					Line.End = StartLocation;
					Lines.Add(Line);*/


					LineBatcher->DrawLines(Lines);
				}
			}
		}
#endif
	}


	FVRGesture GestureLog;

	FVector StartVector;
	FTransform OriginatingTransform;
	float RecordingDelta;

	UFUNCTION(BlueprintCallable, Category = "VRGestures")
		void BeginRecording(bool bRunDetection, bool bDrawGesture = true, bool bDrawAsSpline = false, int SamplingHTZ = 30, int SampleBufferSize = 60, float ClampingTolerance = 0.01f);

	UFUNCTION(BlueprintCallable, Category = "VRGestures")
	FVRGesture EndRecording()
	{
		this->SetComponentTickEnabled(false);
		CurrentState = EVRGestureState::GES_None;

		return GestureLog;
	}

	// Clear the current recording
	UFUNCTION(BlueprintCallable, Category = "VRGestures")
	void ClearRecording()
	{
		GestureLog.Samples.Reset(RecordingBufferSize);
	}

	UFUNCTION(BlueprintCallable, Category = "VRGestures")
	void SaveRecording(UPARAM(ref) FVRGesture &Recording, FString RecordingName)
	{
		if (GesturesDB)
		{
			Recording.Name = RecordingName;
			GesturesDB->Gestures.Add(Recording);
		}
	}

	// Imports a spline as a gesture, Segment len is the max segment length (will break lines up into lengths of this size)
	UFUNCTION(BlueprintCallable, Category = "VRGestures")
	FVRGesture ImportSplineAsGesture(USplineComponent * HostSplineComponent, FString GestureName, bool bKeepSplineCurves = true, float SegmentLen = 10.0f)
	{
		FVRGesture NewGesture;

		if (HostSplineComponent->GetNumberOfSplinePoints() < 2 || GesturesDB == nullptr)
			return NewGesture;
		
		NewGesture.Name = GestureName;

		FVector FirstPointPos = HostSplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);

		float LastDistance= 0.f;
		float ThisDistance = 0.f;
		FVector LastDistanceV;
		FVector ThisDistanceV;
		FVector DistNormal;
		float DistAlongSegment = 0.f;

		// Realign to xForward on the gesture, normally splines lay out as X to the right
		FTransform Realignment = FTransform(FRotator(0.f,90.f,0.f), -FirstPointPos);

		// Prefill the first point
		NewGesture.Samples.Add(Realignment.TransformPosition(HostSplineComponent->GetLocationAtSplinePoint(HostSplineComponent->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::Local)));

		// Inserting in reverse order -2 so we start one down
		for (int i = HostSplineComponent->GetNumberOfSplinePoints() - 2; i >= 0; --i)
		{
			if (bKeepSplineCurves)
			{
				LastDistance = HostSplineComponent->GetDistanceAlongSplineAtSplinePoint(i + 1);
				ThisDistance = HostSplineComponent->GetDistanceAlongSplineAtSplinePoint(i);

				DistAlongSegment = FMath::Abs(ThisDistance - LastDistance);
			}
			else
			{
				LastDistanceV = Realignment.TransformPosition(HostSplineComponent->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::Local));
				ThisDistanceV = Realignment.TransformPosition(HostSplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local));

				DistAlongSegment = FVector::Dist(ThisDistanceV, LastDistanceV);
				DistNormal = ThisDistanceV - LastDistanceV;
				DistNormal.Normalize();
			}


			float SegmentCount = FMath::FloorToFloat(DistAlongSegment / SegmentLen);
			float OverFlow = FMath::Fmod(DistAlongSegment, SegmentLen);

			if (SegmentCount < 1)
			{
				SegmentCount++;
			}

			float DistPerSegment = (DistAlongSegment / SegmentCount);
			//float DistPerSegOverflow = OverFlow / SegmentCount;
			/*if (i == 0)
			{
				// Don't run last segment point
				SegmentCount--;
			}*/

			for (int j = 0; j < SegmentCount; j++)
			{
				if (j == SegmentCount - 1 && i > 0)
					DistPerSegment += OverFlow;

				if (bKeepSplineCurves)
				{
					LastDistance -= DistPerSegment;
					if (j == SegmentCount - 1 && i > 0)
					{
						LastDistance = ThisDistance;
					}
					FVector loc = Realignment.TransformPosition(HostSplineComponent->GetLocationAtDistanceAlongSpline(LastDistance, ESplineCoordinateSpace::Local));

					if(!loc.IsNearlyZero())
						NewGesture.Samples.Add(loc);
				}
				else
				{				
					LastDistanceV += DistPerSegment * DistNormal;

					if (j == SegmentCount - 1 && i > 0)
					{
						LastDistanceV = ThisDistanceV;
					}

					if (!LastDistanceV.IsNearlyZero())
						NewGesture.Samples.Add(LastDistanceV);
				}
			}
		}

		return NewGesture;
	}

	void CaptureGestureFrame();

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;


	// Recognize gesture in the given sequence.
	// It will always assume that the gesture ends on the last observation of that sequence.
	// If the distance between the last observations of each sequence is too great, or if the overall DTW distance between the two sequences is too great, no gesture will be recognized.
	void RecognizeGesture(FVRGesture inputGesture);


	// Compute the min DTW distance between seq2 and all possible endings of seq1.
	float dtw(FVRGesture seq1, FVRGesture seq2, bool bMirrorGesture = false);

};

