// SpartanTools 2019

#pragma once

#include "TcpSocketSettings.generated.h"

UCLASS(config = Engine, defaultconfig)
class TCPSOCKETPLUGIN_API UTcpSocketSettings : public UObject
{
	GENERATED_BODY()
	
public:
	/** Post errors to message log. */
	UPROPERTY(Config, EditAnywhere, Category = "TcpSocketPlugin")
	bool bPostErrorsToMessageLog;	
};
