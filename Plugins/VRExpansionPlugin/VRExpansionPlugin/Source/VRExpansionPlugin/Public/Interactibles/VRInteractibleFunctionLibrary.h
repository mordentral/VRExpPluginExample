// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
//#include "IMotionController.h"
#include "VRBPDatatypes.h"
#include "GameplayTagContainer.h"

#if WITH_EDITOR
//#include "Editor/PropertyEditor/Public/IDetailCustomization.h"
#include "Editor/Kismet/Public/BlueprintEditor.h"
#include "Editor/Kismet/Public/BlueprintEditorModes.h"
//#include "Editor/PropertyEditor/Public/PropertyHandle.h"
#include "PropertyHandle.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyRestriction.h"
#include "DetailCategoryBuilder.h"
#endif

#include "VRInteractibleFunctionLibrary.generated.h"

//General Advanced Sessions Log
DECLARE_LOG_CATEGORY_EXTERN(VRInteractibleFunctionLibraryLog, Log, All);

// A data structure to hold important interactible data
// Should be init'd in Beginplay with BeginPlayInit as well as OnGrip with OnGripInit.
// Works in "static space", it records the original relative transform of the interactible on begin play
// so that calculations on the actual component can be done based off of it.
USTRUCT(BlueprintType, Category = "VRExpansionLibrary")
struct VREXPANSIONPLUGIN_API FBPVRInteractibleBaseData
{
	GENERATED_BODY()
public:

	// Our initial relative transform to our parent "static space" - Set in BeginPlayInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FTransform InitialRelativeTransform;

	// Initial location in "static space" of the interactor on grip - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialInteractorLocation;

	// Initial location of the interactible in the "static space" - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialGripLoc;

	// Initial location on the interactible of the grip, used for drop calculations - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FVector InitialDropLocation;

	// The initial transform in relative space of the grip to us - Set in OnGripInit
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "InteractibleData")
	FTransform ReversedRelativeTransform;
};

