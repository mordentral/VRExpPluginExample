#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "DrawDebugHelpers.h"
#include "VRArmIKActorComponent.generated.h"


namespace VectorHelpers
{
	inline static float axisAngle(FVector v, FVector forward, FVector axis)
	{
		FVector right = FVector::CrossProduct(axis, forward);
		// Why are we reconstructing the same forward vector?
		//forward = FVector::CrossProduct(right, axis);

		return FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(v, right), FVector::DotProduct(v, forward)));
	}

	inline static float getAngleBetween(FVector a, FVector b, FVector forward, FVector axis)
	{
		float angleA = axisAngle(a, forward, axis);
		float angleB = axisAngle(b, forward, axis);

		return FMath::FindDeltaAngleDegrees(angleA, angleB);
	}
}

struct CurrentReferenceTransforms
{
	FTransform CameraTransform;
	FTransform LeftHandTransform;
	FTransform RightHandTransform;
	FRotator PureCameraYaw;
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FArmTransforms
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform Transform;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FTransform upperArm;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform lowerArm;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform wrist1;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform wrist2;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform hand;
	
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float upperArmLength;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float lowerArmLength;
		UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float armLength;

	bool armLengthByScale;
	FVector scaleAxis;
	float scaleHandFactor;

	FArmTransforms()
	{
		armLengthByScale = false;
		scaleAxis = FVector(1.f);
		scaleHandFactor = .7f;
	}

	// Call on init / recalibration
	void updateArmLengths(float ShoulderWidth, float TotalWingSpan)
	{
		armLength = (TotalWingSpan - ShoulderWidth) / 2.f;
		setArmLength(armLength);
	}


	void setArmLength(float length)
	{
		float upperArmFactor = .48f;
		if (armLengthByScale)
		{
			//upperArm.SetScale3D(upperArm.GetScale3D() / armLength * length);
			//hand.SetScale3D(FVector(1.f) / (1.f - (1.f - scaleHandFactor) * (1.f - upperArm.GetScale3D().Y)));
		}
		else
		{
			upperArmLength = length * upperArmFactor;
			lowerArmLength = length * (1.f - upperArmFactor);
		}
	}
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FArmIKElbowSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool calcElbowAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool clampElbowAngle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool softClampElbowAngle;
	float maxAngle, minAngle, softClampRange;
	float offsetAngle;
	float yWeight;
	float zWeightTop, zWeightBottom, zDistanceStart;
	float xWeight, xDistanceStart;

	FArmIKElbowSettings()
	{
		calcElbowAngle = true;
		clampElbowAngle = true;
		softClampElbowAngle = true;
		maxAngle = 175.f;
		minAngle = 13.f;
		softClampRange = 10.f;
		offsetAngle = 135.f;
		yWeight = -60.f;
		zWeightTop = 260.f;
		zWeightBottom = -100.f;
		zDistanceStart = .6f; // * world to meters?
		xWeight = -50.f;
		xDistanceStart = .1f; // * world to meters?
	}
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FBeforePositioningSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool correctElbowOutside;
	float weight;
	float startBelowZ;
	float startAboveY;

	FBeforePositioningSettings()
	{
		correctElbowOutside = true;
		weight = -0.5f;
		startBelowZ = .4f; // * world to meters?
		startAboveY = 0.1f; // * World to meters?
	}
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FElbowCorrectionSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool useFixedElbowWhenNearShoulder;
	float startBelowDistance;
	float startBelowY;
	float weight;
	FVector localElbowPos;

	FElbowCorrectionSettings()
	{
		useFixedElbowWhenNearShoulder = true;
		startBelowDistance = .5f; // * world to meters?
		startBelowY = 0.1f;  // * world to meters?
		weight = 2.f;
		localElbowPos = FVector(-0.3f, 2.f, -1.f); // Check
		//public Vector3 localElbowPos = new Vector3(0.3f, -1f, -2f);
	}
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FHandSettings
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool useWristRotation;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool rotateElbowWithHandRight;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	bool rotateElbowWithHandForward;
	float handDeltaPow, handDeltaFactor, handDeltaOffset;
	// todo fix rotateElbowWithHandForward with factor != 1 -> horrible jumps. good value would be between [0.4, 0.6]
	float handDeltaForwardPow, handDeltaForwardFactor, handDeltaForwardOffset, handDeltaForwardDeadzone;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	float rotateElbowWithHandDelay;

	FHandSettings()
	{
		useWristRotation = true;
		rotateElbowWithHandRight = true;
		rotateElbowWithHandForward = true;
		handDeltaPow = 1.5f;
		handDeltaFactor = -.3f;
		handDeltaOffset = 45.f;  // * world to meters?
		// todo fix rotateElbowWithHandForward with factor != 1 -> horrible jumps. good value would be between [0.4, 0.6]
		handDeltaForwardPow = 2.f;
		handDeltaForwardFactor = 1.f; // 1.f
		handDeltaForwardOffset = 0.f;  // * world to meters?
		handDeltaForwardDeadzone = .3f;  // * world to meters?
		rotateElbowWithHandDelay = .08f; //0.08f
	}
};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FShoulderTransforms
{

	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FTransform Transform;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FTransform leftShoulder;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FTransform rightShoulder;
	//FTransform leftShoulderRenderer, rightShoulderRenderer;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FTransform leftShoulderAnchor;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FTransform rightShoulderAnchor;

};

USTRUCT(BlueprintType)
struct VREXPANSIONPLUGIN_API FVRArmIK
{
	GENERATED_USTRUCT_BODY()
public:

	float DeltaTime;
	CurrentReferenceTransforms CurrentTransforms;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	FArmTransforms armTransforms;
	FShoulderTransforms *shoulder;
	FTransform target;
	bool left;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	FArmIKElbowSettings elbowSettings;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	FBeforePositioningSettings beforePositioningSettings;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	FElbowCorrectionSettings elbowCorrectionSettings;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Component Events")
	FHandSettings handSettings;

	FRotator nextLowerArmAngle;

	FQuat upperArmStartRotation, lowerArmStartRotation, wristStartRotation, handStartRotation;

