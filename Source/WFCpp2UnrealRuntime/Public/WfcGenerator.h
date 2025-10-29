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
	TInstancedStruct<FWfcGameData> TileGameData = TInstancedStruct<FWfcGameData>::Make();
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
//
//For visualization and debugging purposes,
//    it can also save the state of the algorithm to a buffer on request
//    and then rewind to those states.
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
	//Returns/overwrites the input with the set of "interesting" cells, that may be selected for the next tick.
	//Note that if there are any Unsolvable cells then those are handled first.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	void GetNextCells(TSet<FIntVector>& output) const;
	//Returns/overwrites the input with any cells that are no longer solveable.
	//The next tick will clear/undo them.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	void GetUnsolvableCells(TSet<FIntVector>& output) const;

	//Gets the set of "interesting" cells, that may be selected for the next tick.
	//Note that if there are any Unsolvable cells then those are handled first).
	TSet<FIntVector> GetNextCells() const
	{
		TSet<FIntVector> o;
		GetNextCells(o);
		return o;
	}
	//Gets any cells that are no longer solveable.
	//The next tick will clear/undo them.
	TSet<FIntVector> GetUnsolvableCells() const
	{
		TSet<FIntVector> o;
		GetUnsolvableCells(o);
		return o;
	}

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

	//Gets the number of states you've saved (from calling 'SaveState()').
	//If the generator hasn't started yet, this returns 0.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	int GetHistoryLength() const { return stateHistoryBuffer.Num(); }
	//Gets the number of ticks between the most recent saved state (from calling 'SaveState()')
	//    and the present.
	//E.g. if you just called SaveState(), then this returns 0.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	int GetTicksSinceLastSave() const;

	//Mainly intended for debugging.
	const auto& GetHistoryBuffer() const { return stateHistoryBuffer; }
	//Mainly intended for debugging.
	const auto* UnwrapStandardRunner() const { return state.GetPtrOrNull(); }


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
	//
	//If 'isPermanentConstraint' is true, this cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetCell(const FIntVector& cell,
				 int32 tileID, FWFC_Transform3D permutation,
				 bool isPermanentConstraint = false);
	//Explicitly forbids a tile at the given grid cell.
	//
	//This cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetCellNot(const FIntVector& cell,
				    int32 tileID, FWFC_Transform3D permutation);
	//Explicitly forbids or forces a particular cell to be a particular tile.
	//
	//This cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetCellConstraint(const FIntVector& cell,
						   int32 tileID, FWFC_Transform3D permutation,
						   bool forbid = false)
	{
		if (forbid)
			SetCellNot(cell, tileID, permutation);
		else
			SetCell(cell, tileID, permutation, true);
	}
	
	//Constrains the generator to always output the given face at the given cell.
	//
	//This cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFace(const FIntVector& cell, WFC_Directions3D face,
				 int facePrototypeId, WFC_Transforms2D facePermutationOrientation);
	//Constrains the generator to *never* output the given face at the given cell.
	//
	//This cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFaceNot(const FIntVector& cell, WFC_Directions3D face,
				    int facePrototypeId, WFC_Transforms2D facePermutationOrientation);
	//Explicitly forbids or forces a particular cell face.
	//
	//This cannot be undone.
	//You must call 'Start' before this!
	//
	//You may call this in the middle of generation rather than at the beginning,
	//     but it could make the solver worse at its job.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SetFaceConstraint(const FIntVector& cell, WFC_Directions3D face,
						   int facePrototypeId, WFC_Transforms2D facePermutationOrientation,
						   bool forbid = false)
	{
		if (forbid)
			SetFaceNot(cell, face, facePrototypeId, facePermutationOrientation);
		else
			SetFace(cell, face, facePrototypeId, facePermutationOrientation);
	}

	//Stops running the generator, leaving unset cells as permanently unsolved.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Stop();
	

    //Fails if the algorithm isn't running.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Tick(int nIterations = 1);

	//Destroys all progress this generator had, and leave it ready for another call to `Start()`.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void Cancel();
	
	//Runs WFC until the sim finishes (or fails). Stops at the given timeout count.
	//Returns whether it ended due to success.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	bool RunToEnd(int timeoutIterations = 10000);

	//Saves the current state of the algorithm to a buffer.
	//You can call 'LoadState()' to pop this saved state off of the buffer,
	//   effectively rewinding the algorithm.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void SaveState();
	//Pops the most recently-saved state of this algorithm (from calling 'SaveState()'),
	//    and overwrites the current state with it.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void LoadState();
	//Removes all saved states from this generator, freeing up that memory.
	UFUNCTION(BlueprintCallable, Category="WFC/Ops")
	void ClearStateHistory();

	
private:
    UPROPERTY()
    const UWfcTileset* tileset;
	UWfcTileset::Unwrapped wfcLibraryData;
    
	WfcSimState status = WfcSimState::Off;
	
	TOptional<WFC::Tiled3D::StandardRunner> state;
	TArray<WFC::Tiled3D::StandardRunner> stateHistoryBuffer;

};