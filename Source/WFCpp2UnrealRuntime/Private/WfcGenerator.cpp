#include "WfcGenerator.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

#include "WFCpp2UnrealRuntime.h"


float UWfcGenerator::GetProgress() const
{
	switch (GetStatus())
	{
		case WfcSimState::Finished: return 1.0f;
		case WfcSimState::Off: return 0.0f;
		
		case WfcSimState::Running: {
			verify(state.IsSet());
			const auto& wfc = state.GetValue();
		
			int nFinished = 0;
			for (auto cell : WFC::Region3i(wfc.Grid.Cells.GetDimensions()))
				if (wfc.Grid.Cells[cell].IsSet())
					nFinished += 1;
		
			return static_cast<float>(nFinished) /
				     state.GetValue().Grid.Cells.GetNumbElements();
		}
		
		default: return nanf(nullptr);
	}
}
FIntVector UWfcGenerator::GetGridSize() const
{
	if (state.IsSet())
		return { state->Grid.Cells.GetWidth(), state->Grid.Cells.GetHeight(), state->Grid.Cells.GetDepth() };

	UE_LOG(LogWFCpp, Error, TEXT("UWfcGenerator: Called GetGridSize() before the generation started! Returning 0"));
	return { 0, 0, 0 };
}

void UWfcGenerator::GetNextCells(TSet<FIntVector>& output) const
{
	output.Empty();
	
	if (!state.IsSet())
	{
		UE_LOG(LogWFCpp, Warning, TEXT("Calling 'GetNextCells()' when generation hasn't started yet! Returning an empty set"));
		return;
	}
	
	for (const auto& cell : state->GetNextCellsToProcess())
		output.Add({ cell.x, cell.y, cell.z });
}
void UWfcGenerator::GetUnsolvableCells(TSet<FIntVector>& output) const
{
	output.Empty();
	
	if (!state.IsSet())
	{
		UE_LOG(LogWFCpp, Warning, TEXT("Calling 'GetUnsolvableCells()' when generation hasn't started yet! Returning an empty set"));
		return;
	}
	
	for (const auto& cell : state->GetUnsolvableCells())
		output.Add({ cell.x, cell.y, cell.z });
}

int UWfcGenerator::GetTicksSinceLastSave() const
{
	if (stateHistoryBuffer.IsEmpty())
		return 0;
	check(state);

	//Watch for underflow with these uints!
	//Fortunately it should never happen with the history logic as it's currently written. 
	auto t1 = stateHistoryBuffer.Last().CurrentTimestamp,
		 t2 = state->CurrentTimestamp;
	check(t2 >= t1);
	return t2 - t1;
}

UWfcGeneratorInitialState* UWfcGenerator::SerializeInitialState(UObject* owner,
																FName name,
																bool isTransient) const
{
	//Returning a copy is important because users might mess with their returned instance.
	return NewObject<UWfcGeneratorInitialState>(
		owner ? owner : GetTransientPackage(), name,
		isTransient ? RF_Transient : RF_NoFlags,
		initialState
	);
}


