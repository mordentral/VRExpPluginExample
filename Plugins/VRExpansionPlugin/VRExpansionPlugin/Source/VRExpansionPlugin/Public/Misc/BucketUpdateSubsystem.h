// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EngineSubsystem.h"
#include "Tickable.h"


#include "Grippables/GrippablePhysicsReplication.h"

#include "BucketUpdateSubsystem.generated.h"
//#include "GrippablePhysicsReplication.generated.h"


//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FVRPhysicsReplicationDelegate, void, Return);

/*static TAutoConsoleVariable<int32> CVarEnableCustomVRPhysicsReplication(
	TEXT("vr.VRExpansion.EnableCustomVRPhysicsReplication"),
	0,
	TEXT("Enable valves input controller that overrides legacy input.\n")
	TEXT(" 0: use the engines default input mapping (default), will also be default if vr.SteamVR.EnableVRInput is enabled\n")
	TEXT(" 1: use the valve input controller. You will have to define input bindings for the controllers you want to support."),
	ECVF_ReadOnly);*/


UCLASS()
class VREXPANSIONPLUGIN_API UBucketUpdateSubsystem : public UEngineSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UBucketUpdateSubsystem() :
		Super()
	{

	}

	UPROPERTY()
	FReplicationBucketContainer BucketContainer;

	template<typename classType>
	bool AddObjectToBucket(uint32 UpdateHTZ, classType* InObject, void(classType::* _Func)(/*float*/))
	{
		if (!InObject)
			return false;

		return BucketContainer.AddBucketObject(UpdateHTZ, InObject, _Func);
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Object to Bucket Updates by Function Name", ScriptName = "AddBucketObjectFunction"), Category = "BucketUpdateSubsystem")
	bool AddObjectToBucket(int32 UpdateHTZ, UObject* InObject, FName FunctionName)
	{
		if (!InObject)
			return false;

		return BucketContainer.AddBucketObject(UpdateHTZ, InObject, FunctionName);
	}


	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove Object From Bucket Updates", ScriptName = "RemoveBucketObject"), Category = "BucketUpdateSubsystem")
		bool RemoveObjectFromBucket(UObject* InObject)
	{
		if (!InObject)
			return false;

		return BucketContainer.RemoveBucketObject(InObject);
	}

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Is Object In Bucket", ScriptName = "IsObjectInBucket"), Category = "BucketUpdateSubsystem")
		bool IsObjectInBucket(UObject* InObject)
	{
		if (!InObject)
			return false;

		return BucketContainer.IsObjectInBucket(InObject);
	}

	// Would require a dynamic delegate
	/*UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Bucket Object by Event", ScriptName = "AddBucketObjectEvent"), Category = "BucketUpdateSubsystem")
	bool AddBucketObject(int32 UpdateHTZ, UObject * InObject, UPARAM(DisplayName = "Event") FBucketUpdateTickSignature Delegate)
	{
		if (!InObject)
			return false;

		return BucketContainer.AddBucketObject(UpdateHTZ, InObject, Delegate);
	}*/

	// FTickableGameObject functions
	/**
	 * Function called every frame on this GripScript. Override this function to implement custom logic to be executed every frame.
	 * Only executes if bCanEverTick is true and bAllowTicking is true
	 *
	 * @param DeltaTime - The time since the last tick.
	 */
	virtual void Tick(float DeltaTime) override
	{
		BucketContainer.UpdateBuckets(DeltaTime);
	}

	virtual bool IsTickable() const override
	{
		return BucketContainer.bNeedsUpdate;
	}

	virtual UWorld* GetTickableGameObjectWorld() const override
	{
		return GetWorld();
	}

	virtual bool IsTickableInEditor() const
	{
		return false;
	}

	virtual bool IsTickableWhenPaused() const override
	{
		return false;
	}

	virtual ETickableTickType GetTickableTickType() const
	{
		if (IsTemplate(RF_ClassDefaultObject))
			return ETickableTickType::Never;

		return ETickableTickType::Conditional;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UVRGripScriptBase, STATGROUP_Tickables);
	}

	// End tickable object information

};
