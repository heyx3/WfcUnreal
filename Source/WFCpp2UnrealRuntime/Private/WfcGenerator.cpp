#include "WfcGenerator.h"

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

FWfcCellStatus UWfcGenerator::GetCell(const FIntVector& cellPos) const
{
	checkf(GetStatus() != WfcSimState::Off, TEXT("Simulation hasn't started yet"));
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
			tileset->Tiles[tileID].Data
		} };
	}
	else
	{
		return { temperature, false, { cell.NPossibilities }, { } };
	}
}
void UWfcGenerator::SetCell(const FIntVector& cell, int32 unrealTileID, FWFC_Transform3D permutation, bool persistent)
{
	if (!state.IsSet())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't set a WFC grid cell because the WFC generator isn't initialized yet!"));
		return;
	}
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	auto wfcTileID = wfcLibraryData.WfcTileIDByUnrealID[unrealTileID];
	
	state->SetCell({ cell.X, cell.Y, cell.Z }, wfcTileID,
				   permutation.Unwrap(), persistent);
}

void UWfcGenerator::SetFace(const FIntVector& cell, WFC_Directions3D face,
							int facePrototypeId, WFC_Transforms2D facePermutationOrientation)
{
	if (!state.IsSet())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Can't set a WFC grid cell because the WFC generator isn't initialized yet!"));
		return;
	}
	if (!state->Grid.Cells.IsIndexValid({ cell.X, cell.Y, cell.Z }))
	{
		UE_LOG(LogWFCpp, Error, TEXT("Cell index is out of range: %i,%i,%i"), cell.X, cell.Y, cell.Z);
		return;
	}
	if (facePrototypeId < 0 || facePrototypeId >= tileset->FacePrototypes.Num())
	{
		UE_LOG(LogWFCpp, Error, TEXT("Face prototype index is invalid: %i/%i"), facePrototypeId, tileset->FacePrototypes.Num());
		return;
	}

	auto points = tileset->FacePrototypes[facePrototypeId];
	state->SetFaceConstraint(
		{ cell.X, cell.Y, cell.Z }, static_cast<WFC::Tiled3D::Directions3D>(face),
		points.Unwrap(wfcLibraryData.WfcFacePrototypeFirstIDs[facePrototypeId])
	);
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

void UWfcGenerator::Start(const UWfcTileset* tiles,
                          const FIntVector& gridSize,
			              int seed,
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

	//Start the algorithm.
	state.Emplace(
	    wfcLibraryData.Tiles, WFC::Vector3i(gridSize.X, gridSize.Y, gridSize.Z),
	    nullptr,
	    WFC::PRNG(seed)
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
}

void UWfcGenerator::Tick()
{
    checkf(IsRunning(), TEXT("Can't Tick the WFC algorithm if it isn't running!"));
    check(state.IsSet());
	
    bool isFinished = state->Tick();
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