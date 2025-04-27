#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

THIRD_PARTY_INCLUDES_START
#include "WFC++/include/Tiled3D/StandardRunner.h"
THIRD_PARTY_INCLUDES_END


class FWFCpp2Module : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
