﻿#pragma once

#include "WFCpp2.h"
#include "WfcTileset.h"

#include "WfcGenerator.generated.h"


UENUM(BlueprintType)
enum class WfcSimState : uint8
{
    Off, Running, Finished
};


//Encapsulates the running of the WFC algorithm on a tileset.
UCLASS(BlueprintType)
class WFCPP2UNREALRUNTIME_API UWfcGenerator : public UObject
{
    GENERATED_BODY()
public:

    //TODO: Make the tileset and grid size constant over this generator's lifetime, so it can re-use memory between runs.

    UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm", meta=(CompactNodeTitle="Status"))
    WfcSimState GetStatus() const { return status; }
	
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm", meta=(CompactNodeTitle="Running?"))
    bool IsRunning() const { return GetStatus() == WfcSimState::Running; }

    //Returns a progress indicator from 0 to 1.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
    float GetProgress() const;

	//TODO: More ways to get information about the algorithm

	//-----------------
	//  Result queries
	//-----------------

    //Gets the tile assigned to the given cell, if one has been assigned yet.
    //If no tile has been assigned, returns "false".
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	bool GetCell(const FIntVector& cell,
				 int32& out_tileID,
				 UWfcTileGameData*& out_tileData,
				 FWFC_Transform3D& out_tileTransform,
				 float& outTemperature) const;
	//Calculates detailed info on the temperature of unsolved cells across the entire grid.
	UFUNCTION(BlueprintCallable, Category="WFC/Algorithm")
	void GetTemperatureData(float& min, float& max,
		  				    float& mean, float& median);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	int GetTickCount() const { return static_cast<int>(state->CurrentTimestamp); }

	
	//-------------
	//  Operations
	//-------------
	
	//Kicks off the WFC algorithm with the given inputs.
    //If the algorithm was already running, that previous run will be canceled.
	//Note that if 'clearSize' is set to zero, then WFC will fail if it encounters an unsolvable grid cell.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops", meta=(AdvancedDisplay=5))
	void Start(const UWfcTileset* tiles, const FIntVector& gridSize,
	           int seedU32 = 1234567890,
	           float temperatureClearGrowthRateT = 0.5f,
	           bool periodicX = false,
	           bool periodicY = false,
	           bool periodicZ = false);

	//Explicitly sets the given grid cell.
	//You must call 'Start' before this!
	//
	//  'persistent' : if true, the generator is not allowed to clear this tile from this cell. 
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetCell(const FIntVector& cell,
				 int32 tileID, FWFC_Transform3D permutation,
				 bool persistent = true);
	//Constrains the generator to always output the given face at the given cell.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFace(const FIntVector& cell, WFC_Directions3D face,
				 int facePrototypeId, WFC_Transforms2D facePermutationOrientation);

	//Stops running the generator, leaving unset cells as permanently unsolved.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Stop();
	

    //Fails if the algorithm isn't running.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Tick();

	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Cancel();
	
	//Runs WFC until the sim finishes (or fails). Stops at the given timeout count.
	//Returns whether it ended due to success.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	bool RunToEnd(int timeoutIterations = 10000);

	
private:
    UPROPERTY()
    const UWfcTileset* tileset;
    
	WfcSimState status = WfcSimState::Off;
	TOptional<WFC::Tiled3D::StandardRunner> state;

	UWfcTileset::Unwrapped wfcLibraryData;
};