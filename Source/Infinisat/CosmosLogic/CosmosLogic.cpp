#include "CosmosLogic.h"
#include "CosmosWorld.h"
#include "Engine/Engine.h"

void FCosmosLogicModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Cosmos Logic Module Started"));
}

void FCosmosLogicModule::ShutdownModule()
{
	ActiveCosmosWorld = nullptr;
	UE_LOG(LogTemp, Log, TEXT("Cosmos Logic Module Shutdown"));
}

IMPLEMENT_MODULE(FCosmosLogicModule, CosmosLogic)