UCLASS()
class VREXPANSIONPLUGIN_API UVRInteractibleFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	// Get current parent transform
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static FTransform Interactible_GetCurrentParentTransform(USceneComponent * SceneComponentToCheck)
	{
		if (SceneComponentToCheck)
		{
			if (USceneComponent * AttachParent = SceneComponentToCheck->GetAttachParent())
			{
				return AttachParent->GetComponentTransform();
			}
		}
		
		return FTransform::Identity;
	}

	// Get current relative transform (original transform we were at on grip for the current parent transform)
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static FTransform Interactible_GetCurrentRelativeTransform(USceneComponent * SceneComponentToCheck, FBPVRInteractibleBaseData BaseData)
	{
		FTransform ParentTransform = FTransform::Identity;
		if (SceneComponentToCheck)
		{
			if (USceneComponent * AttachParent = SceneComponentToCheck->GetAttachParent())
			{
				// during grip there is no parent so we do this, might as well do it anyway for lerping as well
				ParentTransform = AttachParent->GetComponentTransform();
			}
		}

		return BaseData.InitialRelativeTransform * ParentTransform;
	}

	// Inits the initial relative transform of an interactible on begin play
	UFUNCTION(BlueprintCallable, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
		static void Interactible_BeginPlayInit(USceneComponent * InteractibleComp, UPARAM(ref) FBPVRInteractibleBaseData & BaseDataToInit)
	{
		if (!InteractibleComp)
			return;

		BaseDataToInit.InitialRelativeTransform = InteractibleComp->GetRelativeTransform();
	}

	// Inits the calculated values of a VR Interactible Base Data Structure on a grip event
	UFUNCTION(BlueprintCallable, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
		static void Interactible_OnGripInit(USceneComponent * InteractibleComp, FBPActorGripInformation GripInformation, UPARAM(ref) FBPVRInteractibleBaseData & BaseDataToInit)
	{
		if (!InteractibleComp)
			return;

		BaseDataToInit.ReversedRelativeTransform = FTransform(GripInformation.RelativeTransform.ToInverseMatrixWithScale());
		BaseDataToInit.InitialDropLocation = BaseDataToInit.ReversedRelativeTransform.GetTranslation(); // Technically a duplicate, but will be more clear

		FTransform RelativeToGripTransform = BaseDataToInit.ReversedRelativeTransform * InteractibleComp->GetComponentTransform();
		FTransform CurrentRelativeTransform = BaseDataToInit.InitialRelativeTransform * UVRInteractibleFunctionLibrary::Interactible_GetCurrentParentTransform(InteractibleComp);
		BaseDataToInit.InitialInteractorLocation = CurrentRelativeTransform.InverseTransformPosition(RelativeToGripTransform.GetTranslation());
		
		BaseDataToInit.InitialGripLoc = BaseDataToInit.InitialRelativeTransform.InverseTransformPosition(InteractibleComp->RelativeLocation);
	}

	// Returns (in degrees) the angle around the axis of a location
	// Expects the CurInteractorLocation to be in relative space already
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
	static float Interactible_GetAngleAroundAxis(EVRInteractibleAxis AxisToCalc, FVector CurInteractorLocation)
	{
		float ReturnAxis = 0.0f;

		switch (AxisToCalc)
		{
		case EVRInteractibleAxis::Axis_X:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.Z));
		}break;
		case EVRInteractibleAxis::Axis_Y:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Z, CurInteractorLocation.X));
		}break;
		case EVRInteractibleAxis::Axis_Z:
		{
			ReturnAxis = FMath::RadiansToDegrees(FMath::Atan2(CurInteractorLocation.Y, CurInteractorLocation.X));
		}break;
		default:
		{}break;
		}

		return ReturnAxis;
	}

	// Returns (in degrees) the delta rotation from the initial angle at grip to the current interactor angle around the axis
	// Expects CurInteractorLocation to be in relative space already
	// You can add this to an initial rotation and clamp the result to rotate over time based on hand position
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))	
	static float Interactible_GetAngleAroundAxisDelta(EVRInteractibleAxis AxisToCalc, FVector CurInteractorLocation, float InitialAngle)
	{
		return FRotator::NormalizeAxis(Interactible_GetAngleAroundAxis(AxisToCalc, CurInteractorLocation) - InitialAngle);
	}


	// Returns a value that is snapped to the given settings, taking into account the threshold and increment
	UFUNCTION(BlueprintPure, Category = "VRInteractibleFunctions", meta = (bIgnoreSelf = "true"))
		static float Interactible_GetThresholdSnappedValue(float ValueToSnap, float SnapIncrement, float SnapThreshold)
	{
		if (FMath::Fmod(ValueToSnap, SnapIncrement) <= FMath::Min(SnapIncrement, SnapThreshold))
		{
			return FMath::GridSnap(ValueToSnap, SnapIncrement);
		}

		return ValueToSnap;
	}

};	


UCLASS(Blueprintable, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGripBase : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int OwnerVal;

};

UCLASS(Blueprintable, ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGripBaseChild : public UVRGripBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int ChildVal;

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UVRGripBase2 : public UStaticMeshComponent
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChildActorComponent, meta = (/*OnlyPlaceable, */AllowPrivateAccess = "true", ForceRebuildProperty = "ChildActorTemplate"))
		TSubclassOf<UVRGripBase> ChildActorClass;
		//UVRGripBase * ChildActorClass;
	UPROPERTY(VisibleDefaultsOnly, DuplicateTransient, Category = ChildActorComponent, meta = (ShowInnerProperties))
		UVRGripBase * ChildActorTemplate;
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = ChildActorComponent, meta = (OnlyPlaceable, AllowPrivateAccess = "true", ForceRebuildProperty = "ChildActorTemplate"))
	//	TSubclassOf<UVRGripBase> ChildActorClass;
	//UPROPERTY(VisibleDefaultsOnly, DuplicateTransient, Category = ChildActorComponent, meta = (ShowInnerProperties))
	//UVRGripBase * ChildActorTemplate;
};

