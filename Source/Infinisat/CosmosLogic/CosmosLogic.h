#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCosmosLogicModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	UCosmosWorld* ActiveCosmosWorld = nullptr;
};