FWfcCellStatus UWfcGenerator::GetCell(const FIntVector& cellPos, bool copyInData) const
{
	if (GetStatus() == WfcSimState::Off)
	{
		UE_LOG(LogWFCpp, Error, TEXT("UWfcGenerator::GetCell(): Simulation hasn't started yet!"));
		return { };
	}
	verify(state.IsSet());
	const auto& wfc = state.GetValue();

	WFC::Vector3i wfcPos(cellPos.X, cellPos.Y, cellPos.Z);
	WFC::Region3i wfcBounds(wfc.Grid.Cells.GetDimensions());

	if (!wfcBounds.Contains(wfcPos))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Given out-of-range grid pos: %i,%i,%i / %i,%i,%i"),
				wfcPos.x, wfcPos.y, wfcPos.z,
				wfcBounds.MaxExclusive.x, wfcBounds.MaxExclusive.y, wfcBounds.MaxExclusive.z);

		return { -1, false, { }, { } };
	}

	float temperature = wfc.GetTemperature(wfcPos);
	
	const auto& cell = wfc.Grid.Cells[wfcPos];
	if (cell.IsSet())
	{
		auto tileID = wfcLibraryData.WfcTileIDs[cell.ChosenTile];
		return { temperature, true, { }, {
			tileID,
			{
				static_cast<WFC_Rotations3D>(cell.ChosenPermutation.Rot),
				cell.ChosenPermutation.Invert
			},
			copyInData ? tileset->Tiles[tileID].Data : TInstancedStruct<FWfcGameData>::Make()
		} };
	}
	else
	{
		return { temperature, false, { cell.NPossibilities }, { } };
	}
}
TTuple<FWfcCellStatus, const TInstancedStruct<FWfcGameData>*> UWfcGenerator::GetCellWithPtr(const FIntVector& cellPos)
{
	if (GetStatus() == WfcSimState::Off)
	{
		UE_LOG(LogWFCpp, Error, TEXT("UWfcGenerator::GetCell(): Simulation hasn't started yet!"));
		return { FWfcCellStatus{ }, nullptr };
	}
	verify(state.IsSet());
	const auto& wfc = state.GetValue();

	WFC::Vector3i wfcPos(cellPos.X, cellPos.Y, cellPos.Z);
	WFC::Region3i wfcBounds(wfc.Grid.Cells.GetDimensions());

	if (!wfcBounds.Contains(wfcPos))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Given out-of-range grid pos: %i,%i,%i / %i,%i,%i"),
				wfcPos.x, wfcPos.y, wfcPos.z,
				wfcBounds.MaxExclusive.x, wfcBounds.MaxExclusive.y, wfcBounds.MaxExclusive.z);

		return { FWfcCellStatus{ -1, false, { }, { } }, nullptr };
	}

	float temperature = wfc.GetTemperature(wfcPos);
	
	const auto& cell = wfc.Grid.Cells[wfcPos];
	if (cell.IsSet())
	{
		auto tileID = wfcLibraryData.WfcTileIDs[cell.ChosenTile];
		return {
			FWfcCellStatus{
				temperature, true, { },
				{
					tileID,
					{
						static_cast<WFC_Rotations3D>(cell.ChosenPermutation.Rot),
						cell.ChosenPermutation.Invert
					},
					TInstancedStruct<FWfcGameData>::Make()
				}
			},
			&tileset->Tiles[tileID].Data
		};
	}
	else
	{
		return {
			FWfcCellStatus{
				temperature, false,
				{ cell.NPossibilities },
				{ }
			},
			nullptr
		};
	}
}

void UWfcGenerator::SetCell(const FIntVector& cell,
						    int32 unrealTileID, FWFC_Transform3D permutation,
						    bool permanent)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't set a WFC grid cell, because the WFC generator isn't running!"));
		return;
	}
	check(state.IsSet());
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	if (!wfcLibraryData.WfcTileIDByUnrealID.Contains(unrealTileID))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Invalid tile ID: %i"), unrealTileID);
		return;
	}

	state->SetCell({ cell.X, cell.Y, cell.Z },
				   wfcLibraryData.WfcTileIDByUnrealID[unrealTileID],
				   permutation.Unwrap(), permanent);
	if (permanent)
	{
		constraintsInOrder.Add(TInstancedStruct<FWfcConstraintEntry>::Make<FWfcConstraintEntry_Cell>(
			FWfcConstraintEntry{ },
			cell, unrealTileID, permutation, false
		));
		initialState->Constraints.Add(constraintsInOrder.Last());
	}
}

void UWfcGenerator::SetCellNot(const FIntVector& cell,
							   int32 unrealTileID, FWFC_Transform3D permutation)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't forbid a WFC tile in a grid cell, because the generator isn't running!"));
		return;
	}
	check(state.IsSet());
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	if (!wfcLibraryData.WfcTileIDByUnrealID.Contains(unrealTileID))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Invalid tile ID: %i"), unrealTileID);
		return;
	}
	
	state->SetCellConstraintNot({ cell.X, cell.Y, cell.Z },
							    wfcLibraryData.WfcTileIDByUnrealID[unrealTileID],
							    WFC::Tiled3D::TransformSet::Combine(permutation.Unwrap()));
	constraintsInOrder.Add(TInstancedStruct<FWfcConstraintEntry>::Make<FWfcConstraintEntry_Cell>(
		FWfcConstraintEntry{ },
		cell, unrealTileID, permutation, true
	));
	initialState->Constraints.Add(constraintsInOrder.Last());
}

