#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "VRArmIKActorComponent.generated.h"


/*
#TODO: 

Fix the arm length stuff to be precomputed when the positions change, or precalc based on arm width setting.
No idea why they were dynamically getting it always


*/

namespace VectorHelpers
{
	inline static float axisAngle(FVector v, FVector forward, FVector axis)
	{
		FVector right = FVector::CrossProduct(axis, forward);
		forward = FVector::CrossProduct(right, axis);
		return FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(v, right), FVector::DotProduct(v, forward)));
	}

	inline static float getAngleBetween(FVector a, FVector b, FVector forward, FVector axis)
	{
		float angleA = axisAngle(a, forward, axis);
		float angleB = axisAngle(b, forward, axis);

		return FMath::FindDeltaAngleDegrees(angleA, angleB);
		//return Mathf.DeltaAngle(angleA, angleB);
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
	
	/*inline float upperArmLength
	{
		return FVector::Dist(upperArm.GetLocation(), lowerArm.GetLocation());
	}

	inline float lowerArmLength
	{
		return FVector::Dist(lowerArm.GetLocation(), hand.GetLocation());
	}

	inline float armLength
	{
		return upperArmLength + lowerArmLength;
	}*/
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float upperArmLength;
	UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float lowerArmLength;
		UPROPERTY(BlueprintReadOnly, Category = "VRArmIK Component Events")
	float armLength;

	bool armLengthByScale;// = false;
	FVector scaleAxis;// = FVector(1.f);
	float scaleHandFactor;// = .7f;

	FArmTransforms()
	{
		//float upperArmLength;// = > distance(upperArm, lowerArm);
		//float lowerArmLength;// = > distance(lowerArm, hand);
		//float armLength;// = > upperArmLength + lowerArmLength;
		armLengthByScale = false;
		scaleAxis = FVector(1.f);
		scaleHandFactor = .7f;
	}

	//float distance(FTransform a, FTransform b) = > (a.position - b.position).magnitude;

	/*void Start()
	{
		PoseManager.Instance.onCalibrate += updateArmLengths;
		updateArmLengths();
	}*/

	// Call on init / recalibration
	void updateArmLengths(float ShoulderWidth, float TotalWingSpan)
	{
		//float shoulderWidth = (upperArm.GetLocation() - lowerArm.GetLocation()).Size();
		/*float */armLength = (TotalWingSpan - ShoulderWidth) / 2.f;
		setArmLength(armLength);
	}

	void setUpperArmLength(float length)
	{
		if (armLengthByScale)
		{
			//float oldLowerArmLength = (lowerArm.GetLocation() - hand.GetLocation()).Size();

			// Scale is local scale, relative space
			//FVector newScale = upperArm.GetScale3D() - (upperArm.GetScale3D() * scaleAxis).Size() * scaleAxis;
			//float scaleFactor = (upperArm.GetScale3D() * scaleAxis).Size() / upperArmLength * length;
			//newScale += scaleAxis * scaleFactor;
			//upperArm.SetScale3D(newScale);

			//setLowerArmLength(oldLowerArmLength);
		}
		else
		{
			// Local position for relative space
			//FVector pos = lowerArm.GetLocation();
			//pos.Y = FMath::Sign(pos.Y) * length;
			//lowerArm.SetLocation(pos);
		}
	}

	void setLowerArmLength(float length)
	{
		if (armLengthByScale)
		{
		}
		else
		{
			// Relative position
			//FVector pos = hand.GetLocation();
		//	pos.Y = FMath::Sign(pos.Y) * length;
			//hand.SetLocation(pos);
		}
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
			setUpperArmLength(upperArmLength);
			lowerArmLength = length * (1.f - upperArmFactor);
			setLowerArmLength(lowerArmLength);
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
		zDistanceStart = .6f * 100.f; // * world to meters?
		xWeight = -50.f;
		xDistanceStart = .1f * 100.f; // * world to meters?
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
		startBelowZ = .4f;// *100.f; // * world to meters?
		startAboveY = 0.1f;// *100.f; // * World to meters?
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
		startBelowDistance = .5f * 100.f;  // * world to meters?
		startBelowY = 0.1f * 100.f; // * world to meters?
		weight = 2.f;
		localElbowPos = FVector(-2.f, 0.3f, -1.f) * 100.f; // Check
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
		handDeltaForwardFactor = 1.f;
		handDeltaForwardOffset = 0.f;  // * world to meters?
		handDeltaForwardDeadzone = .3f;  // * world to meters?
		rotateElbowWithHandDelay = .08f;
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

	void Start()
	{
		//setShoulderWidth(PoseManager.Instance.playerWidthShoulders);
	}

	void setShoulderWidth(float width)
	{
		/*FVector localScale = FVector(width * .5f, .05f, .05f);
		FVector localPosition = FVector(width * .25f, 0.f, 0.f);

		leftShoulderRenderer.SetScale3D(localScale);
		leftShoulderRenderer.SetLocation(-localPosition);

		rightShoulderRenderer.SetScale3D(localScale);
		rightShoulderRenderer.SetLocation(localPosition);*/
	}
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
	//float shoulderRightRotation; // Only value used from shoulder poser
	//ShoulderPoser shoulderPoser;
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


	FVRArmIK()
	{
		shoulder = nullptr;
		left = true;
		setUpperArmRotation(FQuat::Identity);
		setLowerArmRotation(FQuat::Identity);
		setHandRotation(FQuat::Identity);
	}

	//////////////////////////////////////////////

	void Awake()
	{
		/*upperArmStartRotation = arm.upperArm.rotation;
		lowerArmStartRotation = arm.lowerArm.rotation;
		wristStartRotation = Quaternion.identity;
		if (arm.wrist1 != null)
			wristStartRotation = arm.wrist1.rotation;
		handStartRotation = arm.hand.rotation;*/
	}

	void Update(CurrentReferenceTransforms CurrentTrans, FShoulderTransforms * ShoulderTransforms, bool isLeft, float dTime)
	{
		//setUpperArmRotation(FQuat::Identity);
		//setLowerArmRotation(FQuat::Identity);

		DeltaTime = dTime;
		left = isLeft;
		CurrentTransforms = CurrentTrans;
		shoulder = ShoulderTransforms;
		//shoulderRightRotation = shoulderRRotation;

		target = left ? CurrentTransforms.LeftHandTransform : CurrentTransforms.RightHandTransform;

		updateUpperArmPosition();
		calcElbowInnerAngle();
		setLowerArmRotation(nextLowerArmAngle.Quaternion());
		rotateShoulder();
		//correctElbowRotation();
		if (elbowSettings.calcElbowAngle)
		{
			//if (elbowCorrectionSettings.useFixedElbowWhenNearShoulder)
			//	correctElbowAfterPositioning();
			//if (handSettings.rotateElbowWithHandRight)
			//	rotateElbowWithHandRight();
			//if (handSettings.rotateElbowWithHandForward)
				//rotateElbowWithHandFoward();
			//rotateHand();
		}

		if (left)
		{
			armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + (armTransforms.upperArm.GetRotation().RotateVector(FVector(0.f, -1.f, 0.f) * armTransforms.upperArmLength)));
			armTransforms.lowerArm.SetRotation(lowerArmRotation());
		}
		else
		{
			armTransforms.lowerArm.SetLocation(armTransforms.upperArm.GetLocation() + ((armTransforms.upperArm.GetRotation().GetRightVector()) * armTransforms.upperArmLength));
			armTransforms.lowerArm.SetRotation(lowerArmRotation());
		}
	}

	/*void updateArmAndTurnElbowUp()
	{
		updateUpperArmPosition();
		calcElbowInnerAngle();
		rotateShoulder();
		correctElbowRotation();
	}*/

	void updateUpperArmPosition()
	{
		armTransforms.upperArm = armTransforms.Transform;
		//armTransforms.upperArm.SetLocation(armTransforms.Transform.GetLocation());
	}

	void calcElbowInnerAngle()
	{
		FRotator eulerAngles = FRotator::ZeroRotator;
		float targetShoulderDistance = (target.GetLocation() - upperArmPos()).Size();
		float innerAngle;

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

		eulerAngles.Yaw = innerAngle;
		nextLowerArmAngle = eulerAngles;
	}

	//source: https://github.com/NickHardeman/ofxIKArm/blob/master/src/ofxIKArm.cpp
	void rotateShoulder()
	{
		FRotator eulerAngles = FRotator::ZeroRotator;
		FVector targetShoulderDirection = (target.GetLocation() - upperArmPos()).GetSafeNormal();
		float targetShoulderDistance = (target.GetLocation() - upperArmPos()).Size();

		eulerAngles.Yaw = (left ? -1.f : 1.f) *
			FMath::Acos(FMath::Clamp((FMath::Pow(targetShoulderDistance, 2.f) + FMath::Pow(armTransforms.upperArmLength, 2.f) -
				FMath::Pow(armTransforms.lowerArmLength, 2.f)) / (2.f * targetShoulderDistance * armTransforms.upperArmLength), -1.f, 1.f));
		
		if (FMath::IsNaN(eulerAngles.Yaw))
			eulerAngles.Yaw = 0.f;

		FQuat shoulderRightRotation = FQuat::FindBetweenNormals(armDirection(), targetShoulderDirection);//Quaternion.FromToRotation(armDirection, targetShoulderDirection);

		setUpperArmRotation(shoulderRightRotation);
		armTransforms.lowerArm.SetRotation(FQuat::Identity);
		//setLowerArmLocalRotation(FQuat::Identity);
		setUpperArmRotation(FQuat(lowerArmRotation().GetUpVector(), eulerAngles.Yaw) * armTransforms.upperArm.GetRotation());
		//armTransforms.upperArm.SetRotation(FQuat(lowerArmRotation().GetUpVector(), eulerAngles.Yaw) * armTransforms.upperArm.GetRotation());
		setLowerArmRotation(nextLowerArmAngle.Quaternion());
		//setLowerArmLocalRotation(nextLowerArmAngle.Quaternion()); // CHECK THIS
	}

	void correctElbowRotation()
	{
		FBeforePositioningSettings &s = beforePositioningSettings;
		FVector localTargetPos = shoulderAnker().InverseTransformPosition(target.GetLocation()) / armTransforms.armLength;

		float Devisor = 100.f;
		float elbowOutsideFactor = 		
			FMath::Clamp(
			FMath::Clamp((s.startBelowZ / Devisor - localTargetPos.X / Devisor) / FMath::Abs(s.startBelowZ / Devisor) * .5f, 0.f, 1.f) * FMath::Clamp((localTargetPos.Z / Devisor - s.startAboveY / Devisor) / FMath::Abs(s.startAboveY / Devisor), 0.f, 1.f) * FMath::Clamp(1.f - (localTargetPos.Y / Devisor) * (left ? -1.f : 1.f), 0.f, 1.f)
			, 0.f, 1.f) * s.weight;

		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();
		FVector targetDir = shoulder->Transform.GetRotation() * (FVector::UpVector + (s.correctElbowOutside ? (armDirection() + FVector::ForwardVector * -.2f) * elbowOutsideFactor : FVector::ZeroVector));

		FVector cross = FVector::CrossProduct(shoulderHandDirection, targetDir * 1000.f);//.GetSafeNormal());// *100000.f);
		FVector upperArmUp = upperArmRotation().GetUpVector();

		float elbowTargetUp = FVector::DotProduct(upperArmUp, targetDir);

		float elbowAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(cross, upperArmUp))) + (left ? 0.f : 180.f);
		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(elbowAngle * FMath::Sign(elbowTargetUp)));
		armTransforms.upperArm.SetRotation(rotation * armTransforms.upperArm.GetRotation());
	}

	float getElbowTargetAngle()
	{

		FVector localHandPosNormalized = FVector::ZeroVector;

		localHandPosNormalized = shoulderAnker().InverseTransformPosition(handPos()) / armTransforms.armLength;

		float Devisor = 100.f;

		// angle from Y
		float angle = elbowSettings.yWeight * localHandPosNormalized.Z + elbowSettings.offsetAngle;

		if (localHandPosNormalized.Z > 0.f)
			angle += (elbowSettings.zWeightTop * ((FMath::Max(elbowSettings.zDistanceStart - localHandPosNormalized.X, 0.f) * FMath::Max(localHandPosNormalized.Z, 0.f)) / Devisor));
		else
			angle += (elbowSettings.zWeightBottom * ((FMath::Max(elbowSettings.zDistanceStart - localHandPosNormalized.X, 0.f) * FMath::Max(-localHandPosNormalized.Z, 0.f)) / Devisor));


		// angle from X
		angle += elbowSettings.xWeight * (FMath::Max(localHandPosNormalized.Y * (left ? 1.0f : -1.0f) + elbowSettings.xDistanceStart, 0.f) / Devisor);

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
		FVector localTargetPos = (shoulderAnker().InverseTransformPosition(target.GetLocation()) / armTransforms.armLength) / 100.f;
		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();
		FVector elbowPos = s.localElbowPos;
		
		float Devisor = 100.f;

		if (left)
			elbowPos.Y *= -1.f;

		FVector targetDir = shoulder->Transform.GetRotation() * elbowPos.GetSafeNormal();
		FVector cross = FVector::CrossProduct(shoulderHandDirection, targetDir);

		FVector upperArmUp = upperArmRotation() * FVector::UpVector;

		FVector distance = target.GetLocation() - upperArmPos();
		distance = distance.Size() * shoulder->Transform.InverseTransformVector(distance / distance.Size());

		float weight = FMath::Clamp(FMath::Clamp(((s.startBelowDistance / Devisor) - (distance.Size2D() / Devisor) / (armTransforms.armLength / Devisor)) /
			(s.startBelowDistance / Devisor), 0.f, 1.f) * s.weight + FMath::Clamp((-distance.X / Devisor + .1f) * 3.f, 0.f, 1.f), 0.f, 1.f) *
			FMath::Clamp((s.startBelowY / Devisor - localTargetPos.Z / Devisor) /
				(s.startBelowY / Devisor), 0.f, 1.f);

		// This can be a sub function
		float elbowTargetUp = FVector::DotProduct(upperArmUp, targetDir);
		float elbowAngle2 = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(cross, upperArmUp))) + (left ? 0.f : 180.f);
		//float elbowAngle2 = FMath::RadiansToDegrees(acosf(cross | upperArmUp))  + (left ? 0.f : 180.f);
		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(toSignedEulerAngle((elbowAngle2 * FMath::Sign(elbowTargetUp))) * FMath::Clamp(weight, 0.f, 1.f)));
		armTransforms.upperArm.SetRotation(rotation * armTransforms.upperArm.GetRotation());
	}

	void rotateElbow(float angle)
	{
		FVector shoulderHandDirection = (upperArmPos() - handPos()).GetSafeNormal();

		FQuat rotation = FQuat(shoulderHandDirection, FMath::DegreesToRadians(angle));
		setUpperArmRotation(rotation * upperArmRotation());
	}

	//source: https://github.com/NickHardeman/ofxIKArm/blob/master/src/ofxIKArm.cpp
	void positionElbow()
	{
		float targetElbowAngle = getElbowTargetAngle();
		rotateElbow(targetElbowAngle);
	}


	void rotateElbowWithHandRight()
	{
		FHandSettings s = handSettings;
		FVector handUpVec = target.GetRotation() * FVector::UpVector;
		float forwardAngle = VectorHelpers::getAngleBetween(lowerArmRotation() * FVector::RightVector, target.GetRotation() * FVector::RightVector,
			lowerArmRotation() * FVector::UpVector, lowerArmRotation() * FVector::ForwardVector);

		// todo reduce influence if hand local forward rotation is high (hand tilted inside)
		FQuat handForwardRotation = FQuat(lowerArmRotation() * FVector::ForwardVector, FMath::DegreesToRadians(-forwardAngle));
		handUpVec = handForwardRotation * handUpVec;

		float elbowTargetAngle = VectorHelpers::getAngleBetween(lowerArmRotation() * FVector::UpVector, handUpVec,
			lowerArmRotation() * FVector::ForwardVector, lowerArmRotation() * armDirection());

		float deltaElbow = (elbowTargetAngle + (left ? -s.handDeltaOffset : s.handDeltaOffset)) / 180.f;

		deltaElbow = FMath::Sign(deltaElbow) * FMath::Pow(FMath::Abs(deltaElbow), s.handDeltaPow) * 180.f * s.handDeltaFactor;
		interpolatedDeltaElbow = FRotator::ClampAxis(FMath::Lerp(interpolatedDeltaElbow, deltaElbow, DeltaTime / s.rotateElbowWithHandDelay));
		rotateElbow(interpolatedDeltaElbow);
	}

	void rotateElbowWithHandFoward()
	{
		FHandSettings s = handSettings;
		FVector handRightVec = target.GetRotation() * armDirection();

		float elbowTargetAngleForward = VectorHelpers::getAngleBetween(lowerArmRotation() * armDirection(), handRightVec,
			lowerArmRotation() * FVector::UpVector, lowerArmRotation() * FVector::ForwardVector);

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
		interpolatedDeltaElbowForward = FRotator::ClampAxis(FMath::Lerp(interpolatedDeltaElbowForward, deltaElbowForward, DeltaTime / s.rotateElbowWithHandDelay));

		float signedInterpolated = toSignedEulerAngle(interpolatedDeltaElbowForward);
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

	/*FVector removeShoulderRightRotation(FVector direction)
	{
		FQuat::ToAxisAndAngle()
		return FQuat(shoulder->Transform.GetRotation().GetRightVector(), -shoulderRightRotation) * direction;
	}*/

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

	/*inline FVector lowerArmPos()
	{
		return armTransforms.lowerArm.GetLocation();
	}*/

	inline FVector handPos()
	{
		return left ? CurrentTransforms.LeftHandTransform.GetLocation() : CurrentTransforms.RightHandTransform.GetLocation();
		//return armTransforms.hand.GetLocation();
	}

	/*inline */FTransform shoulderAnker()
	{
		return left ? shoulder->leftShoulderAnchor : shoulder->rightShoulderAnchor;
	}
	
	/*inline */FQuat upperArmRotation()
	{
		return armTransforms.upperArm.GetRotation();// *upperArmStartRotation.Inverse();
	}

	/*inline */FQuat lowerArmRotation()
	{
		return armTransforms.upperArm.GetRotation() * armTransforms.lowerArm.GetRotation();// *lowerArmStartRotation.Inverse();
	}

	/*inline*/ FQuat handRotation()
	{
		return armTransforms.hand.GetRotation();// *handStartRotation.Inverse();
	}

	void setHandRotation(FQuat rotation)
	{
		armTransforms.hand.SetRotation(rotation);// *handStartRotation);
	}

	float toSignedEulerAngle(float InFloat)
	{
		float result = toPositiveEulerAngle(InFloat);
		if (result > 180.f)
			result = result - 360.f;
		return result;
	}

	float toPositiveEulerAngle(float InFloat)
	{
		return FMath::Fmod(FMath::Fmod(InFloat, 360.f) + 360.f, 360.f);
	}
};