	float interpolatedDeltaElbow;
	float interpolatedDeltaElbowForward;

	USceneComponent * OwningComp;


	FVRArmIK()
	{
		shoulder = nullptr;
		left = true;
		setUpperArmRotation(FQuat::Identity);
		setLowerArmRotation(FQuat::Identity);
		setHandRotation(FQuat::Identity);
	}

	//////////////////////////////////////////////

	void Update(CurrentReferenceTransforms CurrentTrans, FShoulderTransforms * ShoulderTransforms, bool isLeft, float dTime, USceneComponent * oComp)
	{
		OwningComp = oComp;

		setUpperArmRotation(FQuat::Identity);
		setLowerArmRotation(FQuat::Identity);

		DeltaTime = dTime;
		left = isLeft;
		CurrentTransforms = CurrentTrans;
		shoulder = ShoulderTransforms;

		target = left ? CurrentTransforms.LeftHandTransform : CurrentTransforms.RightHandTransform;

		updateUpperArmPosition();
		calcElbowInnerAngle();
		rotateShoulder();
		correctElbowRotation();

		if (elbowSettings.calcElbowAngle)
		{
			positionElbow();
			if (elbowCorrectionSettings.useFixedElbowWhenNearShoulder)
				correctElbowAfterPositioning();
			
			// Below still needs iteration, something is off with them
		
			if (handSettings.rotateElbowWithHandRight)
				rotateElbowWithHandRight();
			if (handSettings.rotateElbowWithHandForward)
				rotateElbowWithHandFoward();
			//rotateHand();
		}

		// Set our lower arm positions to the elbow for easy use
		if (left)
		{
			armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + (armTransforms.upperArm.GetRotation().GetForwardVector() * armTransforms.upperArmLength));
			//armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + (armTransforms.upperArm.GetRotation().RotateVector(FVector(0.f, -1.f, 0.f) * armTransforms.upperArmLength)));
			armTransforms.lowerArm.SetRotation(lowerArmRotation());
		}
		else
		{
			armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + (armTransforms.upperArm.GetRotation().GetForwardVector() * armTransforms.upperArmLength));
			//armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + ((armTransforms.upperArm.GetRotation().GetRightVector()) * armTransforms.upperArmLength));
			armTransforms.lowerArm.SetRotation(lowerArmRotation());
		}
	}

	void updateUpperArmPosition()
	{
		armTransforms.upperArm = armTransforms.Transform;
	}

	void calcElbowInnerAngle()
	{
		float targetShoulderDistance = (target.GetLocation() - upperArmPos()).Size();
		float innerAngle = 0.f;

		if (targetShoulderDistance > armTransforms.armLength)
		{
			innerAngle = 0.f;
		}
		else
		{
			// CHECK THIS
			innerAngle =  FMath::RadiansToDegrees(FMath::Acos((FMath::Clamp((FMath::Pow(armTransforms.upperArmLength, 2.f) + FMath::Pow(armTransforms.lowerArmLength, 2.f) - 
						FMath::Pow(targetShoulderDistance, 2.f)) / (2.f * armTransforms.upperArmLength * armTransforms.lowerArmLength), -1.f, 1.f))));
			
			if (left)
			{
				innerAngle = 180.f - innerAngle;
			}
			else
			{
				innerAngle = 180.f + innerAngle;
			}
			
			if (FMath::IsNaN(innerAngle))
			{
				innerAngle = 180.f;
			}
		}

		nextLowerArmAngle.Yaw = innerAngle;
	}

	//source: https://github.com/NickHardeman/ofxIKArm/blob/master/src/ofxIKArm.cpp
	// this function appears to be working as intended currently
	void rotateShoulder()
	{
		// While this is listed as eulerAngles, our quat initializations take radians so I leave it unconverted
		float YawAngle = 0.f;
		FVector targetShoulderDirection = (target.GetLocation() - upperArmPos()).GetSafeNormal();
		float targetShoulderDistance = (target.GetLocation() - upperArmPos()).Size();

		YawAngle = (left ? -1.f : 1.f) *
			FMath::Acos(FMath::Clamp((FMath::Pow(targetShoulderDistance, 2.f) + FMath::Pow(armTransforms.upperArmLength, 2.f) -
				FMath::Pow(armTransforms.lowerArmLength, 2.f)) / (2.f * targetShoulderDistance * armTransforms.upperArmLength), -1.f, 1.f));
		
		// Skipping the nan check, shouldn't happen?
		// maybe add back in, but they do this same calc later and don't check it
		// regardless I also skipped the degree conversion
		//if (FMath::IsNaN(eulerAngles.Yaw))
		//	eulerAngles.Yaw = 0.f;

		// This requires that the shoulder be in relative space to the neck
		FQuat shoulderRightRotation = FQuat::FindBetweenNormals(/*armDirection()*/FVector::ForwardVector, targetShoulderDirection);
		setUpperArmRotation(shoulderRightRotation);

		// Rezero out our lower arm transform since I don't actually have it follow the upper arm
		//armTransforms.lowerArm.SetRotation(FQuat::Identity);
		// Rezero is handled at start of update now, don't bother

		setUpperArmRotation(FQuat(lowerArmRotation().GetUpVector(), YawAngle) * armTransforms.upperArm.GetRotation());
		setLowerArmRotation(nextLowerArmAngle.Quaternion());
	}

	// There is a singularity near the shoulders from inaccuracies, from the paper it seems expected though, can assume that this function is working as intended
	// Would need compared to the unity sample with both stopping on this function and checking results
	void correctElbowRotation()
	{
		FBeforePositioningSettings& s = beforePositioningSettings;

		FVector localTargetPos = shoulderAnker().InverseTransformPosition(target.GetLocation()) / armTransforms.armLength;
		/*localTargetPos.X = FMath::Abs(localTargetPos.X);
		localTargetPos.Y = FMath::Abs(localTargetPos.Y);
		localTargetPos.Z = FMath::Abs(localTargetPos.Z);*/

		// #TODO: unsure as to the specifics of this factor currently, its only currently coming into effect with hards near to above the shoulders
		float elbowOutsideFactor = FMath::Clamp(
			FMath::Clamp((s.startBelowZ - localTargetPos.X) /
				FMath::Abs(s.startBelowZ) * .5f, 0.f, 1.f) *
			FMath::Clamp((localTargetPos.Z - s.startAboveY) /
				FMath::Abs(s.startAboveY), 0.f, 1.f) *
			FMath::Clamp(1.f - localTargetPos.Y * (left ? -1.f : 1.f), 0.f, 1.f)
		,0.f, 1.f) * s.weight;


		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();
		FVector targetDir = shoulder->Transform.GetRotation() * (FVector::UpVector + (s.correctElbowOutside ? (armDirection() + FVector::ForwardVector * -.2f) * elbowOutsideFactor : FVector::ZeroVector));
		//targetDir.Normalize();

		// Not sure why they increased targetDir in the cross here, but it screws up the dot product below if left unnormalized
		FVector cross = FVector::CrossProduct(shoulderHandDirection, targetDir);// *1000.f);// *100000.f);
		cross.Normalize();

		FVector upperArmUp = upperArmRotation().GetUpVector();

		float elbowTargetUp = FVector::DotProduct(upperArmUp, targetDir);
		float elbowAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(cross, upperArmUp))) + (left ? 0.f : 180.f);
		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(elbowAngle * FMath::Sign(elbowTargetUp)));
		armTransforms.upperArm.SetRotation(rotation * armTransforms.upperArm.GetRotation());

		//FQuat DeltaRot = FQuat::FindBetweenNormals(upperArmUp, cross);
		//armTransforms.upperArm.SetRotation(DeltaRot * armTransforms.upperArm.GetRotation());
	}

	float getElbowTargetAngle()
	{

		FVector localHandPosNormalized = FVector::ZeroVector;

		localHandPosNormalized = shoulderAnker().InverseTransformPosition(handPos()) / armTransforms.armLength;

		// angle from Y
		float angle = elbowSettings.yWeight * localHandPosNormalized.Z + elbowSettings.offsetAngle;

		if (localHandPosNormalized.Z > 0.f)
			angle += (elbowSettings.zWeightTop * ((FMath::Max(elbowSettings.zDistanceStart - localHandPosNormalized.X, 0.f) * FMath::Max(localHandPosNormalized.Z, 0.f))));
		else
			angle += (elbowSettings.zWeightBottom * ((FMath::Max(elbowSettings.zDistanceStart - localHandPosNormalized.X, 0.f) * FMath::Max(-localHandPosNormalized.Z, 0.f))));

		// angle from X
		angle += elbowSettings.xWeight * (FMath::Max(localHandPosNormalized.Y * (left ? 1.0f : -1.0f) + elbowSettings.xDistanceStart, 0.f));

		if (elbowSettings.clampElbowAngle)
		{
			if (elbowSettings.softClampElbowAngle)
			{
				if (angle < elbowSettings.minAngle + elbowSettings.softClampRange)
				{
					float a = elbowSettings.minAngle + elbowSettings.softClampRange - angle;
					angle = elbowSettings.minAngle + elbowSettings.softClampRange * (1.f - FMath::Log2(1.f + a) * 3.f);
				}
			}
			else
			{
				angle = FMath::Clamp(angle, elbowSettings.minAngle, elbowSettings.maxAngle);
			}
		}

		if (left)
			angle *= -1.f;

		return angle;
	}

	/// <summary>
	/// reduces calculation problems when hand is moving around shoulder XZ coordinates -> forces elbow to be outside of body
	/// </summary>
	void correctElbowAfterPositioning()
	{
		FElbowCorrectionSettings s = elbowCorrectionSettings;
		FVector localTargetPos = (shoulderAnker().InverseTransformPosition(target.GetLocation()) / armTransforms.armLength);
		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();
		
		// Is the local elbow pos a generic value or is it an actual relative position
		// #TODO: check this as it make a rather large difference in the accuracy of these calculations
		FVector elbowPos = s.localElbowPos;

		if (left)
			elbowPos.Y *= -1.f;

		FVector targetDir = shoulder->Transform.GetRotation() * elbowPos.GetSafeNormal();
		DrawDebugSphere(OwningComp->GetWorld(), OwningComp->GetComponentTransform().TransformPosition(shoulderAnker().GetLocation() + targetDir * armTransforms.upperArmLength), 4.0f, 32, FColor::Orange);
		FVector cross = FVector::CrossProduct(shoulderHandDirection, targetDir);
		cross.Normalize();

		FVector upperArmUp = upperArmRotation().GetUpVector();

		FVector distance = target.GetLocation() - upperArmPos();

		distance = distance.Size() * shoulderAnker().InverseTransformVector(distance / distance.Size()); // .GetSafeNormal should work too right?
		distance /= 100.f;
		float localarm = armTransforms.armLength / 100.f;

		// #TODO: This weight needs work, need to remove the devisors, also its coming into effect far too far away from the actual shoulders
		float weight = FMath::Clamp(FMath::Clamp((s.startBelowDistance - (distance.Size2D() / localarm)) /
			s.startBelowDistance, 0.f, 1.f) * s.weight + FMath::Clamp((-distance.X + .1f) * 3, 0.f, 1.f), 0.f, 1.f) *
			FMath::Clamp((s.startBelowY - localTargetPos.Z) /
				s.startBelowY, 0.f, 1.f);

		// This can be a sub function
		float elbowTargetUp = FVector::DotProduct(upperArmUp, targetDir);
		float elbowAngle2 = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(cross, upperArmUp))) + (left ? 0.f : 180.f);

		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(FRotator::NormalizeAxis((elbowAngle2 * FMath::Sign(elbowTargetUp))) * FMath::Clamp(weight, 0.f, 1.f)));
		rotation.Normalize();

		armTransforms.upperArm.SetRotation(rotation * armTransforms.upperArm.GetRotation());
	}

	void rotateElbow(float angle)
	{
		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();

		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(angle));
		rotation.Normalize();
		setUpperArmRotation(rotation * upperArmRotation());
	}

	//source: https://github.com/NickHardeman/ofxIKArm/blob/master/src/ofxIKArm.cpp
	void positionElbow()
	{
		float targetElbowAngle = getElbowTargetAngle();
		rotateElbow(targetElbowAngle);
	}

	// Needs work
	void rotateElbowWithHandRight()
	{

		FHandSettings s = handSettings;
		FVector handUpVec = target.GetRotation() * armDirection();// .GetUpVector();

		float forwardAngle = VectorHelpers::getAngleBetween(lowerArmRotation().GetForwardVector()/*.GetRightVector()*/, target.GetRotation().GetForwardVector()/*.GetRightVector()*/,
			lowerArmRotation().GetUpVector(), lowerArmRotation().GetRightVector()/*.GetForwardVector()*/);

		// todo reduce influence if hand local forward rotation is high (hand tilted inside)
		FQuat handForwardRotation = FQuat(/*lowerArmRotation().GetForwardVector()*/lowerArmRotation().GetRightVector(), FMath::DegreesToRadians(-forwardAngle));
		handUpVec = handForwardRotation * handUpVec;

		float elbowTargetAngle = VectorHelpers::getAngleBetween(lowerArmRotation().GetUpVector(), handUpVec,
			lowerArmRotation()/*.GetForwardVector()*/.GetRightVector(), lowerArmRotation().GetForwardVector() /** armDirection()*/);

		float deltaElbow = (elbowTargetAngle + (left ? -s.handDeltaOffset : s.handDeltaOffset)) / 180.f;
		deltaElbow = FMath::Sign(deltaElbow) * FMath::Pow(FMath::Abs(deltaElbow), s.handDeltaPow) * 180.f * s.handDeltaFactor;


		// Lower the weight the closer to straight right that the hand is
		float WeightAwayFromInfluence = 0.2f;
		float WeightTowardsInfluence = 2.0f;

		float WeightedDot = FMath::Clamp((1.f - /*FMath::Abs*/(FVector::DotProduct(shoulderAnker().GetRotation() * -armDirection(), target.GetRotation().GetForwardVector()))) - WeightAwayFromInfluence, 0.f, 1.f);
		float Weight = FMath::Clamp(WeightedDot * WeightTowardsInfluence, 0.f, 1.f);
		deltaElbow = Weight * deltaElbow;

		//interpolatedDeltaElbow = deltaElbow;
		interpolatedDeltaElbow = LerpAxisOver(interpolatedDeltaElbow, deltaElbow, DeltaTime / s.rotateElbowWithHandDelay);
		//interpolatedDeltaElbow = FRotator::ClampAxis(FMath::Lerp(interpolatedDeltaElbow, deltaElbow, FMath::Clamp(DeltaTime / s.rotateElbowWithHandDelay, 0.f, 1.f)));



		rotateElbow(interpolatedDeltaElbow);
	}

	// From unity, don't think this is what we want
	inline float LerpAxisOver(float a, float b, float time)
	{
		time = FMath::Clamp(time, 0.f, 1.f);
		b = (FRotator::ClampAxis(b) - FRotator::ClampAxis(a));

		float delta = FMath::Clamp(b - FMath::FloorToFloat(b / 360.f) * 360.f, 0.0f, 360.f);

		delta = FRotator::NormalizeAxis(delta);
		delta = a + delta * FMath::Clamp(time, 0.f, 1.f);
		return FRotator::NormalizeAxis(delta);
	}

	// Needs work
	void rotateElbowWithHandFoward()
	{
		FHandSettings s = handSettings;
		FVector handRightVec = target.GetRotation() * FVector::ForwardVector; //armDirection();

		float elbowTargetAngleForward = VectorHelpers::getAngleBetween(lowerArmRotation() * /*armDirection()*/ FVector::ForwardVector, handRightVec,
			lowerArmRotation() * FVector::UpVector, lowerArmRotation() * armDirection() /*FVector::ForwardVector*/);

		float deltaElbowForward = (elbowTargetAngleForward + (left ? -s.handDeltaForwardOffset : s.handDeltaForwardOffset)) / 180.f;

		if (FMath::Abs(deltaElbowForward) < s.handDeltaForwardDeadzone)
		{
			deltaElbowForward = 0.f;
		}
		else
		{
			deltaElbowForward = (deltaElbowForward - FMath::Sign(deltaElbowForward) * s.handDeltaForwardDeadzone) / (1.f - s.handDeltaForwardDeadzone);
		}

		deltaElbowForward = FMath::Sign(deltaElbowForward) * FMath::Pow(FMath::Abs(deltaElbowForward), s.handDeltaForwardPow) * 180.f;
	
		// Lower the weight the closer to straight right that the hand is
		/*float WeightAwayFromInfluence = 0.1f;
		float WeightTowardsInfluence = 100.0f;

		FVector forward = shoulderAnker().GetRotation().GetForwardVector();
		forward.Z = 0.f;

		FVector hand = target.GetLocation() - shoulderAnker().GetLocation();
		hand = hand.GetSafeNormal2D();

		float WeightedDot = FMath::Clamp((1.f - FMath::Abs(FVector::DotProduct(forward, hand))) - WeightAwayFromInfluence, 0.f, 1.f);
		float Weight = FMath::Clamp(WeightedDot * WeightTowardsInfluence, 0.f, 1.f);
		deltaElbowForward = Weight * deltaElbowForward;

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("CORRECTION WEIGHT: %f"), Weight));*/

		interpolatedDeltaElbowForward = LerpAxisOver(interpolatedDeltaElbowForward, deltaElbowForward, DeltaTime / s.rotateElbowWithHandDelay);
		//interpolatedDeltaElbowForward = FRotator::ClampAxis(FMath::Lerp(interpolatedDeltaElbowForward, deltaElbowForward, DeltaTime / s.rotateElbowWithHandDelay));

		float signedInterpolated = -FRotator::NormalizeAxis(interpolatedDeltaElbowForward);
		rotateElbow(signedInterpolated * s.handDeltaForwardFactor);
	}

	void rotateHand()
	{
		if (handSettings.useWristRotation)
		{
			FVector handUpVec = target.GetRotation() * FVector::UpVector;
			float forwardAngle = VectorHelpers::getAngleBetween(lowerArmRotation() * FVector::RightVector, target.GetRotation() * FVector::RightVector,
				lowerArmRotation() * FVector::UpVector, lowerArmRotation() * FVector::ForwardVector);

			// todo reduce influence if hand local forward rotation is high (hand tilted inside)
			FQuat handForwardRotation = FQuat(lowerArmRotation() * FVector::ForwardVector, FMath::DegreesToRadians(-forwardAngle));
			handUpVec = handForwardRotation * handUpVec;

			float elbowTargetAngle = VectorHelpers::getAngleBetween(lowerArmRotation() * FVector::UpVector, handUpVec,
				lowerArmRotation() * FVector::ForwardVector, lowerArmRotation() * armDirection());

			elbowTargetAngle = FMath::Clamp(elbowTargetAngle, -90.f, 90.f);
			//if (arm.wrist1 != null)
				setWrist1Rotation(FQuat(lowerArmRotation() * armDirection(), FMath::DegreesToRadians(elbowTargetAngle * .3f)) * lowerArmRotation());
			//if (arm.wrist2 != null)
				setWrist2Rotation(FQuat(lowerArmRotation() * armDirection(), FMath::DegreesToRadians(elbowTargetAngle * .8f)) * lowerArmRotation());
		}

		setHandRotation(target.GetRotation());
	}

	void setUpperArmRotation(FQuat rotation)
	{
		armTransforms.upperArm.SetRotation(rotation/* * upperArmStartRotation*/);
	}

	void setLowerArmRotation(FQuat rotation)
	{
		armTransforms.lowerArm.SetRotation(rotation);// *lowerArmStartRotation);
	}

	void setLowerArmLocalRotation(FQuat rotation)
	{
		armTransforms.lowerArm.SetRotation(upperArmRotation() * rotation);// *lowerArmStartRotation);
	}

	void setWrist1Rotation(FQuat rotation)
	{
		armTransforms.wrist1.SetRotation(rotation);// *wristStartRotation);
	}

	void setWrist2Rotation(FQuat rotation)
	{
		armTransforms.wrist2.SetRotation(rotation);// *wristStartRotation);
	}

	void setWristLocalRotation(FQuat rotation)
	{
		armTransforms.wrist1.SetRotation(armTransforms.lowerArm.GetRotation() * rotation);// *wristStartRotation);
	}
	
	inline FVector armDirection()
	{
		return left ? FVector(0.f, -1.f, 0.f) : FVector(0.f, 1.f, 0.f);
	}

	inline FVector upperArmPos()
	{
		return armTransforms.upperArm.GetLocation();
	}

	inline FVector handPos()
	{
		return left ? CurrentTransforms.LeftHandTransform.GetLocation() : CurrentTransforms.RightHandTransform.GetLocation();
	}

	inline FTransform shoulderAnker()
	{
		return left ? shoulder->leftShoulderAnchor : shoulder->rightShoulderAnchor;
	}
	
	inline FQuat upperArmRotation()
	{
		return armTransforms.upperArm.GetRotation();// *upperArmStartRotation.Inverse();
	}

	inline FQuat lowerArmRotation()
	{
		return armTransforms.upperArm.GetRotation() * armTransforms.lowerArm.GetRotation();// *lowerArmStartRotation.Inverse();
	}

	inline FQuat handRotation()
	{
		return armTransforms.hand.GetRotation();// *handStartRotation.Inverse();
	}

	void setHandRotation(FQuat rotation)
	{
		armTransforms.hand.SetRotation(rotation);// *handStartRotation);
	}
};