void UWfcGenerator::SetFace(const FIntVector& cell, WFC_Directions3D face,
                            int facePrototypeId, WFC_Transforms2D facePermutationOrientation)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't set a WFC grid cell's face, because the generator isn't running!"));
		return;
	}
	check(state.IsSet());
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	if (!tileset->FacePrototypes.Contains(facePrototypeId))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Face prototype index is invalid: %i"), facePrototypeId);
		return;
	}
	check(wfcLibraryData.WfcFacePrototypeFirstIDs.Contains(facePrototypeId));

	auto permutedFacePoints = FWfcConstraintEntry_Face::UnwrapFacePermutation(
		face, facePermutationOrientation,
		tileset->FacePrototypes[facePrototypeId],
		wfcLibraryData.WfcFacePrototypeFirstIDs[facePrototypeId]
	);
	
	state->SetFaceConstraint(
		{ cell.X, cell.Y, cell.Z },
		static_cast<WFC::Tiled3D::Directions3D>(face),
		permutedFacePoints
	);
	constraintsInOrder.Add(TInstancedStruct<FWfcConstraintEntry>::Make<FWfcConstraintEntry_Face>(
		FWfcConstraintEntry{ },
		cell, face, facePrototypeId, facePermutationOrientation, false
	));
	initialState->Constraints.Add(constraintsInOrder.Last());
}
void UWfcGenerator::SetFaceNot(const FIntVector& cell, WFC_Directions3D face,
							   int facePrototypeId, WFC_Transforms2D facePermutationOrientation)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't forbid a WFC grid cell from having a particular face, because the generator isn't running!"));
		return;
	}
	check(state.IsSet());
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	if (!tileset->FacePrototypes.Contains(facePrototypeId))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Face prototype index is invalid: %i"), facePrototypeId);
		return;
	}

	auto permutedFacePoints = FWfcConstraintEntry_Face::UnwrapFacePermutation(
		face, facePermutationOrientation,
		tileset->FacePrototypes[facePrototypeId],
		wfcLibraryData.WfcFacePrototypeFirstIDs[facePrototypeId]
	);
	
	state->SetFaceConstraintNot(
		{ cell.X, cell.Y, cell.Z },
		static_cast<WFC::Tiled3D::Directions3D>(face),
		permutedFacePoints
	);
	constraintsInOrder.Add(TInstancedStruct<FWfcConstraintEntry>::Make<FWfcConstraintEntry_Face>(
		FWfcConstraintEntry{ },
		cell, face, facePrototypeId, facePermutationOrientation, true
	));
	initialState->Constraints.Add(constraintsInOrder.Last());
}

void UWfcGenerator::AddConstraints(const TArray<TInstancedStruct<FWfcConstraintEntry>>& constraints)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't add constraints, because the WFC generator isn't running!"));
		return;
	}
	check(state.IsSet());

	for (const auto& constraintEntry : constraints)
	{
		if (constraintEntry.GetScriptStruct() == FWfcConstraintEntry_Face::StaticStruct())
		{
			const auto& face = constraintEntry.Get<FWfcConstraintEntry_Face>();
			SetFaceConstraint(
				face.Cell, face.Side,
				face.FacePrototypeId, face.FacePrototypeTransform,
				face.IsForbidding
			);
		}
		else if (constraintEntry.GetScriptStruct() == FWfcConstraintEntry_Cell::StaticStruct())
		{
			const auto& cell = constraintEntry.Get<FWfcConstraintEntry_Cell>();
			SetCellConstraint(
				cell.Cell,
				cell.TileId, cell.TilePermutation,
				cell.IsForbidding
			);
		}
		else
		{
			auto typeName = (constraintEntry.GetScriptStruct() ?
								constraintEntry.GetScriptStruct()->GetStructCPPName() :
								FString{ TEXT("[null]") });
			UE_LOG(LogWFCpp, Error,
				   TEXT("Unhandled FWfcConstraintEntry type! Constraint will be ignored. %s"), *typeName);
		}
	}
}


