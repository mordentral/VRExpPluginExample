// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "WheeledVehicle.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Pawn.h"
#include "VRWheeledVehicle.generated.h"


UCLASS(config = Game, BlueprintType)
class VREXPANSIONPLUGIN_API AVRWheeledVehicle : public AWheeledVehicle
{
	GENERATED_BODY()

public:

	UFUNCTION()
		virtual void OnRep_Controller() override
	{
		if ((Controller != NULL) && (Controller->GetPawn() == NULL))
		{
			// This ensures that APawn::OnRep_Pawn is called. Since we cant ensure replication order of APawn::Controller and AController::Pawn,
			// if APawn::Controller is repped first, it will set AController::Pawn locally. When AController::Pawn is repped, the rep value will not
			// be different from the just set local value, and OnRep_Pawn will not be called. This can cause problems if OnRep_Pawn does anything important.
			//
			// It would be better to never ever set replicated properties locally, but this is pretty core in the gameplay framework and I think there are
			// lots of assumptions made in the code base that the Pawn and Controller will always be linked both ways.
			//Controller->SetPawnFromRep(this);

			/*APlayerController* const PC = Cast<APlayerController>(Controller);
			if ((PC != NULL) && PC->bAutoManageActiveCameraTarget && (PC->PlayerCameraManager->ViewTarget.Target == Controller))
			{
				PC->AutoManageActiveCameraTarget(this);
			}*/
		}
	}


	UFUNCTION(BlueprintCallable, Category = "Pawn")
		virtual bool ForceSecondaryPossession(AController * NewController)
	{
		if (NewController)
			PossessedBy(NewController);
		else
			UnPossessed();

		return true;
		//INetworkPredictionInterface* NetworkPredictionInterface = GetPawn() ? Cast<INetworkPredictionInterface>(GetPawn()->GetMovementComponent()) : NULL;
		//if (NetworkPredictionInterface)
		//{
		//	NetworkPredictionInterface->ResetPredictionData_Server();
	//	}


	// Local PCs will have the Restart() triggered right away in ClientRestart (via PawnClientRestart()), but the server should call Restart() locally for remote PCs.
	// We're really just trying to avoid calling Restart() multiple times.
	//	if (!IsLocalPlayerController())
	//	{
		//	GetPawn()->Restart();
	//	}
	//	ClientRestart(GetPawn());

		//ChangeState(NAME_Playing);
		//if (bAutoManageActiveCameraTarget)
		//{
		//	AutoManageActiveCameraTarget(GetPawn());
		//	ResetCameraMode();
		//}
		//UpdateNavigationComponents();
	}

};