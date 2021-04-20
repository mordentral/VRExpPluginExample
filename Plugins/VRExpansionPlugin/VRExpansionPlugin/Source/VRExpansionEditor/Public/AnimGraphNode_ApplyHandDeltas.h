#pragma once

#include "AnimGraphDefinitions.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Editor/AnimGraph/Classes/AnimGraphNode_Base.h"
#include "Grippables/AnimNode_ApplyHandDeltas.h"
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_ApplyHandDeltas.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_ApplyHandDeltas : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_ApplyHandDeltas Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetNodeCategory() const override;
	// End of UEdGraphNode interface

protected:

	// UAnimGraphNode_SkeletalControlBase protected interface
	virtual FText GetControllerDescription() const;
	//virtual const UAnimGraphNode_Base* GetNode() const override { return &Node; }
	// End of UAnimGraphNode_SkeletalControlBase protected interface

};