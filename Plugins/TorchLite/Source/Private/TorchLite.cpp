//#include "StandAlonePrivatePCH.h"
#include "TorchLite.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

void LoadLibTorchDLLs()
{

}

void UnloadLibTorchDLLs()
{

}

void TorchLite::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("LibTorch Plugin Loaded!"));
   // LoadLibTorchDLLs();

}
 
void TorchLite::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("LibTorch Plugin Unloaded!"));
    //UnloadLibTorchDLLs();
}
 
IMPLEMENT_MODULE(TorchLite, TorchLite)