// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectIterator.h"

#include "DLibLibrary.generated.h"

UCLASS()
class UDLibLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	// Runs a test function on DLib
	UFUNCTION(BlueprintCallable, Category = "AI|DLib")
	static FString GetTestString();

};	