int UWfcGenerator::GetNTilePossibilities() const
{
	return state.IsSet() ? state->Grid.NPermutedTiles : 0;
}

void UWfcGenerator::Stop()
{
	status = WfcSimState::Finished;
}

void UWfcGenerator::GetTemperatureData(float& out_min, float& out_max,
	  							       float& out_mean, float& out_median)
{
	out_min = std::numeric_limits<float>::infinity();
	out_max = -std::numeric_limits<float>::infinity();
	float sum = 0;
	TArray<float> sortedValues;

	for (WFC::Vector3i i : WFC::Region3i{ state->Grid.Cells.GetDimensions() })
	{
		if (state->Grid.Cells[i].IsSet())
			continue;
		float t = state->GetTemperature(i);
		
		out_min = FMath::Min(out_min, t);
		out_max = FMath::Max(out_max, t);
		
		sum += t;
		sortedValues.Insert(t, Algo::LowerBound(sortedValues, t));
	}

	out_mean = sum / FMath::Max(1, sortedValues.Num());
	out_median = (sortedValues.IsEmpty() ? 0 : sortedValues[sortedValues.Num() / 2]);
}

TOptional<TTuple<int, WFC_Transforms2D>> UWfcGenerator::GetFacePossibility(FIntVector cellPos,
																		   WFC_Directions3D face) const
{
	if (!state)
	{
		UE_LOG(LogWFCpp, Error,
			   TEXT("Tried to call UWfcGenerator::GetFacePossibility before the generator is running!"));
		return NullOpt;
	}

	//Wrap the given cell coordinate if applicable, then check that it's valid.
	auto cellPosWfc = state->Grid.FilterPos({ cellPos.X, cellPos.Y, cellPos.Z });
	cellPos = { cellPosWfc.x, cellPosWfc.y, cellPosWfc.z };
	if (!state->Grid.Cells.IsIndexValid({ cellPos.X, cellPos.Y, cellPos.Z }))
	{
		UE_LOG(
			LogWFCpp, Error,
			TEXT("Your cell pos %s is out of range of the grid (size %s)!"),
			*cellPos.ToString(),
			*FIntVector(state->Grid.Cells.GetWidth(),
					    state->Grid.Cells.GetHeight(),
						state->Grid.Cells.GetDepth()).ToString()
		);
		return NullOpt;
	}

	//Helper function that actually finds the Unreal face data.
	auto generateResult = [&](WFC::Tiled3D::TileIdx tile, WFC::Tiled3D::Transform3D tilePermutation) {
		auto permutedCube = tilePermutation.ApplyToCube(state->Grid.InputTiles[tile].Data);
		auto permutedFace = permutedCube.Faces[permutedCube.GetFace(static_cast<WFC::Tiled3D::Directions3D>(face))];
		return wfcLibraryData.ToUnrealFace(permutedFace, tileset);
	};
	
	//If the cell is already set, just grab its current face.
	//This isn't merely an optimization: the usual lookup is undefined after a cell is set.
	const auto& cell = state->Grid.Cells[{ cellPos.X, cellPos.Y, cellPos.Z }];
	if (cell.IsSet())
		return generateResult(cell.ChosenTile, cell.ChosenPermutation);
	
	//Look for the supported face on that cell;
	//    if we find a second one then immediately give up.
	WFC::Vector3i wfcCell{ cellPos.X, cellPos.Y, cellPos.Z };
	TOptional<TTuple<WfcFacePrototypeID, WFC_Transforms2D>> currentResult;
	for (int wfcTileI = 0; wfcTileI < state->Grid.InputTiles.size(); ++wfcTileI)
	{
		for (auto permutation : state->Grid.PossiblePermutations[{ wfcTileI, wfcCell }])
		{
			auto newResult = generateResult(wfcTileI, permutation);
			check(newResult.IsSet());
			
			if (currentResult != *newResult)
				if (currentResult.IsSet())
					return NullOpt;
				else
					currentResult = *newResult;
		}
	}
	return currentResult;
}

