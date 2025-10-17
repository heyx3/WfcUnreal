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


//General utilities:
struct WFCPP2_API WFCppUtils
{
	template<typename TKey, typename TValue, typename TAllocator, typename TKeyFuncs>
	static TValue TryGetByCopy(const TMap<TKey, TValue, TAllocator, TKeyFuncs>& map,
							   const TKey& key,
							   const TValue& defaultValue = { })
	{
		const TValue* found = map.Find(key);
		return found ? *found : defaultValue;
	}
};