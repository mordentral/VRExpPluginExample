// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GripMotionControllerComponent.h"
#include "Engine/Engine.h"
#include "VRBPDatatypes.h"
#include "VRGripInterface.h"
#include "VRExpansionFunctionLibrary.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "GripScripts/VRGripScriptBase.h"
#include "Engine/ActorChannel.h"
#include "DrawDebugHelpers.h"
#include "Grippables/GrippablePhysicsReplication.h"
#include "Grippables/GrippableDataTypes.h"
#include "Misc/BucketUpdateSubsystem.h"
#include "GrippableSkeletalMeshActor.generated.h"

class UOptionalRepSkeletalMeshComponent;

/**
* A component specifically for being able to turn off movement replication in the component at will
* Has the upside of also being a blueprintable base since UE4 doesn't allow that with std ones
*/

USTRUCT()
struct FSkeletalMeshComponentEndPhysicsTickFunctionVR : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	UOptionalRepSkeletalMeshComponent* TargetVR;

	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override;

	virtual FString DiagnosticMessage() override;

	virtual FName DiagnosticContext(bool bDetailed) override;

};
template<>
struct TStructOpsTypeTraits<FSkeletalMeshComponentEndPhysicsTickFunctionVR> : public TStructOpsTypeTraitsBase2<FSkeletalMeshComponentEndPhysicsTickFunctionVR>
{
	enum
	{
		WithCopy = false
	};
};

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, ChildCanTick), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API UOptionalRepSkeletalMeshComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:
	UOptionalRepSkeletalMeshComponent(const FObjectInitializer& ObjectInitializer);

