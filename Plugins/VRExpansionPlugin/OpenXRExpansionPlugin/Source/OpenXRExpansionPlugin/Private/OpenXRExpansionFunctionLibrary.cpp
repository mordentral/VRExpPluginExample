// Fill out your copyright notice in the Description page of Project Settings.
#include "OpenXRExpansionFunctionLibrary.h"
//#include "EngineMinimal.h"
#include "Engine/Engine.h"
#include "CoreMinimal.h"
#include "IXRTrackingSystem.h"
#include "IHeadMountedDisplay.h"

//General Log
DEFINE_LOG_CATEGORY(OpenXRExpansionFunctionLibraryLog);

UOpenXRExpansionFunctionLibrary::UOpenXRExpansionFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

//=============================================================================
UOpenXRExpansionFunctionLibrary::~UOpenXRExpansionFunctionLibrary()
{

}
