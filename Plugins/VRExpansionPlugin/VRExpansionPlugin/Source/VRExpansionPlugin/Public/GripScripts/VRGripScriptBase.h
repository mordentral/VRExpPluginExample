// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "GripMotionControllerComponent.h"
//#include "MotionControllerComponent.h"
//#include "VRGripInterface.h"
//#include "GameplayTagContainer.h"
//#include "GameplayTagAssetInterface.h"
//#include "VRInteractibleFunctionLibrary.h"
//#include "PhysicsEngine/ConstraintInstance.h"


#include "VRGripScriptBase.generated.h"


UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, Abstract, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGripScriptBase : public UObject
{
	GENERATED_BODY()
public:
	UVRGripScriptBase(const FObjectInitializer& ObjectInitializer);
	// Need to add TICK and BeginPlay implementations

	// Returns if the script is currently active and should be used
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripScript")
		bool IsScriptActive();
	virtual bool IsScriptActive_Implementation();

	// Is currently active helper variable, normally returned from IsScriptActive()
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	bool bIsActive;

	// Returns if the script is going to override or modify the world transform of the grip
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripScript")
		bool ModifiesWorldTransform();
		virtual bool ModifiesWorldTransform_Implementation();

	// Is currently active helper variable, normally returned from IsScriptActive()
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
		bool bModifiesWorldTransform;


	// Implement VRGripInterface so that we can add functionality with it

	// Useful functions to override in c++ for functionality
	//virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);

	// Not all scripts will require this function, specific ones that use things like Lever logic however will. Best to call it.
	//UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripScript|Init")
		//void BeginPlay();
		//virtual void BeginPlay_Implementation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "VRGripScript|Steps")
		void ModifyWorldTransform(float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface);
		virtual void ModifyWorldTransform_Implementation(float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface);

	// Event triggered on the interfaced object when gripped
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripScript")
		void OnGrip(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);
		virtual void OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation);

	// Event triggered on the interfaced object when grip is released
	UFUNCTION(BlueprintNativeEvent, Category = "VRGripScript")
		void OnGripRelease(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed = false);
		virtual void OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed = false);

};
