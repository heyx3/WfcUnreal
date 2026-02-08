// Copyright Cazals-de-fabel, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "WfcTileDebugActor.generated.h"

class UWfcTileGameData_Debug;

UCLASS(Abstract)
class WFCPP2UNREALRUNTIME_API AWfcTileDebugActor : public AActor
{
	GENERATED_BODY()

public:

	AWfcTileDebugActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
	TObjectPtr<UWfcTileGameData_Debug> TileGameData = nullptr;
	
	UFUNCTION(BlueprintImplementableEvent, Category = "WfcTileDebugActor")
	void OnConstructPreview(const UWfcTileGameData_Debug* InTileGameData);
};
