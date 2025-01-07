#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// #define WFCPP_ASSERT check
// #define WFCPP_CHECK_MEMORY_ERROR(s, ...) { checkf(false, TEXT("Wfcpp Unreal: heap corruption detected! " s), ##__VA_ARGS__); }
#include "Tiled3D/StandardRunner.h"


class FWFCpp2Module : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
