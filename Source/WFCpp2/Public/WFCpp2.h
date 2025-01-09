#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "WFC++/include/Tiled3D/StandardRunner.h"


class FWFCpp2Module : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