void UWfcGenerator::Start(const UWfcTileset* tiles,
                          const FIntVector& gridSize,
                          int newSeed,
                          float temperatureClearGrowthRateT, float fuzziness, int maxUnwinding,
                          bool periodicX, bool periodicY, bool periodicZ)
{
	//Clean up from any previous runs.
	if (IsRunning())
		Cancel();

    tileset = tiles;
	if (!IsValid(tileset) || tileset->Tiles.Num() == 0)
	{
		UE_LOG(LogWFCpp, Error, TEXT("Given a null or empty tileset to generate from! Generator will immediately exit"));
		return;
	}
	tileset->Unwrap(wfcLibraryData);

	//Create an object to remember this generator's initial state.
	static FName NAME_WfcInitialState = TEXT("WfcGeneratorInitialState");
	initialState = UWfcGeneratorInitialState::MakeInitialWfcState(
		tileset, gridSize, newSeed,
		periodicX, periodicY, periodicZ, { },
		temperatureClearGrowthRateT, fuzziness, maxUnwinding,
		this, NAME_WfcInitialState, true
	);

	//Start the algorithm.
	state.Emplace(
	    wfcLibraryData.Tiles, WFC::Vector3i(gridSize.X, gridSize.Y, gridSize.Z),
	    periodicX, periodicY, periodicZ,
	    WFC::PRNG(newSeed)
	);
	state->PriorityWeightRandomness = fuzziness,
	state->ClearRegionGrowthRateT = temperatureClearGrowthRateT;
	state->MaxUnwindingCount = maxUnwinding;
	status = WfcSimState::Running;
}
void UWfcGenerator::Cancel()
{
    status = WfcSimState::Off;
	
    state.Reset();
	stateHistoryBuffer.Empty();

	constraintsInOrder.Empty();
}

void UWfcGenerator::Tick(int nIterations)
{
	if (!IsRunning())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't Tick the UWfcGenerator if it isn't running!"));
        return;
	}
    check(state.IsSet());
	
    bool isFinished = state->TickN(nIterations);
    if (isFinished)
        status = WfcSimState::Finished;
    else
        status = WfcSimState::Running;
}

bool UWfcGenerator::RunToEnd(int timeoutIterations)
{
    bool isFinished = state.GetValue().TickN(timeoutIterations);
    if (isFinished)
    {
        status = WfcSimState::Finished;
        return true;
    }
    else
    {
        status = WfcSimState::Running;
        return false;
    }
}

void UWfcGenerator::SaveState()
{
	if (status == WfcSimState::Off)
	{
		UE_LOG(LogWFCpp, Error, TEXT("Trying to SaveState() on a UWfcGenerator that isn't running yet!"));
		return;
	}
	check(state);
	
	stateHistoryBuffer.Add(*state);
}
void UWfcGenerator::LoadState()
{
	if (status == WfcSimState::Off)
	{
		UE_LOG(LogWFCpp, Error, TEXT("Trying to LoadState() on a UWfcGenerator that isn't running yet!"));
		return;
	}
	check(state);

	if (stateHistoryBuffer.IsEmpty())
	{
		UE_LOG(LogWFCpp, Error, TEXT("There is no history; can't use LoadState() on this UWfcGenerator!"));
		return;
	}

	state = std::move(stateHistoryBuffer.Pop());
}
void UWfcGenerator::ClearStateHistory()
{
	if (status == WfcSimState::Off)
	{
		UE_LOG(LogWFCpp, Error, TEXT("Trying to ClearStateHistory() on a UWfcGenerator that isn't running yet!"));
		return;
	}
	check(state);
	
	stateHistoryBuffer.Empty();
}


