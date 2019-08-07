// SpartanTools 2019

#include "TcpSocketPlugin.h"
#include "TcpSocketSettings.h"
#include "Developer/Settings/Public/ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FTcpSocketPluginModule"

void FTcpSocketPluginModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Register settings
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "TcpSocket",
			LOCTEXT("RuntimeSettingsName", "TcpSocket"),
			LOCTEXT("RuntimeSettingsDescription", "Configure TcpSocket Plugin"),
			GetMutableDefault<UTcpSocketSettings>());
	}
}

void FTcpSocketPluginModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "TcpSocket");
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTcpSocketPluginModule, TcpSocketPlugin)