public:

	// Overrides the default of : true and allows for controlling it like in an actor, should be default of off normally with grippable components
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Component Replication")
		bool bReplicateMovement;

	FSkeletalMeshComponentEndPhysicsTickFunctionVR EndPhysicsTickFunctionVR;

	friend struct FSkeletalMeshComponentEndPhysicsTickFunctionVR;

	void EndPhysicsTickComponentVR(FSkeletalMeshComponentEndPhysicsTickFunctionVR& ThisTickFunction)
	{
		//IMPORTANT!
		//
		// The decision on whether to use EndPhysicsTickComponent or not is made by ShouldRunEndPhysicsTick()
		// Any changes that are made to EndPhysicsTickComponent that affect whether it should be run or not
		// have to be reflected in ShouldRunEndPhysicsTick() as well

		// if physics is disabled on dedicated server, no reason to be here. 
		if (!bEnablePhysicsOnDedicatedServer && IsRunningDedicatedServer())
		{
			FinalizeBoneTransform();
			return;
		}

		if (IsRegistered() && IsSimulatingPhysics() && RigidBodyIsAwake())
		{
			if (bNotifySyncComponentToRBPhysics)
			{
				OnSyncComponentToRBPhysics();
			}

			SyncComponentToRBPhysics();
		}

		// this used to not run if not rendered, but that causes issues such as bounds not updated
		// causing it to not rendered, at the end, I think we should blend body positions
		// for example if you're only simulating, this has to happen all the time
		// whether looking at it or not, otherwise
		// @todo better solution is to check if it has moved by changing SyncComponentToRBPhysics to return true if anything modified
		// and run this if that is true or rendered
		// that will at least reduce the chance of mismatch
		// generally if you move your actor position, this has to happen to approximately match their bounds
		if (ShouldBlendPhysicsBones())
		{
			if (IsRegistered())
			{
				BlendInPhysicsInternalVR(ThisTickFunction);
			}
		}
	}

	void BlendInPhysicsInternalVR(FTickFunction& ThisTickFunction)
	{
		check(IsInGameThread());

		// Can't do anything without a SkeletalMesh
		if (!SkeletalMesh)
		{
			return;
		}

		// We now have all the animations blended together and final relative transforms for each bone.
		// If we don't have or want any physics, we do nothing.
		if (Bodies.Num() > 0 && CollisionEnabledHasPhysics(GetCollisionEnabled()))
		{
			//HandleExistingParallelEvaluationTask(/*bBlockOnTask = */ true, /*bPerformPostAnimEvaluation =*/ true);
			// start parallel work
			//check(!IsValidRef(ParallelAnimationEvaluationTask));

			const bool bParallelBlend = false;// !!CVarUseParallelBlendPhysics.GetValueOnGameThread() && FApp::ShouldUseThreadingForPerformance();
			if (bParallelBlend)
			{
				/*SwapEvaluationContextBuffers();

				ParallelAnimationEvaluationTask = TGraphTask<FParallelBlendPhysicsTask>::CreateTask().ConstructAndDispatchWhenReady(this);

				// set up a task to run on the game thread to accept the results
				FGraphEventArray Prerequistes;
				Prerequistes.Add(ParallelAnimationEvaluationTask);

				check(!IsValidRef(ParallelBlendPhysicsCompletionTask));
				ParallelBlendPhysicsCompletionTask = TGraphTask<FParallelBlendPhysicsCompletionTask>::CreateTask(&Prerequistes).ConstructAndDispatchWhenReady(this);

				ThisTickFunction.GetCompletionHandle()->DontCompleteUntil(ParallelBlendPhysicsCompletionTask);*/
			}
			else
			{
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
					PerformBlendPhysicsBonesVR(RequiredBones, BoneSpaceTransforms);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
					FinalizeAnimationUpdateVR();
			}
		}
	}

	void FinalizeAnimationUpdateVR()
	{
		//SCOPE_CYCLE_COUNTER(STAT_FinalizeAnimationUpdate);

		// Flip bone buffer and send 'post anim' notification
		FinalizeBoneTransform();

		if (!bSimulationUpdatesChildTransforms || !IsSimulatingPhysics())	//If we simulate physics the call to MoveComponent already updates the children transforms. If we are confident that animation will not be needed this can be skipped. TODO: this should be handled at the scene component layer
		{
			//SCOPE_CYCLE_COUNTER(STAT_FinalizeAnimationUpdate_UpdateChildTransforms);

			// Update Child Transform - The above function changes bone transform, so will need to update child transform
			// But only children attached to us via a socket.
			UpdateChildTransforms(EUpdateTransformFlags::OnlyUpdateIfUsingSocket);
		}

		if (bUpdateOverlapsOnAnimationFinalize)
		{
			//SCOPE_CYCLE_COUNTER(STAT_FinalizeAnimationUpdate_UpdateOverlaps);

			// animation often change overlap. 
			UpdateOverlaps();
		}

		// update bounds
		/*if (bSkipBoundsUpdateWhenInterpolating)
		{
			if (AnimEvaluationContext.bDoEvaluation)
			{
				//SCOPE_CYCLE_COUNTER(STAT_FinalizeAnimationUpdate_UpdateBounds);
				// Cached local bounds are now out of date
				InvalidateCachedBounds();

				UpdateBounds();
			}
		}
		else*/
		{
			//SCOPE_CYCLE_COUNTER(STAT_FinalizeAnimationUpdate_UpdateBounds);
			// Cached local bounds are now out of date
			InvalidateCachedBounds();

			UpdateBounds();
		}

		// Need to send new bounds to 
		MarkRenderTransformDirty();

		// New bone positions need to be sent to render thread
		MarkRenderDynamicDataDirty();

		// If we have any Slave Components, they need to be refreshed as well.
		RefreshSlaveComponents();
	}

	struct FAssetWorldBoneTM
	{
		FTransform	TM;			// Should never contain scaling.
		bool bUpToDate;			// If this equals PhysAssetUpdateNum, then the matrix is up to date.
	};

	typedef TArray<FAssetWorldBoneTM, TMemStackAllocator<alignof(FAssetWorldBoneTM)>> TAssetWorldBoneTMArray;

	void UpdateWorldBoneTM(TAssetWorldBoneTMArray& WorldBoneTMs, const TArray<FTransform>& InBoneSpaceTransforms, int32 BoneIndex, USkeletalMeshComponent* SkelComp, const FTransform& LocalToWorldTM, const FVector& Scale3D)
	{
		// If its already up to date - do nothing
		if (WorldBoneTMs[BoneIndex].bUpToDate)
		{
			return;
		}

		FTransform ParentTM, RelTM;
		if (BoneIndex == 0)
		{
			// If this is the root bone, we use the mesh component LocalToWorld as the parent transform.
			ParentTM = LocalToWorldTM;
		}
		else
		{
			// If not root, use our cached world-space bone transforms.
			int32 ParentIndex = SkelComp->SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
			UpdateWorldBoneTM(WorldBoneTMs, InBoneSpaceTransforms, ParentIndex, SkelComp, LocalToWorldTM, Scale3D);
			ParentTM = WorldBoneTMs[ParentIndex].TM;
		}

		if (InBoneSpaceTransforms.IsValidIndex(BoneIndex))
		{
			RelTM = InBoneSpaceTransforms[BoneIndex];
			RelTM.ScaleTranslation(Scale3D);

			WorldBoneTMs[BoneIndex].TM = RelTM * ParentTM;
			WorldBoneTMs[BoneIndex].bUpToDate = true;
		}
	}

	void PerformBlendPhysicsBonesVR(const TArray<FBoneIndexType>& InRequiredBones, TArray<FTransform>& InBoneSpaceTransforms)
	{
		//SCOPE_CYCLE_COUNTER(STAT_BlendInPhysics);
		// Get drawscale from Owner (if there is one)
		FVector TotalScale3D = GetComponentTransform().GetScale3D();
		FVector RecipScale3D = TotalScale3D.Reciprocal();

		UPhysicsAsset* const PhysicsAsset = GetPhysicsAsset();
		check(PhysicsAsset);

		if (GetNumComponentSpaceTransforms() == 0)
		{
			return;
		}

		// Get the scene, and do nothing if we can't get one.
		FPhysScene* PhysScene = nullptr;
		if (GetWorld() != nullptr)
		{
			PhysScene = GetWorld()->GetPhysicsScene();
		}

		if (PhysScene == nullptr)
		{
			return;
		}

		FMemMark Mark(FMemStack::Get());
		// Make sure scratch space is big enough.
		TAssetWorldBoneTMArray WorldBoneTMs;
		WorldBoneTMs.AddZeroed(GetNumComponentSpaceTransforms());

		FTransform LocalToWorldTM = GetComponentTransform();
		LocalToWorldTM.SetScale3D(LocalToWorldTM.GetScale3D().GetSignVector());
		LocalToWorldTM.NormalizeRotation();
		//LocalToWorldTM.RemoveScaling();

		TArray<FTransform>& EditableComponentSpaceTransforms = GetEditableComponentSpaceTransforms();

		struct FBodyTMPair
		{
			FBodyInstance* BI;
			FTransform TM;
		};

		FPhysicsCommand::ExecuteRead(this, [&]()
			{
				bool bSetParentScale = false;
				const bool bSimulatedRootBody = Bodies.IsValidIndex(RootBodyData.BodyIndex) && Bodies[RootBodyData.BodyIndex]->IsInstanceSimulatingPhysics();
				const FTransform NewComponentToWorld = bSimulatedRootBody ? GetComponentTransformFromBodyInstance(Bodies[RootBodyData.BodyIndex]) : FTransform::Identity;

				// For each bone - see if we need to provide some data for it.
				for (int32 i = 0; i < InRequiredBones.Num(); i++)
				{
					int32 BoneIndex = InRequiredBones[i];

					// See if this is a physics bone..
					int32 BodyIndex = PhysicsAsset->FindBodyIndex(SkeletalMesh->RefSkeleton.GetBoneName(BoneIndex));
					// need to update back to physX so that physX knows where it was after blending
					FBodyInstance* PhysicsAssetBodyInstance = nullptr;

					// If so - get its world space matrix and its parents world space matrix and calc relative atom.
					if (BodyIndex != INDEX_NONE)
					{
						PhysicsAssetBodyInstance = Bodies[BodyIndex];

						//if simulated body copy back and blend with animation
						if (PhysicsAssetBodyInstance->IsInstanceSimulatingPhysics())
						{
							FTransform PhysTM = PhysicsAssetBodyInstance->GetUnrealWorldTransform_AssumesLocked();

							// Store this world-space transform in cache.
							WorldBoneTMs[BoneIndex].TM = PhysTM;
							WorldBoneTMs[BoneIndex].bUpToDate = true;

							float UsePhysWeight = (bBlendPhysics) ? 1.f : PhysicsAssetBodyInstance->PhysicsBlendWeight;

							// Find this bones parent matrix.
							FTransform ParentWorldTM;

							// if we wan't 'full weight' we just find 
							if (UsePhysWeight > 0.f)
							{
								if (!(ensure(InBoneSpaceTransforms.Num())))
								{
									continue;
								}

								if (BoneIndex == 0)
								{
									ParentWorldTM = LocalToWorldTM;
								}
								else
								{
									// If not root, get parent TM from cache (making sure its up-to-date).
									int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
									UpdateWorldBoneTM(WorldBoneTMs, InBoneSpaceTransforms, ParentIndex, this, LocalToWorldTM, TotalScale3D);
									ParentWorldTM = WorldBoneTMs[ParentIndex].TM;
								}


								// Then calc rel TM and convert to atom.
								FTransform RelTM = PhysTM.GetRelativeTransform(ParentWorldTM);
								RelTM.RemoveScaling();
								FQuat RelRot(RelTM.GetRotation());
								FVector RelPos = RecipScale3D * RelTM.GetLocation();
								FTransform PhysAtom = FTransform(RelRot, RelPos, InBoneSpaceTransforms[BoneIndex].GetScale3D());

								// Now blend in this atom. See if we are forcing this bone to always be blended in
								InBoneSpaceTransforms[BoneIndex].Blend(InBoneSpaceTransforms[BoneIndex], PhysAtom, UsePhysWeight);

								if (!bSetParentScale)
								{
									//We must update RecipScale3D based on the atom scale of the root
									TotalScale3D *= InBoneSpaceTransforms[0].GetScale3D();
									RecipScale3D = TotalScale3D.Reciprocal();
									bSetParentScale = true;
								}

							}
						}
					}

					if (!(ensure(BoneIndex < EditableComponentSpaceTransforms.Num())))
					{
						continue;
					}

					// Update SpaceBases entry for this bone now
					if (BoneIndex == 0)
					{
						if (!(ensure(InBoneSpaceTransforms.Num())))
						{
							continue;
						}
						EditableComponentSpaceTransforms[0] = InBoneSpaceTransforms[0];
					}
					else
					{
						if (bLocalSpaceKinematics || BodyIndex == INDEX_NONE || Bodies[BodyIndex]->IsInstanceSimulatingPhysics())
						{
							if (!(ensure(BoneIndex < InBoneSpaceTransforms.Num())))
							{
								continue;
							}
							const int32 ParentIndex = SkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex);
							EditableComponentSpaceTransforms[BoneIndex] = InBoneSpaceTransforms[BoneIndex] * EditableComponentSpaceTransforms[ParentIndex];

							/**
							* Normalize rotations.
							* We want to remove any loss of precision due to accumulation of error.
							* i.e. A componentSpace transform is the accumulation of all of its local space parents. The further down the chain, the greater the error.
							* SpaceBases are used by external systems, we feed this to PhysX, send this to gameplay through bone and socket queries, etc.
							* So this is a good place to make sure all transforms are normalized.
							*/
							EditableComponentSpaceTransforms[BoneIndex].NormalizeRotation();
						}
						else if (bSimulatedRootBody)
						{
							EditableComponentSpaceTransforms[BoneIndex] = Bodies[BodyIndex]->GetUnrealWorldTransform_AssumesLocked().GetRelativeTransform(NewComponentToWorld);
						}
					}
				}
			});	//end scope for read lock

	}

	virtual void RegisterEndPhysicsTick(bool bRegister) override 
	{
		if (bRegister != EndPhysicsTickFunctionVR.IsTickFunctionRegistered())
		{
			if (bRegister)
			{
				if (SetupActorComponentTickFunction(&EndPhysicsTickFunctionVR))
				{
					EndPhysicsTickFunctionVR.TargetVR = this;
					// Make sure our EndPhysicsTick gets called after physics simulation is finished
					UWorld* World = GetWorld();
					if (World != nullptr)
					{
						EndPhysicsTickFunctionVR.AddPrerequisite(World, World->EndPhysicsTickFunction);
					}
				}
			}
			else
			{
				EndPhysicsTickFunctionVR.UnRegisterTickFunction();
			}
		}
	}

	/*virtual void RegisterClothTick(bool bRegister) override
	{
		if (bRegister != ClothTickFunction.IsTickFunctionRegistered())
		{
			if (bRegister)
			{
				if (SetupActorComponentTickFunction(&ClothTickFunction))
				{
					ClothTickFunction.Target = this;
					ClothTickFunction.AddPrerequisite(this, PrimaryComponentTick);
					ClothTickFunction.AddPrerequisite(this, EndPhysicsTickFunctionVR);	//If this tick function is running it means that we are doing physics blending so we should wait for its results
				}
			}
			else
			{
				ClothTickFunction.UnRegisterTickFunction();
			}
		}
	}*/

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		//CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Animation);

		bool bShouldRunPhysTick = (bEnablePhysicsOnDedicatedServer || !IsNetMode(NM_DedicatedServer)) && // Early out if we are on a dedicated server and not running physics.
			((IsSimulatingPhysics() && RigidBodyIsAwake()) || ShouldBlendPhysicsBones());

		RegisterEndPhysicsTick(PrimaryComponentTick.IsTickFunctionRegistered() && bShouldRunPhysTick);
		//UpdateEndPhysicsTickRegisteredState();
		RegisterClothTick(PrimaryComponentTick.IsTickFunctionRegistered() && ShouldRunClothTick());
		//UpdateClothTickRegisteredState();
		

		// If we are suspended, we will not simulate clothing, but as clothing is simulated in local space
		// relative to a root bone we need to extract simulation positions as this bone could be animated.