UWfcGeneratorInitialState* UWfcGeneratorInitialState::MakeInitialWfcState(
						       const UWfcTileset* tileset,
							   FIntVector gridSize, int seedU32,
							   bool isPeriodicX, bool isPeriodicY, bool isPeriodicZ,
							   const TArray<TInstancedStruct<FWfcConstraintEntry>> constraints,
							   float tempClearGrowthRateT, float fuzziness,
							   int maxUnwinding,
							   UObject* owner,
							   FName name,
							   bool isTransient
						   )
{
	auto* o = NewObject<UWfcGeneratorInitialState>(owner, name, isTransient ? RF_Transient : RF_NoFlags);

	o->Tileset = tileset;
	o->PeriodicX = isPeriodicX;
	o->PeriodicY = isPeriodicY;
	o->PeriodicZ = isPeriodicZ;
	o->GridSize = gridSize;
	o->SeedU32 = seedU32;
	o->Constraints = constraints;
	o->TemperatureClearGrowthRateT = tempClearGrowthRateT;
	o->Fuzziness = fuzziness;
	o->MaxUnwinding = maxUnwinding;
	
	return o;
}

bool UWfcGeneratorInitialState::CompareWfcInitialStates(const UWfcGeneratorInitialState* a,
														const UWfcGeneratorInitialState* b)
{
	if (!IsValid(a))
		return !IsValid(b);

	return IsValid(b) &&
		   a->Tileset == b->Tileset &&
		   a->PeriodicX == b->PeriodicX &&
  		   a->PeriodicY == b->PeriodicY &&
		   a->PeriodicZ == b->PeriodicZ &&
		   a->GridSize == b->GridSize &&
		   a->SeedU32 == b->SeedU32 &&
		   a->Constraints == b->Constraints &&
		   a->TemperatureClearGrowthRateT == b->TemperatureClearGrowthRateT &&
		   a->Fuzziness == b->Fuzziness &&
		   a->MaxUnwinding == b->MaxUnwinding;
}

void UWfcGeneratorInitialState::RestartGenerator(UWfcGenerator* generatorToUse) const
{
	generatorToUse->Start(
		Tileset, GridSize,
		SeedU32,
		TemperatureClearGrowthRateT, Fuzziness,
		MaxUnwinding,
		PeriodicX, PeriodicY, PeriodicZ
	);
	generatorToUse->AddConstraints(Constraints);
}
UWfcGeneratorInitialState* UWfcGeneratorInitialState::SaveAsAsset(const FString& pathWithinContentFolder,
																  const UEngine::FCopyPropertiesForUnrelatedObjectsParams& copyParams,
																  bool highlightAsset)
{
	#if WITH_EDITOR

	//Source: https://dev.epicgames.com/community/learning/knowledge-base/wzdm/unreal-engine-how-to-create-new-assets-in-c
    
	FAssetToolsModule& moduleAssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& moduleContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FAssetRegistryModule& moduleAssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	//Generate a unique asset name.
	FString assetName, packageName;
	moduleAssetTools.Get().CreateUniqueAssetName(FString("/Game/") + pathWithinContentFolder,
												 TEXT("_WfcInitialState"),
												 packageName, assetName);
	auto packageDirPath = FPackageName::GetLongPackagePath(packageName);
	 
	//Create the object and its package.
	UPackage* package = CreatePackage(*packageName);
	auto* assetObject = CastChecked<UWfcGeneratorInitialState>(moduleAssetTools.Get().CreateAsset(
		assetName,
		packageDirPath,
		GetClass(),
		nullptr
	));
	UEngine::CopyPropertiesForUnrelatedObjects(this, assetObject, copyParams);

	//Save the package.
	FSavePackageArgs saveArgs;
	saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	GEditor->Save(package, assetObject,
				  *FPackageName::LongPackageNameToFilename(packageName, FPackageName::GetAssetPackageExtension()),
				  saveArgs);

	//Ping the editor about the new asset.
	moduleAssetRegistry.Get().AssetCreated(assetObject);
	if (highlightAsset)
		moduleContentBrowser.Get().SyncBrowserToAssets(TArray<UObject*>{ assetObject });

	return assetObject;

	#else
		UE_LOG(LogWFCpp, Error, TEXT("Tried to call UWfcGeneratorInitialState::SaveAsAsset() outside the editor!"));
		return nullptr;
	#endif
}