/**
*	A custom constraint component subclass that exposes additional missing functionality from the default one
*/
UCLASS(meta = (BlueprintSpawnableComponent), HideCategories = (Activation, "Components|Activation", Physics, Mobility))
class VREXPANSIONPLUGIN_API UVRArmIKActorComponent : public UActorComponent
{
	
	GENERATED_BODY()
public:

	UVRArmIKActorComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		LeftArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
		RightArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
	}

		//	public delegate void OnCalibrateListener();

		//	public event OnCalibrateListener onCalibrate;
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
		//FTransform CameraTransform;
		//FTransform LeftHandTransform;
		//FTransform RightHandTransform;
		
		virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
		{
			Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

			//if (!bCalibrated)
			//	return;

			AVRBaseCharacter * OwningChar = Cast<AVRBaseCharacter>(GetOwner());
			if (!OwningChar)
				return;

			FTransform ToLocalTrans = OwningChar->GetActorTransform().Inverse();

			CurrentTransforms.CameraTransform = OwningChar->VRReplicatedCamera->GetComponentTransform() * ToLocalTrans;
			CurrentTransforms.LeftHandTransform = OwningChar->LeftMotionController->GetComponentTransform() * ToLocalTrans;
			CurrentTransforms.RightHandTransform = OwningChar->RightMotionController->GetComponentTransform() * ToLocalTrans;

			/*CurrentTransforms.CameraTransform = OwningChar->VRReplicatedCamera->GetRelativeTransform();
			CurrentTransforms.LeftHandTransform = OwningChar->LeftMotionController->GetRelativeTransform();
			CurrentTransforms.RightHandTransform = OwningChar->RightMotionController->GetRelativeTransform();*/


			CurrentTransforms.PureCameraYaw = UVRExpansionFunctionLibrary::GetHMDPureYaw_I(CurrentTransforms.CameraTransform.GetRotation().Rotator());

			UpdateShoulders();		
			LeftArm.Update(CurrentTransforms, &shoulder, true, GetWorld()->GetDeltaSeconds());
			RightArm.Update(CurrentTransforms, &shoulder, false, GetWorld()->GetDeltaSeconds());

			// Update Arms
			// Waist estimation?
		}



		// Init
		void Awake()
		{
			if (loadPlayerSizeOnAwake)
			{
				loadPlayerSize();
			}
			//	var device = XRSettings.loadedDeviceName;
			//	vrSystemOffsetHeight = string.IsNullOrEmpty(device) || device == "OpenVR" ? 0 : playerHeightHmd;
		}

		/*void Start()
		{
			onCalibrate += OnCalibrate;
		}*/

		void loadPlayerWidthShoulders()
		{
			playerWidthShoulders = 0.31f;// PlayerPrefs.GetFloat("VRArmIK_PlayerWidthShoulders", 0.31f);
		}

		void savePlayerWidthShoulders(float width)
		{
			//PlayerPrefs.SetFloat("VRArmIK_PlayerWidthShoulders", width);
		}

		UFUNCTION(BlueprintCallable, Category = "BaseVRCharacter|VRLocations")
		void CalibrateIK()
		{
			playerWidthWrist = (CurrentTransforms.LeftHandTransform.GetLocation() - CurrentTransforms.RightHandTransform.GetLocation()).Size();
			playerHeightHmd = CurrentTransforms.CameraTransform.GetLocation().Z;

			// Calibrate shoulders as well from frontal position?
			
			LeftArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
			RightArm.armTransforms.updateArmLengths(playerWidthShoulders, playerWidthWrist);
			//savePlayerSize(playerHeightHmd, playerWidthWrist);
		}

		void savePlayerSize(float heightHmd, float widthWrist)
		{
			//PlayerPrefs.SetFloat("VRArmIK_PlayerHeightHmd", heightHmd);
			//PlayerPrefs.SetFloat("VRArmIK_PlayerWidthWrist", widthWrist);
			loadPlayerSize();
			//onCalibrate ? .Invoke();
		}

		void loadPlayerSize()
		{
			//playerHeightHmd = PlayerPrefs.GetFloat("VRArmIK_PlayerHeightHmd", referencePlayerHeightHmd);
			//playerWidthWrist = PlayerPrefs.GetFloat("VRArmIK_PlayerWidthWrist", referencePlayerWidthWrist);
		}

	//class ShoulderPoser
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

			if (bEnableDistinctShoulderRotation)
			{
				rotateLeftShoulder();
				rotateRightShoulder();
			}

			shoulder.leftShoulderAnchor = FTransform(leftShoulderAnkerStartLocalPosition) * shoulder.Transform;
			shoulder.rightShoulderAnchor = FTransform(rightShoulderAnkerStartLocalPosition) * shoulder.Transform;
			// Update our shoulder anchors now as the shoulder is done computing
			shoulder.leftShoulder = shoulder.leftShoulderAnchor; //
			shoulder.rightShoulder = shoulder.rightShoulderAnchor; //
			//shoulder.leftShoulder.SetLocation(shoulder.leftShoulderAnchor.GetLocation());// = shoulder.leftShoulderAnchor; //
			//shoulder.rightShoulder.SetLocation(shoulder.rightShoulderAnchor.GetLocation());// = shoulder.rightShoulderAnchor; //
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
				//LeftArm.armTransforms.Transform.SetLocation(FVector::ZeroVector);
				//RightArm.armTransforms.Transform.SetLocation(FVector::ZeroVector);
			}

			//Debug.DrawRay(shoulder.transform.position, shoulder.transform.forward);
		}

		void Start()
		{
			/*if (vrTrackingReferences == null)
				vrTrackingReferences = PoseManager.Instance.vrTransforms;

			leftShoulderAnkerStartLocalPosition = shoulder.transform.InverseTransformPoint(shoulder.leftShoulderAnchor.position);
			rightShoulderAnkerStartLocalPosition =
				shoulder.transform.InverseTransformPoint(shoulder.rightShoulderAnchor.position);*/
		}

		void onCalibrate()
		{

			//shoulder.leftShoulderAnchor = shoulder.leftShoulder;
			//shoulder.rightShoulderAnchor = shoulder.rightShoulder;

			/*shoulder.leftArm.setArmLength((avatarTrackingReferences.leftHand.transform.position - shoulder.leftShoulderAnchor.position)
				.magnitude);
			shoulder.rightArm.setArmLength((avatarTrackingReferences.rightHand.transform.position - shoulder.rightShoulderAnchor.position)
				.magnitude);*/
		}

		// Gets the estimated shoulder position based on the head and hands
		//UFUNCTION(BlueprintPure, Category = "BaseVRCharacter|VRLocations")

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

			FQuat newRot = FQuat(FVector::UpVector, FMath::DegreesToRadians(TargetAngle)); // Make directly from up vec and angle instead
			//GetEstShoulderPitch(newRot);

			shoulder.Transform.SetRotation(newRot);
		}

		// Get combined direction angle up
		float GetEstShoulderRotation()
		{
			FVector LeftHand = CurrentTransforms.LeftHandTransform.GetLocation();
			FVector RightHand = CurrentTransforms.RightHandTransform.GetLocation();

			FRotator LeveledRel = CurrentTransforms.PureCameraYaw;
			//shoulder.Transform.GetLocation();
			FVector DistLeft = LeftHand - shoulder.Transform.GetLocation();//(CameraTransform.GetLocation() + LeveledRel.RotateVector(FVector(-8.f, 0.f, 0.f)));
			FVector DistRight = RightHand - shoulder.Transform.GetLocation();//(CameraTransform.GetLocation() + LeveledRel.RotateVector(FVector(-8.f, 0.f, 0.f)));


			/*
						Transform leftHand = avatarTrackingReferences.leftHand.transform, rightHand = avatarTrackingReferences.rightHand.transform;

			Vector3 distanceLeftHand = leftHand.position - shoulder.transform.position,
				distanceRightHand = rightHand.position - shoulder.transform.position;
			*/

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

			shoulderSide.SetRotation((targetAngle * angleSign).Rotation().Quaternion());
		}


		void positionShoulder()
		{

			//FRotator LeveledRel = CurrentTransforms.PureCameraYaw;

			FVector headNeckOffset = CurrentTransforms.CameraTransform.GetRotation().RotateVector(headNeckDirectionVector);
			FVector targetPosition = CurrentTransforms.CameraTransform.GetLocation() + headNeckOffset * headNeckDistance;
			shoulder.Transform.SetLocation(targetPosition + CurrentTransforms.CameraTransform.GetRotation().RotateVector(neckShoulderDistance));
			//shoulder.transform.localPosition =
			//	shoulder.transform.parent.InverseTransformPoint(targetPosition) + neckShoulderDistance;
		}

		// Shoulder yaw
		//virtual void rotateShoulderUp()
		//{


			/*			float angle = getCombinedDirectionAngleUp();

						Vector3 targetRotation = new Vector3(0f, angle, 0f);

						if (autoDetectHandsBehindHead)
						{
							detectHandsBehindHead(ref targetRotation);
						}

						if (clampRotationToHead)
						{
							clampHeadRotationDeltaUp(ref targetRotation);
						}

						shoulder.transform.eulerAngles = targetRotation;*/
		//}


		void positionShoulderRelative()
		{
			FQuat deltaRot = FQuat(shoulder.Transform.GetRotation().GetRightVector(), FMath::DegreesToRadians(shoulderRightRotation));
			FVector shoulderHeadDiff = shoulder.Transform.GetLocation() - CurrentTransforms.CameraTransform.GetLocation();
			shoulder.Transform.SetLocation(deltaRot * shoulderHeadDiff + CurrentTransforms.CameraTransform.GetLocation());

			/*Quaternion deltaRot = Quaternion.AngleAxis(shoulderRightRotation, shoulder.transform.right);
			Vector3 shoulderHeadDiff = shoulder.transform.position - avatarTrackingReferences.head.transform.position;
			shoulder.transform.position = deltaRot * shoulderHeadDiff + avatarTrackingReferences.head.transform.position;*/
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
			float headPitchRotation = VectorHelpers::getAngleBetween(OrigRot.GetForwardVector(), HMDRot.GetForwardVector(), FVector::UpVector, OrigRot.GetRightVector()) + rightRotationHeadRotationOffset;
			//axisAngle(HMDRot.GetForwardVector(), FVector::UpVector, OrigRot.GetRightVector());
			float heightFactor = FMath::Clamp(relativeHeightDiff - 0.f, 0.f, 1.0f);
			shoulderRightRotation = heightFactor * rightRotationHeightFactor;
			shoulderRightRotation += FMath::Clamp(headPitchRotation * rightRotationHeadRotationFactor * heightFactor, 0.f, 50.f);


			FQuat DeltaRot = FQuat(OrigRot.GetRightVector()/*FVector::RightVector*/, FMath::DegreesToRadians(shoulderRightRotation));

			OrigRot = DeltaRot * OrigRot;

			// Positionrelative(), offsets by the rot position
		}
};