/**
*	A custom constraint component subclass that exposes additional missing functionality from the default one
*/
UCLASS(meta = (BlueprintSpawnableComponent), HideCategories = (Activation, "Components|Activation", Physics, Mobility))
class VREXPANSIONPLUGIN_API UVRArmIKActorComponent : public USceneComponent
{
	
	GENERATED_BODY()
public:

	// Input the name of the effector that you want to track, otherwise we will track the motion controllers directly
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Effectors")
		FName LeftArmEffector;
	TWeakObjectPtr<USceneComponent> LeftArmEff;


	// Input the name of the effector that you want to track, otherwise we will track the motion controllers directly
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Effectors")
		FName RightArmEffector;
	TWeakObjectPtr<USceneComponent> RightArmEff;

	// Input the name of the effector that you want to track, otherwise we will track the camera directly
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRArmIK Effectors")
		FName HeadEffector;
	TWeakObjectPtr<USceneComponent> HeadEff;

	UVRArmIKActorComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		LeftArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
		RightArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);


		AVRBaseCharacter* OwningChar = Cast<AVRBaseCharacter>(GetOwner());
		if (!OwningChar)
			return;

		FName localName = NAME_None;
		int counter = 0;
		for (UActorComponent* aComp : OwningChar->GetComponents())
		{

			localName = aComp->GetFName();

			if (localName == LeftArmEffector)
			{
				if (USceneComponent * sComp = Cast<USceneComponent>(aComp))
				{
					LeftArmEff = sComp;
					++counter;
					if (counter >= 3)
						break;
					else
						continue;
				}
				else
					continue;
			}

			if (localName == RightArmEffector)
			{
				if (USceneComponent * sComp = Cast<USceneComponent>(aComp))
				{
					RightArmEff = sComp;
					++counter;
					if (counter >= 3)
						break;
					else
						continue;
				}
				else
					continue;
			}

			if (localName == HeadEffector)
			{
				if (USceneComponent * sComp = Cast<USceneComponent>(aComp))
				{
					HeadEff = sComp;
					++counter;
					if (counter >= 3)
						break;
					else
						continue;
				}
				else
					continue;
			}
		}

		if (!LeftArmEff.IsValid())
		{
			LeftArmEff = OwningChar->LeftMotionController;
		}

		if (!RightArmEff.IsValid())
		{
			RightArmEff = OwningChar->RightMotionController;
		}

		if (!HeadEff.IsValid())
		{
			HeadEff = OwningChar->VRReplicatedCamera;
		}
	}

		float referencePlayerHeightHmd;
		float referencePlayerWidthWrist;
		float playerHeightHmd;
		float playerWidthWrist;
		float playerWidthShoulders;
		bool loadPlayerSizeOnAwake;

		UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FVRArmIK LeftArm;

		UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FVRArmIK RightArm;

		CurrentReferenceTransforms CurrentTransforms;

		
		void DrawJoint(FTransform &JointTransform, bool bDrawAxis = true)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			FTransform WorldTransform = JointTransform * this->GetComponentTransform();
			
			DrawDebugSphere(GetWorld(), WorldTransform.GetLocation(), 3.0f, 32.f, FColor::Silver);
			
			if (bDrawAxis)
			{
				FQuat WorldRot = WorldTransform.GetRotation();
				DrawDebugLine(GetWorld(), WorldTransform.GetLocation(), WorldTransform.GetLocation() + (WorldRot.GetUpVector() * 10.f), FColor::Blue);
				DrawDebugLine(GetWorld(), WorldTransform.GetLocation(), WorldTransform.GetLocation() + (WorldRot.GetForwardVector() * 10.f), FColor::Red);
				DrawDebugLine(GetWorld(), WorldTransform.GetLocation(), WorldTransform.GetLocation() + (WorldRot.GetRightVector() * 10.f), FColor::Green);
			}
