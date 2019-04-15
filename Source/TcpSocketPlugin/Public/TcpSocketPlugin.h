// SpartanTools 2019

#pragma once

#include "Modules/ModuleManager.h"

class FTcpSocketPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
