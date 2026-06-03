#pragma once

#include "WFCpp2.h"
#include "WfcTileset.h"
#include "WfcConstraintHistory.h"

#include <functional>

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

	//A value < 1 means the cell is unsolvable.
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
	
	//Creates a data-object representing this generator's startup parameters,
	//    including any constraints you've set.
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="WFC/Algorithm")
	class UWfcGeneratorInitialState* SerializeInitialState(UObject* owner = nullptr,
														   FName name = NAME_None,
														   bool isTransient = true) const;
	
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

	//Gets the history of all constraints you have passed to this generator.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	const TArray<TInstancedStruct<FWfcConstraintEntry>>& GetConstraintHistory() const { return constraintsInOrder; }
	

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

	//Given a specific cell face, this represents a tile/face that is forced to be there.
	//The face permutation must always be the first matching one so that
	//   we don't have to worry about symmetry when checking if two instances are compatible.
	struct ForcedCellFace
	{
		int FacePrototypeID;
		WFC_Transforms2D FacePermutation;

		bool CompatibleWith(const ForcedCellFace& otherForce) const
		{
			return FacePrototypeID == otherForce.FacePrototypeID &&
				   FacePermutation == otherForce.FacePermutation;
		}
	};
	//Stores all the potential ways that a cell face may be forced to take a specific value.
	struct ForcedCellFaceSources
	{
		//Represents the neighboring cell across this face which has chosen a tile already. 
		TOptional<ForcedCellFace> Neighbor;
		//Represents this own cell which has chosen a tile already.
		TOptional<ForcedCellFace> Self;

		//Not set if there are multiple conflicting constraints (see 'IsPermanentlyUnsolvable').
		TOptional<ForcedCellFace> InitialConstraints;
		//If true, there are conflicting Initial Constraints that make this cell permanently unsolvable.
		//In this case, 'InitialConstraints' is left unset since there is no single value it can have.
		bool IsPermanentlyUnsolvable = false;
		//If true, there are some less-strict constraints not represented in this struct
		//   (e.g. constraints forbidding a particular kind of face).
		bool HasLighterConstraints = true;

		//True if more than one of these sources is set and they disagree with each other
		//  (or 'IsPermanentlyUnsolvable' is true).
		bool IsUnsolvable = false;

		void InitNeighbor(const ForcedCellFace& force);
		void InitSelf(const ForcedCellFace& force);
		void AddInitialConstraint(const ForcedCellFace& force);

		//If this struct has exactly one forced cell face (i.e. not unsolvable and not empty), returns a reference to it.
		//Otherwise returns null.
		const ForcedCellFace* TryGetFace() const
		{
			if (IsUnsolvable)
				return nullptr;
			else if (Neighbor.IsSet())
				return Neighbor.GetPtrOrNull();
			else if (Self.IsSet())
				return Self.GetPtrOrNull();
			else
				return InitialConstraints.GetPtrOrNull();
		}
	};
	//For the given cell face, gathers all the ways in which a certain tile/face must be placed there.
	//
	//Not thread-safe (i.e. don't run this more than once at a time).
	ForcedCellFaceSources GetFacePossibilities(FIntVector cellPos, WFC_Directions3D face) const;
	//(BP overload; not recommended in C++).
	//
	//For the given cell face, reports any constraints which force that face to take a certain value.
	//
	//Also reports if the constraints are conflicting (making it unsolvable),
	//   and whether that unsolvability goes all the way back to the initial constraints (making it permanent).
	//
	//Not thread-safe (i.e. don't run this more than once at a time).
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	void GetFacePossibilities(const FIntVector& cellPos, WFC_Directions3D face,
							  bool& isUnsolvable, bool& unsolvabilityIsPermanent,
							  int& solvedFacePrototypeID, WFC_Transforms2D& solvedFacePermutation,
							  bool& solvedByNeighborCell, bool& solvedBySelfCell, bool& solvedByInitialConstraints) const
	{
		auto result = GetFacePossibilities(cellPos, face);
		
		isUnsolvable = result.IsUnsolvable;
		unsolvabilityIsPermanent = result.IsPermanentlyUnsolvable;
		solvedByNeighborCell = result.Neighbor.IsSet();
		solvedBySelfCell = result.Self.IsSet();
		solvedByInitialConstraints = result.InitialConstraints.IsSet();
		
		if (!isUnsolvable)
		{
			ForcedCellFace* source;
			if (solvedByNeighborCell)
				source = result.Neighbor.GetPtrOrNull();
			else if (solvedBySelfCell)
				source = result.Self.GetPtrOrNull();
			else
			{
				check(solvedByInitialConstraints);
				source = result.InitialConstraints.GetPtrOrNull();
			}

			solvedFacePrototypeID = source->FacePrototypeID;
			solvedFacePermutation = source->FacePermutation;
		}
	}
	
	//(BP overload; not recommended in C++).
	//
	//For the given cell face, if it can only be one kind of face, returns that face.
	//Note that if a face has symmetries then the specific permutation returned is arbitrary but deterministic.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	void GetFacePossibility(const FIntVector& cellPos, WFC_Directions3D face,
							bool& exists, int& facePrototypeId, WFC_Transforms2D& facePermutation) const
	{
		auto result = GetFacePossibility(cellPos, face);
		exists = result.IsSet();
		if (exists)
		{
			facePrototypeId = result->Get<0>();
			facePermutation = result->Get<1>();
		}
		else
		{
			facePrototypeId = -1;
			facePermutation = WFC_Transforms2D::None;
		}
	}
	//For the given cell face, if it can only be one kind of face,
	//    returns that face (as its prototype ID and permutation).
	//Note that if a face has symmetries,
	//    then the specific permutation returned is arbitrary but deterministic.
	TOptional<TTuple<int, WFC_Transforms2D>> GetFacePossibility(FIntVector cellPos,
																WFC_Directions3D face) const;

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
	const auto& GetStateHistoryBuffer() const { return stateHistoryBuffer; }
	//Mainly intended for debugging.
	const auto* UnwrapStandardRunner() const { return state.GetPtrOrNull(); }


    //-------------
	//  Operations
	//-------------
	
	//Kicks off the WFC algorithm with the given inputs.
    //If the algorithm was already running, that previous run will be canceled.
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
	
	//Applies a set of constraints to this generator, in order.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="WFC/Algorithm")
	void AddConstraints(const TArray<TInstancedStruct<FWfcConstraintEntry>>& constraints);

	
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
    const UWfcTileset* tileset = nullptr;
	UPROPERTY()
	UWfcGeneratorInitialState* initialState = nullptr;
	
	UWfcTileset::Unwrapped wfcLibraryData;
    
	WfcSimState status = WfcSimState::Off;
	
	TOptional<WFC::Tiled3D::StandardRunner> state;
	TArray<WFC::Tiled3D::StandardRunner> stateHistoryBuffer;

	TArray<TInstancedStruct<FWfcConstraintEntry>> constraintsInOrder;

	//Used internally for some functions.
	mutable TMap<int, WFC::TransformationFlags> facePermutationsBuffer;
};