/*		if (bClothingSimulationSuspended && this->GetClothingSimulation() && this->GetClothingSimulation()->ShouldSimulate())
		{
			//CSV_SCOPED_TIMING_STAT(Animation, Cloth);

			this->GetClothingSimulation()->GetSimulationData(CurrentSimulationData, this, Cast<USkeletalMeshComponent>(MasterPoseComponent.Get()));
		}*/

		Super::Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

		PendingRadialForces.Reset();

		// Update bOldForceRefPose
		bOldForceRefPose = bForceRefpose;

		/** Update the end group and tick priority */
		//const bool bDoLateEnd = CVarAnimationDelaysEndGroup.GetValueOnGameThread() > 0;
		const bool bRequiresPhysics = EndPhysicsTickFunctionVR.IsTickFunctionRegistered();
		//const ETickingGroup EndTickGroup = bDoLateEnd && !bRequiresPhysics ? TG_PostPhysics : TG_PrePhysics;
		if (ThisTickFunction)
		{
			ThisTickFunction->EndTickGroup = TG_PostPhysics;// EndTickGroup;

			// Note that if animation is so long that we are blocked in EndPhysics we may want to reduce the priority. However, there is a risk that this function will not go wide early enough.
			// This requires profiling and is very game dependent so cvar for now makes sense
			/*bool bDoHiPri = CVarHiPriSkinnedMeshesTicks.GetValueOnGameThread() > 0;
			if (ThisTickFunction->bHighPriority != bDoHiPri)
			{
				ThisTickFunction->SetPriorityIncludingPrerequisites(bDoHiPri);
			}*/
		}

		// If we are waiting for ParallelEval to complete or if we require Physics, 
		// then FinalizeBoneTransform will be called and Anim events will be dispatched there. 
		// We prefer doing it there so these events are triggered once we have a new updated pose.
		// Note that it's possible that FinalizeBoneTransform has already been called here if not using ParallelUpdate.
		// or it's possible that it hasn't been called at all if we're skipping Evaluate due to not being visible.
		// ConditionallyDispatchQueuedAnimEvents will catch that and only Dispatch events if not already done.
		if (!IsRunningParallelEvaluation() && !bRequiresPhysics)
		{
			/////////////////////////////////////////////////////////////////////////////
			// Notify / Event Handling!
			// This can do anything to our component (including destroy it) 
			// Any code added after this point needs to take that into account
			/////////////////////////////////////////////////////////////////////////////

			ConditionallyDispatchQueuedAnimEvents();
		}
	}

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

};

