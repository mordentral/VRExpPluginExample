// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "GripMotionControllerComponent.h"
//#include "MotionControllerComponent.h"
//#include "VRGripInterface.h"
//#include "GameplayTagContainer.h"
//#include "GameplayTagAssetInterface.h"
//#include "VRInteractibleFunctionLibrary.h"
#include "PhysicsEngine/ConstraintInstance.h"


#include "VRGripScriptBase.generated.h"


UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGripScriptBase : public UObject
{
	GENERATED_UCLASS_BODY()

	// Need to add TICK and BeginPlay implementations

	// Implement VRGripInterface so that we can add functionality with it

	// Useful functions to override in c++ for functionality
	//virtual void BeginPlay() override;
	//virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);

};