UVRArmIKActorComponent::UVRArmIKActorComponent(const FObjectInitializer& ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	PrimaryComponentTick.bTickEvenWhenPaused = true;

	//OwningCharacter = nullptr;
	//vrSystemOffsetHeight = 0.0f;

	referencePlayerHeightHmd = 1.94f * 100.f; // World To Meters idealy
	referencePlayerWidthWrist = /*1.39f*/ 2.f * 100.f; // World To Meters idealy
	playerHeightHmd = 1.94f * 100.f; // World To Meters idealy
	playerWidthWrist = 1.39f/*2.f*/ * 100.f; // World To Meters idealy
	playerWidthShoulders = 0.31f * 100.f; // World To Meters idealy

	leftShoulderAnkerStartLocalPosition = FVector(0.f, -playerWidthShoulders / 2.f, 0.f);
	rightShoulderAnkerStartLocalPosition = FVector(0.f, playerWidthShoulders / 2.f, 0.f);

	loadPlayerSizeOnAwake = false;

	headNeckDistance = 0.03f * 100.f; // World To Meters idealy
	neckShoulderDistance = FVector(-0.02f, 0.f, -.1f) * 100.f; // World To Meters idealy

	distinctShoulderRotationMultiplier = 30.f;
	distinctShoulderRotationLimitForward = 33.f;
	distinctShoulderRotationLimitBackward = 0.f;
	distinctShoulderRotationLimitUpward = 33.f;
	bShoulderDislocated = false;
	bEnableShoulderDislocation = false;
	rightRotationStartHeight = 0.f;
	rightRotationHeightFactor = 142.f;
	rightRotationHeadRotationFactor = 0.3f;
	rightRotationHeadRotationOffset = -20.f;
	startShoulderDislocationBefore = 0.005f * 100.f; // World To Meters idealy
	headNeckDirectionVector = FVector(-.05f, 0.f, -1.f);// *100.f; // World To Meters idealy
	bIgnoreZPos = true;
}