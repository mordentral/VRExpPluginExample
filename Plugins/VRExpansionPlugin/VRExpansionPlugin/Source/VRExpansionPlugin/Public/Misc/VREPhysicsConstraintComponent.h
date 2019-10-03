#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

// Delete this eventually when the physics interface is fixed
#if WITH_PHYSX
#include "PhysXPublic.h"
#endif // WITH_PHYSX

#include "VREPhysicsConstraintComponent.generated.h"

/**
*	A custom constraint component subclass that exposes additional missing functionality from the default one
*/
UCLASS(ClassGroup = Physics, meta = (BlueprintSpawnableComponent), HideCategories = (Activation, "Components|Activation", Physics, Mobility), ShowCategories = ("Physics|Components|PhysicsConstraint", "VRE Constraint Settings"))
class VREXPANSIONPLUGIN_API UVREPhysicsConstraintComponent : public UPhysicsConstraintComponent
{
	
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "VRE Physics Constraint Component")
		void SetConstraintToForceBased(bool bUseForceConstraint)
	{

		// This is a temp workaround until epic fixes the drive creation to allow force constraints
		// I wanted to use the new interface and not directly set the drive so that it is ready to delete this section
		// When its fixed
		//#if WITH_PHYSX
			PxD6JointDrive driveVal = ConstraintInstance.ConstraintHandle.ConstraintData->getDrive(PxD6Drive::Enum::eX);
			driveVal.flags = PxD6JointDriveFlags();
			if(!bUseForceConstraint) 
				driveVal.flags &= ~PxD6JointDriveFlag::eACCELERATION;
			ConstraintInstance.ConstraintHandle.ConstraintData->setDrive(PxD6Drive::Enum::eX, driveVal);

			driveVal = ConstraintInstance.ConstraintHandle.ConstraintData->getDrive(PxD6Drive::Enum::eY);
			driveVal.flags = PxD6JointDriveFlags();
			if (!bUseForceConstraint)
				driveVal.flags &= ~PxD6JointDriveFlag::eACCELERATION;
			ConstraintInstance.ConstraintHandle.ConstraintData->setDrive(PxD6Drive::Enum::eY, driveVal);

			driveVal = ConstraintInstance.ConstraintHandle.ConstraintData->getDrive(PxD6Drive::Enum::eZ);
			driveVal.flags = PxD6JointDriveFlags();
			if (!bUseForceConstraint)
				driveVal.flags &= ~PxD6JointDriveFlag::eACCELERATION;
			ConstraintInstance.ConstraintHandle.ConstraintData->setDrive(PxD6Drive::Enum::eZ, driveVal);

			// Check if slerp
			driveVal = ConstraintInstance.ConstraintHandle.ConstraintData->getDrive(PxD6Drive::Enum::eSLERP);
			driveVal.flags = PxD6JointDriveFlags();
			if (!bUseForceConstraint)
				driveVal.flags &= ~PxD6JointDriveFlag::eACCELERATION;
			ConstraintInstance.ConstraintHandle.ConstraintData->setDrive(PxD6Drive::Enum::eSLERP, driveVal);
		//#endif
	}


	UFUNCTION(BlueprintCallable, Category = "VRE Physics Constraint Component")
	void GetConstraintReferenceFrame(EConstraintFrame::Type Frame, FTransform& RefFrame)
	{
		RefFrame = ConstraintInstance.GetRefFrame(Frame);
	}

	// Gets the angular offset on the constraint
	UFUNCTION(BlueprintCallable, Category = "VRE Physics Constraint Component")
	FRotator GetAngularOffset()
	{
		return ConstraintInstance.AngularRotationOffset;
	}

	// Sets the angular offset on the constraint and re-initializes it
	UFUNCTION(BlueprintCallable, Category="VRE Physics Constraint Component")
	void SetAngularOffset(FRotator NewAngularOffset)
	{
		FVector RefPos = ConstraintInstance.Pos2;
		const float RefScale = FMath::Max(GetConstraintScale(), 0.01f);
		if (GetBodyInstance(EConstraintFrame::Frame2))
		{
			RefPos *= RefScale;
		}

		FQuat DeltaRot = ConstraintInstance.AngularRotationOffset.Quaternion().Inverse() * NewAngularOffset.Quaternion();
		DeltaRot.Normalize();

		ConstraintInstance.AngularRotationOffset = NewAngularOffset;

		ConstraintInstance.PriAxis2 = DeltaRot.RotateVector(ConstraintInstance.PriAxis2);
		ConstraintInstance.SecAxis2 = DeltaRot.RotateVector(ConstraintInstance.SecAxis2);

		FPhysicsInterface::ExecuteOnUnbrokenConstraintReadWrite(ConstraintInstance.ConstraintHandle, [&](const FPhysicsConstraintHandle& InUnbrokenConstraint)
			{
				FTransform URefTransform = FTransform(ConstraintInstance.PriAxis2, ConstraintInstance.SecAxis2, ConstraintInstance.PriAxis2 ^ ConstraintInstance.SecAxis2, RefPos);
				FPhysicsInterface::SetLocalPose(InUnbrokenConstraint, URefTransform, EConstraintFrame::Frame2);
			});

		return;
	}

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRE Constraint Settings")
	//	bool bUseForceConstraint;
};