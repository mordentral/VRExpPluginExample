// Fill out your copyright notice in the Description page of Project Settings.

#include "GripScripts/VRGripScriptBase.h"
 
UVRGripScriptBase::UVRGripScriptBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
//	PrimaryComponentTick.bCanEverTick = false;
//	PrimaryComponentTick.bStartWithTickEnabled = false;
//	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PrePhysics;
}


//void UVRGripScriptBase::BeginPlay_Implementation() {}
void UVRGripScriptBase::ModifyWorldTransform_Implementation(float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface) {}
void UVRGripScriptBase::OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) {}
void UVRGripScriptBase::OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) {}

bool UVRGripScriptBase::ModifiesWorldTransform_Implementation() { return bModifiesWorldTransform; }
bool UVRGripScriptBase::IsScriptActive_Implementation() { return bIsActive; }