#pragma once

#include "WFCpp2.h"
#include "WfcTileset.h"

#include "WfcGenerator.generated.h"


UENUM(BlueprintType)
enum class WfcSimState : uint8
{
    Off, Running, Finished
};

//Info about a cell in WFC generation that has been set already.
USTRUCT(BlueprintType)
struct FWfcCellSet
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int TileID = -1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWFC_Transform3D TilePermutation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UWfcTileGameData* TileGameData = nullptr;
};

//Info about a cell in WFC generation that has not been set yet.
USTRUCT(BlueprintType)
struct FWfcCellUnset
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int NPossibilities = 0;
};

//Info about a cell in WFC generation.
USTRUCT(BlueprintType)
struct FWfcCellStatus
{
	GENERATED_BODY()
public:

	//A metric that increases as a cell gets repeatedly cleared within a short number of ticks. 
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Temperature = 0;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool IsSet = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWfcCellUnset IfUnset;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FWfcCellSet IfSet;
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


	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	FIntVector GetGridSize() const;
    //Returns a progress indicator from 0 to 1.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
    float GetProgress() const;

	//TODO: More ways to get information about the algorithm

	//-----------------
	//  Result queries
	//-----------------

    //Gets the tile assigned to the given cell, if one has been assigned yet.
    //If no tile has been assigned, returns "false".
	//
	//If the cell is set, the return value can contain a copy of the tile's 'Data' field.
	//If you decline, it will always be left null (an instance of 'FWfcGameData').
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	FWfcCellStatus GetCell(const FIntVector& cell, bool copyInData) const;
	//A more efficient C++ version of 'GetCell'.
	//The cell's custom data is never copied into the returned struct,
	//    instead being given as an extra pointer to it (if it exists, else null).
	TTuple<FWfcCellStatus, const TInstancedStruct<FWfcGameData>*> GetCellWithPtr(const FIntVector& cell);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	int GetNTilePossibilities() const;
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
	           float fuzziness = 0.1f,
	           int maxUnwinding = 0,
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
	//This cannot be undone.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFace(const FIntVector& cell, WFC_Directions3D face,
				 int facePrototypeId, WFC_Transforms2D facePermutationOrientation,
				 bool invert = false);
	//Constrains the generator to *never* output the given face at the given cell.
	//This cannot be undone.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFaceNot(const FIntVector& cell, WFC_Directions3D face,
					int facePrototypeId, WFC_Transforms2D facePermutationOrientation);

	//Stops running the generator, leaving unset cells as permanently unsolved.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Stop();
	

    //Fails if the algorithm isn't running.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Tick(int nIterations = 1);

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