//A serialized initial state for a WFC generator, including constraints.
UCLASS(BlueprintType)
class WFCPP2UNREALRUNTIME_API UWfcGeneratorInitialState : public UObject
{
	GENERATED_BODY()
public:
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	const UWfcTileset* Tileset = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool PeriodicX = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool PeriodicY = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool PeriodicZ = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FIntVector GridSize = { 0, 0, 0 };
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int SeedU32 = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<TInstancedStruct<FWfcConstraintEntry>> Constraints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float TemperatureClearGrowthRateT = 0.5f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Fuzziness = 0.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int MaxUnwinding = 0;


	UFUNCTION(BlueprintCallable)
	static UWfcGeneratorInitialState* MakeInitialWfcState(const UWfcTileset* theTileset,
														  FIntVector theGridSize, int u32Seed,
														  bool periodicAlongX, bool periodicAlongY, bool periodicAlongZ,
														  const TArray<TInstancedStruct<FWfcConstraintEntry>> theConstraints,
														  float theTempClearGrowthRateT, float randFuzziness,
														  int maxUnwindingCount,
														  UObject* owner = nullptr,
														  FName name = NAME_None,
														  bool isTransient = true);
	UFUNCTION(BlueprintCallable, BlueprintPure)
	static bool CompareWfcInitialStates(const UWfcGeneratorInitialState* a, const UWfcGeneratorInitialState* b);
	
	//Starts a new generator from this initial state.
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	UWfcGenerator* StartGenerator(UObject* owner = nullptr,
								  bool isTransient = true) const
	{
		auto* gen = NewObject<UWfcGenerator>(owner, NAME_None, isTransient ? RF_Transient : RF_NoFlags);
		RestartGenerator(gen);
		return gen;
	}
	
	//Starts a new generator from this initial state.
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	void RestartGenerator(UWfcGenerator* generatorToUse) const;

	//Saves this object as a new project asset, returning that asset copy.
	//Does nothing outside the editor.
	UFUNCTION(BlueprintCallable, CallInEditor)
	UWfcGeneratorInitialState* SaveAsAsset(const FString& pathWithinContentFolder,
										   bool highlightAsset)
	{
		return SaveAsAsset(pathWithinContentFolder,
						   UEngine::FCopyPropertiesForUnrelatedObjectsParams{ },
						   highlightAsset);
	}
	//Saves this object as a new project asset, returning that asset copy.
	//Does nothing outside the editor.
	UWfcGeneratorInitialState* SaveAsAsset(const FString& pathWithinContentFolder,
										   const UEngine::FCopyPropertiesForUnrelatedObjectsParams& copyParams,
										   bool highlightAsset);
};