#endif
		}

		void DrawBone(FTransform &ParentBone, float BoneLength, FVector AxisToDraw)
		{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			FTransform WorldTransform = ParentBone * this->GetComponentTransform();
			FQuat WorldRot = WorldTransform.GetRotation();
			DrawDebugLine(GetWorld(), WorldTransform.GetLocation(), WorldTransform.GetLocation() + ((WorldRot.RotateVector(AxisToDraw)) * BoneLength), FColor::Magenta, false, -1.f, 0, 2.f);
#endif
		}
		
		
		virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
		{
			Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


			if (!HeadEff.IsValid() || !LeftArmEff.IsValid() || !RightArmEff.IsValid())
				return;

			// I'm using this comp as a proxy mesh component for testing
			this->SetRelativeTransform(FTransform::Identity);

			// Doing this lets it work with my FPS pawn
			FTransform ToLocalTrans = this->GetComponentTransform().Inverse();

			CurrentTransforms.CameraTransform = HeadEff->GetComponentTransform() * ToLocalTrans;
			CurrentTransforms.LeftHandTransform = LeftArmEff->GetComponentTransform() * ToLocalTrans;
			CurrentTransforms.RightHandTransform = RightArmEff->GetComponentTransform() * ToLocalTrans;

			DrawDebugLine(GetWorld(), CurrentTransforms.LeftHandTransform.GetLocation(), CurrentTransforms.LeftHandTransform.GetLocation() + CurrentTransforms.LeftHandTransform.GetRotation().GetRightVector() * 20.f, FColor::Cyan);
			
			/*CurrentTransforms.CameraTransform = OwningChar->VRReplicatedCamera->GetRelativeTransform();
			CurrentTransforms.LeftHandTransform = OwningChar->LeftMotionController->GetRelativeTransform();
			CurrentTransforms.RightHandTransform = OwningChar->RightMotionController->GetRelativeTransform();*/
			
			CurrentTransforms.PureCameraYaw = UVRExpansionFunctionLibrary::GetHMDPureYaw_I(CurrentTransforms.CameraTransform.GetRotation().Rotator());

			UpdateShoulders();

			// Current moving them into relative space to this rotated component to simulate being in front of a rotated mesh
			ToLocalTrans = this->GetRelativeTransform().Inverse();

			CurrentTransforms.CameraTransform = CurrentTransforms.CameraTransform * ToLocalTrans;
			CurrentTransforms.LeftHandTransform = CurrentTransforms.LeftHandTransform * ToLocalTrans;
			CurrentTransforms.RightHandTransform = CurrentTransforms.RightHandTransform * ToLocalTrans;

			LeftArm.Update(CurrentTransforms, &shoulder, true, GetWorld()->GetDeltaSeconds(), this);
			RightArm.Update(CurrentTransforms, &shoulder, false, GetWorld()->GetDeltaSeconds(), this);

			DrawJoint(shoulder.leftShoulder);
			DrawJoint(LeftArm.armTransforms.lowerArm);
			DrawJoint(shoulder.rightShoulder);
			DrawJoint(RightArm.armTransforms.lowerArm);

			//DrawJoint(CurrentTransforms.LeftHandTransform);
			//DrawJoint(CurrentTransforms.RightHandTransform);

			DrawBone(LeftArm.armTransforms.upperArm, LeftArm.armTransforms.upperArmLength, FVector::ForwardVector);
			DrawBone(RightArm.armTransforms.upperArm, RightArm.armTransforms.upperArmLength, FVector::ForwardVector);
			DrawBone(LeftArm.armTransforms.lowerArm, LeftArm.armTransforms.lowerArmLength, FVector::ForwardVector);
			DrawBone(RightArm.armTransforms.lowerArm, RightArm.armTransforms.lowerArmLength, FVector::ForwardVector);

			/*DrawBone(LeftArm.armTransforms.upperArm, LeftArm.armTransforms.upperArmLength, -FVector::RightVector);
			DrawBone(RightArm.armTransforms.upperArm, RightArm.armTransforms.upperArmLength, FVector::RightVector);
			DrawBone(LeftArm.armTransforms.lowerArm, LeftArm.armTransforms.lowerArmLength, -FVector::RightVector);
			DrawBone(RightArm.armTransforms.lowerArm, RightArm.armTransforms.lowerArmLength, FVector::RightVector);*/
			
			// Waist estimation?
		}

		UFUNCTION(BlueprintCallable, Category = "BaseVRCharacter|VRLocations")
		void CalibrateIK()
		{
			playerWidthWrist = (CurrentTransforms.LeftHandTransform.GetLocation() - CurrentTransforms.RightHandTransform.GetLocation()).Size();
			playerHeightHmd = CurrentTransforms.CameraTransform.GetLocation().Z;

			// Calibrate shoulders as well from frontal position?
			
			LeftArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
			RightArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
		}

	public:

		float LastTargetRot;
		bool bClampingHeadRotation;
		bool bHandsBehindHead;
		float TargetAngle;

		UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
		FShoulderTransforms shoulder;
		FVector leftShoulderAnkerStartLocalPosition;
		FVector rightShoulderAnkerStartLocalPosition;

		float headNeckDistance;
		FVector neckShoulderDistance;

		float distinctShoulderRotationMultiplier;
		float distinctShoulderRotationLimitForward;
		float distinctShoulderRotationLimitBackward;
		float distinctShoulderRotationLimitUpward;
		FVector headNeckDirectionVector;
		float rightRotationStartHeight;
		float rightRotationHeightFactor;
		float rightRotationHeadRotationFactor;
		float rightRotationHeadRotationOffset;
		float startShoulderDislocationBefore;
		float shoulderRightRotation;

		bool bEnableDistinctShoulderRotation;
		bool bEnableShoulderDislocation;
		bool bShoulderDislocated;
		bool bIgnoreZPos;

		virtual void UpdateShoulders()
		{
			shoulder.Transform.SetRotation(FQuat::Identity);
			positionShoulder();
			GetShoulderIKRot(); // Also calls to get right value

			//GetShoulderIKRot covers both of the below
			//rotateShoulderUp();
			//rotateShoulderRight();

			shoulder.leftShoulderAnchor = FTransform(leftShoulderAnkerStartLocalPosition) * shoulder.Transform;
			shoulder.rightShoulderAnchor = FTransform(rightShoulderAnkerStartLocalPosition) * shoulder.Transform;
			
			// Update our shoulder anchors now as the shoulder is done computing
			shoulder.leftShoulder = shoulder.leftShoulderAnchor; //
			shoulder.rightShoulder = shoulder.rightShoulderAnchor; //
			//shoulder.leftShoulder.SetLocation(shoulder.leftShoulderAnchor.GetLocation());// = shoulder.leftShoulderAnchor; //
			//shoulder.rightShoulder.SetLocation(shoulder.rightShoulderAnchor.GetLocation());// = shoulder.rightShoulderAnchor; //

			// Correct and use
			/*if (bEnableDistinctShoulderRotation)
			{
				rotateLeftShoulder();
				rotateRightShoulder();
			}*/

			LeftArm.armTransforms.Transform = shoulder.leftShoulder;
			RightArm.armTransforms.Transform = shoulder.rightShoulder;

			if (bEnableShoulderDislocation)
			{
				clampShoulderHandDistance();
			}
			else
			{
				LeftArm.armTransforms.Transform = shoulder.leftShoulder;
				RightArm.armTransforms.Transform = shoulder.rightShoulder;
			}

		}

		// Rotate shoulder up
		void GetShoulderIKRot(bool bGetInWorldSpace = false)
		{
			TargetAngle = GetEstShoulderRotation();

			if (true)
			{
				DetectHandsBehindHead(TargetAngle);
			}

			if (true)
			{
				ClampHeadRotationDelta(TargetAngle);
			}

			if (bGetInWorldSpace)
			{
				// implement
			}

			// Gets the yaw, I split these functions for now as I am seperating things for testing
			FQuat newRot = FQuat(FVector::UpVector, FMath::DegreesToRadians(TargetAngle)); // Make directly from up vec and angle instead
			
			// Adds pitch to the shoulder
			GetEstShoulderPitch(newRot);

			// Zero out the shoulder location and only apply Z for now, I split things for testing
			FVector shoulderloc = shoulder.Transform.GetLocation();
			this->SetRelativeLocation(FVector(shoulderloc.X, shoulderloc.Y, 0.f));
			shoulder.Transform.SetLocation(FVector(0.f, 0.f, shoulderloc.Z));
			this->SetRelativeRotation(newRot);

			// originally pitch was applied to newrot and then here, need to go back to that eventually
			//shoulder.Transform.SetRotation(newRot);
		}

		// Get combined direction angle up
		float GetEstShoulderRotation()
		{
			FVector LeftHand = CurrentTransforms.LeftHandTransform.GetLocation();
			FVector RightHand = CurrentTransforms.RightHandTransform.GetLocation();

			FRotator LeveledRel = CurrentTransforms.PureCameraYaw;

			FVector DistLeft = LeftHand - shoulder.Transform.GetLocation();
			FVector DistRight = RightHand - shoulder.Transform.GetLocation();

			if (bIgnoreZPos)
			{
				DistLeft.Z = 0.0f;
				DistRight.Z = 0.0f;
			}

			FVector DirLeft = DistLeft.GetSafeNormal();
			FVector DirRight = DistRight.GetSafeNormal();

			FVector CombinedDir = DirLeft + DirRight;
			float FinalRot = FMath::RadiansToDegrees(FMath::Atan2(CombinedDir.Y, CombinedDir.X));

			return FinalRot;
		}

		virtual void rotateLeftShoulder()
		{
			rotateShoulderUp(shoulder.leftShoulder, LeftArm.armTransforms, CurrentTransforms.LeftHandTransform,
				leftShoulderAnkerStartLocalPosition, 1.f);
		}

		virtual void rotateRightShoulder()
		{
			rotateShoulderUp(shoulder.rightShoulder, RightArm.armTransforms, CurrentTransforms.RightHandTransform,
				rightShoulderAnkerStartLocalPosition, -1.f);
		}

		void rotateShoulderUp(FTransform &shoulderSide, FArmTransforms arm, FTransform targetHand,
			FVector initialShoulderLocalPos, float angleSign)
		{
			FVector initialShoulderPos = shoulder.Transform.TransformPosition(initialShoulderLocalPos);
			FVector handShoulderOffset = targetHand.GetLocation() - initialShoulderPos;
			float armLength = arm.armLength;

			FVector targetAngle = FVector::ZeroVector;

			float forwardDistanceRatio = FVector::DotProduct(handShoulderOffset, shoulder.Transform.GetRotation().GetForwardVector()) / armLength;
			float upwardDistanceRatio = FVector::DotProduct(handShoulderOffset, shoulder.Transform.GetRotation().GetUpVector()) / armLength;
			if (forwardDistanceRatio > 0.f)
			{
				targetAngle.Z = FMath::Clamp((forwardDistanceRatio - 0.5f) * distinctShoulderRotationMultiplier, 0.f,
					distinctShoulderRotationLimitForward);
			}
			else
			{
				targetAngle.Z =  FMath::Clamp(-(forwardDistanceRatio + 0.08f) * distinctShoulderRotationMultiplier * 10.f,
					-distinctShoulderRotationLimitBackward, 0.f);
			}

			targetAngle.X = FMath::Clamp(-(upwardDistanceRatio - 0.5f) * distinctShoulderRotationMultiplier,
				-distinctShoulderRotationLimitUpward, 0.f);

			shoulderSide.SetRotation(shoulderSide.GetRotation() * (targetAngle * angleSign).Rotation().Quaternion());
		}


		void positionShoulder()
		{
			FVector headNeckOffset = CurrentTransforms.CameraTransform.GetRotation().RotateVector(headNeckDirectionVector);
			FVector targetPosition = CurrentTransforms.CameraTransform.GetLocation() + headNeckOffset * headNeckDistance;		
			shoulder.Transform.SetLocation(targetPosition + CurrentTransforms.CameraTransform.GetRotation().RotateVector(neckShoulderDistance));
		}

		void positionShoulderRelative()
		{
			FQuat deltaRot = FQuat(shoulder.Transform.GetRotation().GetRightVector(), FMath::DegreesToRadians(shoulderRightRotation));
			FVector shoulderHeadDiff = shoulder.Transform.GetLocation() - CurrentTransforms.CameraTransform.GetLocation();
			shoulder.Transform.SetLocation(deltaRot * shoulderHeadDiff + CurrentTransforms.CameraTransform.GetLocation());
		}

		void DetectHandsBehindHead(float & TargetRot)
		{
			float delta = FRotator::ClampAxis(TargetRot - LastTargetRot);

			if (delta > 150.f && delta < 210.0f && !FMath::IsNearlyZero(LastTargetRot) && !bClampingHeadRotation)
			{
				bHandsBehindHead = !bHandsBehindHead;
			}

			LastTargetRot = TargetRot;

			if (bHandsBehindHead)
			{
				TargetRot += 180.f;
			}
		}

		// Clamp head rotation delta yaw
		void ClampHeadRotationDelta(float& TargetRotation)
		{
			float HeadRotation = FRotator::ClampAxis(CurrentTransforms.PureCameraYaw.Yaw);
			float cTargetRotation = FRotator::ClampAxis(TargetRotation);

			float delta = HeadRotation - cTargetRotation;

			if ((delta > 80.f && delta < 180.f) || (delta < -180.f && delta >= -360.f + 80))
			{
				TargetRotation = HeadRotation - 80.f;
				bClampingHeadRotation = true;
			}
			else if ((delta < -80.f && delta > -180.f) || (delta > 180.f && delta < 360.f - 80.f))
			{
				TargetRotation = HeadRotation + 80.f;
				bClampingHeadRotation = true;
			}
			else
			{
				bClampingHeadRotation = false;
			}
		}

		void clampShoulderHandDistance()
		{
			FVector leftHandVector = CurrentTransforms.LeftHandTransform.GetLocation() - shoulder.leftShoulderAnchor.GetLocation();
			FVector rightHandVector = CurrentTransforms.RightHandTransform.GetLocation() - shoulder.rightShoulderAnchor.GetLocation();
			float leftShoulderHandDistance = leftHandVector.Size();
			float rightShoulderHandDistance = rightHandVector.Size();
			bShoulderDislocated = false;

			float startBeforeFactor = (1.f - startShoulderDislocationBefore);

			if (leftShoulderHandDistance > LeftArm.armTransforms.armLength * startBeforeFactor)
			{
				bShoulderDislocated = true;
				LeftArm.armTransforms.Transform.SetLocation(shoulder.leftShoulderAnchor.GetLocation() +
					leftHandVector.GetSafeNormal() *
					(leftShoulderHandDistance - LeftArm.armTransforms.armLength * startBeforeFactor));
			}
			else
			{
				LeftArm.armTransforms.Transform.SetLocation(FVector::ZeroVector);
			}

			if (rightShoulderHandDistance > RightArm.armTransforms.armLength * startBeforeFactor)
			{
				bShoulderDislocated = true;
				RightArm.armTransforms.Transform.SetLocation(shoulder.rightShoulderAnchor.GetLocation() +
					rightHandVector.GetSafeNormal() *
					(rightShoulderHandDistance -
						RightArm.armTransforms.armLength * startBeforeFactor));
			}
			else
			{
				RightArm.armTransforms.Transform.SetLocation(FVector::ZeroVector);
			}
		}

		// Rotate shoulder right
		void GetEstShoulderPitch(FQuat &OrigRot)
		{
			float HeightDiff = referencePlayerHeightHmd - CurrentTransforms.CameraTransform.GetLocation().Z;
			float relativeHeightDiff = HeightDiff / referencePlayerHeightHmd;
			FQuat HMDRot = CurrentTransforms.CameraTransform.GetRotation();
			//float headpitchEst = FQuat::FindBetweenNormals(OrigRot.GetForwardVector(), HMDRot.GetForwardVector()).GetAngle() + rightRotationHeadRotationOffset;
			float headPitchRotation = VectorHelpers::getAngleBetween(OrigRot.GetForwardVector(), HMDRot.GetForwardVector(), FVector::UpVector, OrigRot.GetRightVector()) + rightRotationHeadRotationOffset;

			float heightFactor = FMath::Clamp(relativeHeightDiff - 0.f, 0.f, 1.0f);
			shoulderRightRotation = heightFactor * rightRotationHeightFactor;
			shoulderRightRotation += FMath::Clamp(headPitchRotation * rightRotationHeadRotationFactor * heightFactor, 0.f, 50.f);

			FQuat DeltaRot = FQuat(FVector::RightVector, FMath::DegreesToRadians(shoulderRightRotation));
			shoulder.Transform.SetRotation(DeltaRot);
		}
};