// Fill out your copyright notice in the Description page of Project Settings.


#include "GripScripts/GS_InteractibleSettings.h"

//void UGS_InteractibleSettings::BeginPlay_Implementation() {}
void UGS_InteractibleSettings::GetWorldTransform_Implementation(UGripMotionControllerComponent* OwningController, float DeltaTime, FTransform & WorldTransform, const FTransform &ParentTransform, FBPActorGripInformation &Grip, AActor * actor, UPrimitiveComponent * root, bool bRootHasInterface, bool bActorHasInterface) {}
void UGS_InteractibleSettings::OnGrip_Implementation(UGripMotionControllerComponent * GrippingController, const FBPActorGripInformation & GripInformation) {}
void UGS_InteractibleSettings::OnGripRelease_Implementation(UGripMotionControllerComponent * ReleasingController, const FBPActorGripInformation & GripInformation, bool bWasSocketed) {}