#if WITH_EDITOR

/*

truct TStructOpsTypeTraits< FBPAdvGripPhysicsSettings > : public TStructOpsTypeTraitsBase2<FBPAdvGripPhysicsSettings>
{
enum
{
WithNetSerializer = true
};
};


*/


#define LOCTEXT_NAMESPACE "GripDetailCustomization"

template <class CPPSTRUCT>
class FGripDetails : public IDetailCustomization
{
private:
	/** Weak reference to the Blueprint editor */
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	//static TSharedRef<IDetailCustomization> MakeInstance();
	static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<FBlueprintEditor> BlueprintEditorPtrIn)
	{
		return MakeShareable(new FGripDetails(BlueprintEditorPtrIn));
	}

	FGripDetails(TWeakPtr<FBlueprintEditor> BlueprintEditorPtrIn)
		: BlueprintEditorPtr(BlueprintEditorPtrIn)
	{
	}

	/** IDetailCustomization interface */
	//virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		TSharedPtr<IPropertyHandle> ActorClassProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(CPPSTRUCT, ChildActorClass));
		if (ActorClassProperty->IsValidHandle())
		{
			if (BlueprintEditorPtr.IsValid())
			{
				// only restrict for the components view (you can successfully add 
				// a self child component in the execution graphs)
				if (BlueprintEditorPtr.Pin()->GetCurrentMode() == FBlueprintEditorApplicationModes::BlueprintComponentsMode)
				{
					if (UBlueprint* Blueprint = BlueprintEditorPtr.Pin()->GetBlueprintObj())
					{
						FText RestrictReason = LOCTEXT("NoSelfChildActors", "Cannot append a child-actor of this blueprint type (could cause infinite recursion).");
						TSharedPtr<FPropertyRestriction> ClassRestriction = MakeShareable(new FPropertyRestriction(MoveTemp(RestrictReason)));

						ClassRestriction->AddDisabledValue(Blueprint->GetName());
						ClassRestriction->AddDisabledValue(Blueprint->GetPathName());
						if (Blueprint->GeneratedClass)
						{
							ClassRestriction->AddDisabledValue(Blueprint->GeneratedClass->GetName());
							ClassRestriction->AddDisabledValue(Blueprint->GeneratedClass->GetPathName());
						}

						ActorClassProperty->AddRestriction(ClassRestriction.ToSharedRef());
					}
				}
			}

			TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
			DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

			IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory(TEXT("GripSettings"));

			// Ensure ordering is what we want by adding class in first
			CategoryBuilder.AddProperty(GET_MEMBER_NAME_CHECKED(CPPSTRUCT, ChildActorClass));

			IDetailPropertyRow& CATRow = CategoryBuilder.AddProperty(GET_MEMBER_NAME_CHECKED(CPPSTRUCT, ChildActorTemplate));
			CATRow.Visibility(TAttribute<EVisibility>::Create([ObjectsBeingCustomized]()
			{
				for (const TWeakObjectPtr<UObject>& ObjectBeingCustomized : ObjectsBeingCustomized)
				{
					if (CPPSTRUCT* CAC = Cast<CPPSTRUCT>(ObjectBeingCustomized.Get()))
					{
						if (CAC->ChildActorTemplate == nullptr)
						{
							return EVisibility::Hidden;
						}
					}
					else
					{
						return EVisibility::Hidden;
					}
				}

				return EVisibility::Visible;
			}));
		}
	}
};


class UVRGripBase2Details : public FGripDetails<class UVRGripBase2>
{
	/*static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<FBlueprintEditor> BlueprintEditorPtrIn)
	{
		return MakeShareable(new UVRGripBase2Details(BlueprintEditorPtrIn));
	}*/
};

#endif
#undef LOCTEXT_NAMESPACE