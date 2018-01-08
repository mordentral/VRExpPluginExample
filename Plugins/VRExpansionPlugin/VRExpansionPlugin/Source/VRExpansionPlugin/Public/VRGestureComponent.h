

#pragma once

#include "CoreMinimal.h"
#include "VRBPDatatypes.h"
#include "Algo/Reverse.h"
#include "Components/SplineMeshComponent.h"
#include "Components/SplineComponent.h"
#include "VRBaseCharacter.h"
#include "VRGestureComponent.generated.h"


UENUM(Blueprintable)
enum class EVRGestureState : uint8
{
	GES_None,
	GES_Recording,
	GES_Detecting
};

USTRUCT(BlueprintType, Category = "VRGestures")
struct VREXPANSIONPLUGIN_API FVRGesture
{
	GENERATED_BODY()
public:

	// Name of the recorded gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	FString Name;

	// Samples in the recorded gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	TArray<FVector> Samples;

	// Minimum length to start recognizing this gesture at
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	int Minimum_Gesture_Length;

	// Maximum distance between the last observations before throwing out this gesture, raise this to make it easier to start checking this gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	float firstThreshold;

	// Full threshold before detecting the gesture, raise this to lower accuracy but make it easier to detect this gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		float FullThreshold;

	// If enabled this gesture will be checked when inside a DB
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	bool bEnabled;

	FVRGesture()
	{
		Minimum_Gesture_Length = 1;
		firstThreshold = 5.0f;
		FullThreshold = 5.0f;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FVRGestureDetectedSignature, float, DTW, int, DetectedGestureIndex, FString, DetectedGestureName, UGesturesDatabase *, GestureDataBase);

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
		void OnGestureDetected(float & DTW, int & DetectedGestureIndex, FString &DetectedGestureName, UGesturesDatabase * GestureDatabase);

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

	// Tolerance within we throw out duplicate samples
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
		AVRBaseCharacter * TargetCharacter;

	// HTZ to run recording at for detection and saving
	int RecordingHTZ;

	// Number of samples to keep in memory during detection
	int RecordingBufferSize;

	float RecordingClampingTolerance;

	// Maximum vertical or horizontal steps in a row in the lookup table before throwing out a gesture
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRGestures")
	int maxSlope;

	UPROPERTY(BlueprintReadOnly, Category = "VRGestures")
	EVRGestureState CurrentState;

	FVRGesture GestureLog;


	FVector StartVector;
	FTransform OriginatingTransform;
	float RecordingDelta;

	UFUNCTION(BlueprintCallable, Category = "VRGestures")
		void BeginRecording(bool bRunDetection, int SamplingHTZ = 30, int SampleBufferSize = 60, float ClampingTolerance = 0.01f);

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

	UFUNCTION(BlueprintCallable, Category = "VRGestures")
	void ImportSplineAsGesture(USplineComponent * HostSplineComponent, FString GestureName)
	{
		if (HostSplineComponent->GetNumberOfSplinePoints() < 1 || GesturesDB == nullptr)
			return;

		FVRGesture NewGesture;
		NewGesture.Name = GestureName;

		FVector FirstPointPos = HostSplineComponent->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);

		for (int i = 1; i < HostSplineComponent->GetNumberOfSplinePoints(); ++i)
		{
			NewGesture.Samples.Insert(HostSplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local) - FirstPointPos, 0);
		}

		GesturesDB->Gestures.Add(NewGesture);
	}

	void CaptureGestureFrame();

	void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;


	// Recognize gesture in the given sequence.
	// It will always assume that the gesture ends on the last observation of that sequence.
	// If the distance between the last observations of each sequence is too great, or if the overall DTW distance between the two sequences is too great, no gesture will be recognized.
	void RecognizeGesture(FVRGesture inputGesture);


	// Compute the min DTW distance between seq2 and all possible endings of seq1.
	float dtw(FVRGesture seq1, FVRGesture seq2);

};

