// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "VRExpansionEditor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Grippables/HandSocketComponent.h"
#include "HandSocketVisualizer.h"


IMPLEMENT_MODULE(FVRExpansionEditorModule, VRExpansionEditor);

void FVRExpansionEditorModule::StartupModule()
{
	RegisterComponentVisualizer(UHandSocketComponent::StaticClass()->GetFName(), MakeShareable(new FHandSocketVisualizer));
}

void FVRExpansionEditorModule::ShutdownModule()
{
	if (GUnrealEd != NULL)
	{
		// Iterate over all class names we registered for
		for (FName ClassName : RegisteredComponentClassNames)
		{
			GUnrealEd->UnregisterComponentVisualizer(ClassName);
		}
	}
}

void FVRExpansionEditorModule::RegisterComponentVisualizer(FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
{
	if (GUnrealEd != NULL)
	{
		GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
	}

	RegisteredComponentClassNames.Add(ComponentClassName);

	if (Visualizer.IsValid())
	{
		Visualizer->OnRegister();
	}
}
