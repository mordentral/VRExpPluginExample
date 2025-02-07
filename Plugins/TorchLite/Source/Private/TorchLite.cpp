//#include "StandAlonePrivatePCH.h"
#include "TorchLite.h"

void TorchLite::StartupModule()
{
    UE_LOG(LogTemp, Warning, TEXT("LibTorch Plugin Loaded!"));
}
 
void TorchLite::ShutdownModule()
{
    UE_LOG(LogTemp, Warning, TEXT("LibTorch Plugin Unloaded!"));
}
 
IMPLEMENT_MODULE(TorchLite, TorchLite)