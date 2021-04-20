// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_ApplyHandDeltas.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_ModifyBSHand

UAnimGraphNode_ApplyHandDeltas::UAnimGraphNode_ApplyHandDeltas(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

//Title Color!
FLinearColor UAnimGraphNode_ApplyHandDeltas::GetNodeTitleColor() const
{
	return FLinearColor(12, 12, 0, 1);
}

//Node Category
FString UAnimGraphNode_ApplyHandDeltas::GetNodeCategory() const
{
	return FString("Hand Animation");
}
FText UAnimGraphNode_ApplyHandDeltas::GetControllerDescription() const
{
	return FText::FromString("Apply Hand Deltas");
}

FText UAnimGraphNode_ApplyHandDeltas::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText Result = GetControllerDescription();
	return Result;
}