/**
*
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, ChildCanTick), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API AOptionalRepGrippableSkeletalMeshActor : public ASkeletalMeshActor
{
	GENERATED_BODY()

public:
	AOptionalRepGrippableSkeletalMeshActor(const FObjectInitializer& ObjectInitializer);

	// Skips the attachment replication
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Replication")
		bool bIgnoreAttachmentReplication;

	// Skips the physics replication
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Replication")
		bool bIgnorePhysicsReplication;

	// Fix bugs with replication and bReplicateMovement
	virtual void OnRep_ReplicateMovement() override;
	virtual void PostNetReceivePhysicState() override;
};

/**
*
*/
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent, ChildCanTick), ClassGroup = (VRExpansionPlugin))
class VREXPANSIONPLUGIN_API AGrippableSkeletalMeshActor : public ASkeletalMeshActor, public IVRGripInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AGrippableSkeletalMeshActor(const FObjectInitializer& ObjectInitializer);


	~AGrippableSkeletalMeshActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_AttachmentReplication)
		FRepAttachmentWithWeld AttachmentWeldReplication;

	virtual void GatherCurrentMovement() override;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Instanced, Category = "VRGripInterface")
		TArray<class UVRGripScriptBase *> GripLogicScripts;

	bool ReplicateSubobjects(UActorChannel* Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;

	// Sets the Deny Gripping variable on the FBPInterfaceSettings struct
	UFUNCTION(BlueprintCallable, Category = "VRGripInterface")
		void SetDenyGripping(bool bDenyGripping);

	// Sets the grip priority on the FBPInterfaceSettings struct
	UFUNCTION(BlueprintCallable, Category = "VRGripInterface")
		void SetGripPriority(int NewGripPriority);

	// Called when a object is gripped
	// If you override the OnGrip event then you will need to call the parent implementation or this event will not fire!!
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnGripSignature OnGripped;

	// Called when a object is dropped
	// If you override the OnGrip event then you will need to call the parent implementation or this event will not fire!!
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnDropSignature OnDropped;

	// Called when an object we hold is secondary gripped
	// If you override the OnGrip event then you will need to call the parent implementation or this event will not fire!!
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnGripSignature OnSecondaryGripAdded;

	// Called when an object we hold is secondary dropped
	// If you override the OnGrip event then you will need to call the parent implementation or this event will not fire!!
	UPROPERTY(BlueprintAssignable, Category = "Grip Events")
		FVROnGripSignature OnSecondaryGripRemoved;

	// ------------------------------------------------
	// Client Auth Throwing Data and functions 
	// ------------------------------------------------

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Replication")
		FVRClientAuthReplicationData ClientAuthReplicationData;

	// Add this to client side physics replication (until coming to rest or timeout period is hit)
	UFUNCTION(BlueprintCallable, Category = "Networking")
		bool AddToClientReplicationBucket();

	// Remove this from client side physics replication
	UFUNCTION(BlueprintCallable, Category = "Networking")
		bool RemoveFromClientReplicationBucket();

	UFUNCTION()
	bool PollReplicationEvent();

	UFUNCTION(Category = "Networking")
		void CeaseReplicationBlocking();

	// Notify the server that we are no longer trying to run the throwing auth
	UFUNCTION(Reliable, Server, WithValidation, Category = "Networking")
		void Server_EndClientAuthReplication();

	// Notify the server about a new movement rep
	UFUNCTION(UnReliable, Server, WithValidation, Category = "Networking")
		void Server_GetClientAuthReplication(const FRepMovementVR& newMovement);

	// Returns if this object is currently client auth throwing
	UFUNCTION(BlueprintPure, Category = "Networking")
		FORCEINLINE bool IsCurrentlyClientAuthThrowing()
	{
		return ClientAuthReplicationData.bIsCurrentlyClientAuth;
	}

	// End client auth throwing data and functions //

	// ------------------------------------------------
	// Gameplay tag interface
	// ------------------------------------------------

	/** Overridden to return requirements tags */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = GameplayTags;
	}

	/** Tags that are set on this object */
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	// End Gameplay Tag Interface

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	// Skips the attachment replication if we are locally owned and our grip settings say that we are a client authed grip.
	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "Replication")
		bool bAllowIgnoringAttachOnOwner;

	// Should we skip attachment replication (vr settings say we are a client auth grip and our owner is locally controlled)
	inline bool ShouldWeSkipAttachmentReplication(bool bConsiderHeld = true) const
	{
		if ((bConsiderHeld && !VRGripInterfaceSettings.bWasHeld) || GetNetMode() < ENetMode::NM_Client)
			return false;

		if (VRGripInterfaceSettings.MovementReplicationType == EGripMovementReplicationSettings::ClientSide_Authoritive ||
			VRGripInterfaceSettings.MovementReplicationType == EGripMovementReplicationSettings::ClientSide_Authoritive_NoRep)
		{
			return HasLocalNetOwner();
		}
		else
			return false;
	}

	// Fix bugs with replication and bReplicateMovement
	virtual void OnRep_AttachmentReplication() override;
	virtual void OnRep_ReplicateMovement() override;
	virtual void OnRep_ReplicatedMovement() override;
	virtual void PostNetReceivePhysicState() override;

	// Debug printing of when the object is replication destroyed
	/*virtual void OnSubobjectDestroyFromReplication(UObject *Subobject) override
	{
	Super::OnSubobjectDestroyFromReplication(Subobject);

	GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Killed Object On Actor: x: %s"), *Subobject->GetName()));
	}*/

	// This isn't called very many places but it does come up
	virtual void MarkComponentsAsPendingKill() override;

	/** Called right before being marked for destruction due to network replication */
	// Clean up our objects so that they aren't sitting around for GC
	virtual void PreDestroyFromReplication() override;

	// On Destroy clean up our objects
	virtual void BeginDestroy() override;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface")
		bool bRepGripSettingsAndGameplayTags;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadWrite, Category = "VRGripInterface")
		FBPInterfaceProperties VRGripInterfaceSettings;

	// Set up as deny instead of allow so that default allows for gripping
	virtual bool DenyGripping_Implementation() override;

	// How an interfaced object behaves when teleporting
	virtual EGripInterfaceTeleportBehavior TeleportBehavior_Implementation() override;

	// Should this object simulate on drop
	virtual bool SimulateOnDrop_Implementation() override;

	// Grip type to use
	virtual EGripCollisionType GetPrimaryGripType_Implementation(bool bIsSlot) override;

	// Secondary grip type
	virtual ESecondaryGripType SecondaryGripType_Implementation() override;

	// Define which movement repliation setting to use
	virtual EGripMovementReplicationSettings GripMovementReplicationType_Implementation() override;

	// Define the late update setting
	virtual EGripLateUpdateSettings GripLateUpdateSetting_Implementation() override;

	// What grip stiffness and damping to use if using a physics constraint
	virtual void GetGripStiffnessAndDamping_Implementation(float& GripStiffnessOut, float& GripDampingOut) override;

	// Get the advanced physics settings for this grip
	virtual FBPAdvGripSettings AdvancedGripSettings_Implementation() override;

	// What distance to break a grip at (only relevent with physics enabled grips
	virtual float GripBreakDistance_Implementation() override;

	// Get closest primary slot in range
	virtual  void ClosestGripSlotInRange_Implementation(FVector WorldLocation, bool bSecondarySlot, bool& bHadSlotInRange, FTransform& SlotWorldTransform, FName& SlotName, UGripMotionControllerComponent* CallingController = nullptr, FName OverridePrefix = NAME_None) override;

	// Check if an object allows multiple grips at one time
	virtual  bool AllowsMultipleGrips_Implementation() override;

	// Returns if the object is held and if so, which controllers are holding it
	virtual void IsHeld_Implementation(TArray<FBPGripPair>& HoldingControllers, bool& bIsHeld) override;

	// Sets is held, used by the plugin
	virtual void SetHeld_Implementation(UGripMotionControllerComponent* HoldingController, uint8 GripID, bool bIsHeld) override;

	// Returns if the object wants to be socketed
	virtual bool RequestsSocketing_Implementation(USceneComponent*& ParentToSocketTo, FName& OptionalSocketName, FTransform_NetQuantize& RelativeTransform) override;

	// Get grip scripts
	virtual bool GetGripScripts_Implementation(TArray<UVRGripScriptBase*>& ArrayReference) override;

	// Events //

	// Event triggered each tick on the interfaced object when gripped, can be used for custom movement or grip based logic
	virtual void TickGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation, float DeltaTime) override;

	// Event triggered on the interfaced object when gripped
	virtual void OnGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when grip is released
	virtual void OnGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController, const FBPActorGripInformation& GripInformation, bool bWasSocketed = false) override;

	// Event triggered on the interfaced object when child component is gripped
	virtual void OnChildGrip_Implementation(UGripMotionControllerComponent* GrippingController, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when child component is released
	virtual void OnChildGripRelease_Implementation(UGripMotionControllerComponent* ReleasingController, const FBPActorGripInformation& GripInformation, bool bWasSocketed = false) override;

	// Event triggered on the interfaced object when secondary gripped
	virtual void OnSecondaryGrip_Implementation(UGripMotionControllerComponent* GripOwningController, USceneComponent* SecondaryGripComponent, const FBPActorGripInformation& GripInformation) override;

	// Event triggered on the interfaced object when secondary grip is released
	virtual void OnSecondaryGripRelease_Implementation(UGripMotionControllerComponent* GripOwningController, USceneComponent* ReleasingSecondaryGripComponent, const FBPActorGripInformation& GripInformation) override;

	// Interaction Functions

	// Call to use an object
	virtual void OnUsed_Implementation() override;

	// Call to stop using an object
	virtual void OnEndUsed_Implementation() override;

	// Call to use an object
	virtual void OnSecondaryUsed_Implementation() override;

	// Call to stop using an object
	virtual void OnEndSecondaryUsed_Implementation() override;

	// Call to send an action event to the object
	virtual void OnInput_Implementation(FKey Key, EInputEvent KeyEvent) override;
};