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

	if (permanent && FMath::Abs(FVector::Dist(FVector{ cell }, FVector{ 2, 9, 5 }) - 1.0) < 0.0001)
		__debugbreak();
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

	//DEBUG:
	UE_LOG(LogWFCpp, Warning, TEXT("DEBUG// Starting constraints..."));
	for (const auto& constraintEntry : constraints)
	{
		if (constraintEntry.GetScriptStruct() == FWfcConstraintEntry_Face::StaticStruct())
		{
			const auto& face = constraintEntry.Get<FWfcConstraintEntry_Face>();
			//DEBUG:
			if (face.Cell == FIntVector{ 4, 9, 5 } ||
				(WFC::Vector3i{ face.Cell.X, face.Cell.Y, face.Cell.Z } + WFC::Tiled3D::GetFaceDirection((WFC::Tiled3D::Directions3D)face.Side)) == WFC::Vector3i{4, 9, 5})
			{
				UE_LOG(LogWFCpp, Warning, TEXT("DEBUG// %s the %s face of '%s'/%s at %i,%i,%i"),
					   face.IsForbidding ? TEXT("Forbidding") : TEXT("Guaranteeing"),
					   *UEnum::GetValueAsString(face.Side).RightChop(18),
					   *tileset->FacePrototypes[face.FacePrototypeId].Nickname,
					   *UEnum::GetValueAsString(face.FacePrototypeTransform).RightChop(18),
					   face.Cell.X, face.Cell.Y, face.Cell.Z);
			}
			SetFaceConstraint(
				face.Cell, face.Side,
				face.FacePrototypeId, face.FacePrototypeTransform,
				face.IsForbidding
			);
		}
		else if (constraintEntry.GetScriptStruct() == FWfcConstraintEntry_Cell::StaticStruct())
		{
			const auto& cell = constraintEntry.Get<FWfcConstraintEntry_Cell>();
			//DEBUG:
			if (FVector::Dist(FVector{ cell.Cell }, FVector{ 4, 9, 5 }) < 1.0001)
			{
				UE_LOG(LogWFCpp, Warning, TEXT("DEBUG// %s the cell at %i,%i,%i to '%s'/%s/%s"),
					   cell.IsForbidding ? TEXT("Forbidding") : TEXT("Guaranteeing"),
					   cell.Cell.X, cell.Cell.Y, cell.Cell.Z,
					   *tileset->Tiles[cell.TileId].GetDisplayName(),
					   cell.TilePermutation.Invert ? TEXT("inv") : TEXT(""),
					   *UEnum::GetValueAsString(cell.TilePermutation.Rot).RightChop(17));
			}
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

void UWfcGenerator::ForcedCellFaceSources::InitNeighbor(const ForcedCellFace& force)
{
	check(!Neighbor.IsSet());
	Neighbor = force;

	IsUnsolvable |= (Self.IsSet() && !Self->CompatibleWith(force));
	IsUnsolvable |= (InitialConstraints.IsSet() && !InitialConstraints->CompatibleWith(force));
}

void UWfcGenerator::ForcedCellFaceSources::InitSelf(const ForcedCellFace& force)
{
	check(!Self.IsSet());
	Self = force;

	IsUnsolvable |= (Neighbor.IsSet() && !Neighbor->CompatibleWith(force));
	IsUnsolvable |= (InitialConstraints.IsSet() && !InitialConstraints->CompatibleWith(force));
}

void UWfcGenerator::ForcedCellFaceSources::AddInitialConstraint(const ForcedCellFace& force)
{
	if (IsPermanentlyUnsolvable)
	{
		check(!InitialConstraints.IsSet());
		return;
	}
	else if (InitialConstraints.IsSet())
	{
		if (!InitialConstraints->CompatibleWith(force))
		{
			InitialConstraints.Reset();
			IsPermanentlyUnsolvable = true;
		}
	}
	else
	{
		InitialConstraints = force;

		IsUnsolvable |= (Self.IsSet() && !Self->CompatibleWith(force));
		IsUnsolvable |= (Neighbor.IsSet() && !Neighbor->CompatibleWith(force));
	}
}

UWfcGenerator::ForcedCellFaceSources UWfcGenerator::GetFacePossibilities(FIntVector cellPos, WFC_Directions3D face) const
{
	ForcedCellFaceSources output;
	
	if (!state)
	{
		UE_LOG(LogWFCpp, Error, TEXT("Tried to call UWfcGenerator::GetFacePossibilities before the generator is running!"));
		return output;
	}

	auto faceWfc = static_cast<WFC::Tiled3D::Directions3D>(face),
		 oppositeFaceWfc = WFC::Tiled3D::GetOpposite(faceWfc);
	auto oppositeFace = static_cast<WFC_Directions3D>(oppositeFaceWfc);
	
	//Wrap the given cell coordinate if applicable, then check that it's valid.
	auto cellPosWfc = state->Grid.FilterPos({ cellPos.X, cellPos.Y, cellPos.Z });
	cellPos = { cellPosWfc.x, cellPosWfc.y, cellPosWfc.z };
	if (!state->Grid.Cells.IsIndexValid({ cellPos.X, cellPos.Y, cellPos.Z }))
	{
		UE_LOG(
			LogWFCpp, Error,
			TEXT("UWfcGenerator::GetFacePossibilities(): "
					"Your cell pos %s is out of range of the grid (size %s)!"),
			*cellPos.ToString(),
			*FIntVector(state->Grid.Cells.GetWidth(),
						state->Grid.Cells.GetHeight(),
						state->Grid.Cells.GetDepth()).ToString()
		);
		return output;
	}
	//Get the neighbor's position too.
	auto cellPosNeighborWfc = cellPosWfc;
	cellPosNeighborWfc = state->Grid.FilterPos(cellPosWfc + WFC::Tiled3D::GetFaceDirection(faceWfc));
	FIntVector cellPosNeighbor{ cellPosNeighborWfc.x, cellPosNeighborWfc.y, cellPosNeighborWfc.z }; 
	bool neighborExists = state->Grid.Cells.IsIndexValid(cellPosNeighborWfc);

	//Lambda that gets the WFC library face data, for a specific tile permutation,
	//  on the face being queried.
	auto generateResult = [&](WFC::Tiled3D::TileIdx tile, WFC::Tiled3D::Transform3D tilePermutation) {
		auto permutedCube = tilePermutation.ApplyToCube(state->Grid.InputTiles[tile].Data);
		auto permutedFace = permutedCube.Faces[permutedCube.GetFace(faceWfc)];
		return permutedFace;
	};

	//Check whether this cell is set.
	const auto& cell = state->Grid.Cells[{ cellPos.X, cellPos.Y, cellPos.Z }];
	if (cell.IsSet())
	{
		auto faceData = generateResult(cell.ChosenTile, cell.ChosenPermutation);
		auto faceDataU = wfcLibraryData.ToUnrealFace(faceData, tileset);
		output.InitSelf({faceDataU->Key, faceDataU->Value });
	}

	//Check whether the neighbor's cell is set.
	const auto* neighborCellPtr = neighborExists ? &state->Grid.Cells[cellPosNeighborWfc] : nullptr;
	if (neighborCellPtr && neighborCellPtr->IsSet())
	{
		auto faceData = generateResult(neighborCellPtr->ChosenTile, neighborCellPtr->ChosenPermutation);
		auto faceDataU = wfcLibraryData.ToUnrealFace(faceData, tileset);
		output.InitSelf({ faceDataU->Key, faceDataU->Value });
	}

	//Gather user constraints for this face.
	//Constraints that set a face to a value are obviously important,
	//    but it's also possible to force a face by outlawing all the others.
	check(facePermutationsBuffer.IsEmpty());
	auto& nonForbiddenFaces = facePermutationsBuffer;
	for (const auto& kvp : tileset->FacePrototypes)
		nonForbiddenFaces.Add(kvp.Key, WFC::TransformationFlags::All());
	auto countNonForbiddenFaces = [&]() {
		return Algo::Accumulate(nonForbiddenFaces, 0,
							    [&](int count, const auto& kvp) { return count + kvp.Value; });
	};
	int initialNonForbiddenFacesCount = countNonForbiddenFaces();
	//
	for (const auto& _constraint : GetConstraintHistory())
	{
		//Exit early if conflicting constraints have already been found.
		if (output.IsPermanentlyUnsolvable)
			break;

		if (_constraint.GetScriptStruct() == FWfcConstraintEntry_Face::StaticStruct() ||
			_constraint.GetScriptStruct() == FWfcConstraintEntry_Cell::StaticStruct())
		{
			FIntVector constraintCellPos;
			WFC_Directions3D constraintAffectedFace;
			if (_constraint.GetScriptStruct() == FWfcConstraintEntry_Face::StaticStruct())
			{
				auto& constraint = _constraint.Get<FWfcConstraintEntry_Face>();
				constraintCellPos = constraint.Cell;
				constraintAffectedFace = constraint.Side;
			}
			else
			{
				auto& constraint = _constraint.Get<FWfcConstraintEntry_Cell>();
				constraintCellPos = constraint.Cell;
				constraintAffectedFace = (constraintCellPos == cellPos) ? face : oppositeFace;
			}

			//Is this constraint relevant?
			if ((constraintCellPos != cellPos && constraintCellPos != cellPosNeighbor) ||
				(constraintCellPos == cellPos && constraintAffectedFace != face) ||
				(constraintCellPos == cellPosNeighbor && constraintAffectedFace != oppositeFace))
			{
				continue;
			}

			//Extract more info now that we know it's relevant.
			bool constraintIsForbidding;
			WfcFacePrototypeID constraintFaceID;
			WFC_Transforms2D constraintFacePermutation;
			if (_constraint.GetScriptStruct() == FWfcConstraintEntry_Face::StaticStruct())
			{
				auto& constraint = _constraint.Get<FWfcConstraintEntry_Face>();
				constraintIsForbidding = constraint.IsForbidding;
				constraintFaceID = constraint.FacePrototypeId;
				constraintFacePermutation = constraint.FacePrototypeTransform;
			}
			else
			{
				auto& constraint = _constraint.Get<FWfcConstraintEntry_Cell>();
				constraintIsForbidding = constraint.IsForbidding;

				//Figure out the face data for this permuted tile.
				auto wfcTileID = wfcLibraryData.WfcTileIDByUnrealID[constraint.TileId];
				auto permutedWfcFace = WFC::Tiled3D::GetFace(
					wfcLibraryData.Tiles[wfcTileID].Data,
					constraint.TilePermutation.Unwrap(),
					static_cast<WFC::Tiled3D::Directions3D>(constraintAffectedFace)
				);
				auto unrealFace = wfcLibraryData.ToUnrealFace(permutedWfcFace, tileset);

				constraintFaceID = unrealFace->Get<0>();
				constraintFacePermutation = unrealFace->Get<1>();
			}
			
			auto wfcSide = static_cast<WFC::Tiled3D::Directions3D>(constraintAffectedFace);
			auto wfcPermutation = static_cast<WFC::Transformations>(constraintFacePermutation);
			bool permutationIsInverted = !WFC::Tiled3D::IsFaceLeftHanded(wfcSide);
			auto wfcLocalPermutation = permutationIsInverted ? WFC::Invert(wfcPermutation) : wfcPermutation;
			
			if (constraintIsForbidding)
			{
				//Go through all symmetric permutations of this constraint's chosen face,
				//    and remove them.
				if (nonForbiddenFaces.Contains(constraintFaceID))
				{
					auto wfcFace = wfcLibraryData.ToWfcFace(constraintFaceID,
															tileset->FacePrototypes[constraintFaceID]);

					auto& permutationsLeft = nonForbiddenFaces[constraintFaceID];
					wfcFace.ForEachSymmetry(wfcLocalPermutation, [&](WFC::Transformations localSymmPerm) {
						auto symmPerm = permutationIsInverted ? WFC::Invert(localSymmPerm) : localSymmPerm;
						permutationsLeft -= symmPerm;
					});
					
					if (permutationsLeft.IsEmpty())
						nonForbiddenFaces.Remove(constraintFaceID);
				}
			}
			else
			{
				//For consistency we must use the first permutation that's symmetric with this face.
				WFC_Transforms2D consistentPerm = WFC_Transforms2D::None;
				auto wfcFace = wfcLibraryData.ToWfcFace(constraintFaceID,
														tileset->FacePrototypes[constraintFaceID]);
				wfcFace.ForEachSymmetry(wfcPermutation, [&](WFC::Transformations p) {
					consistentPerm = static_cast<WFC_Transforms2D>(p);
					return true; //Exit after the first iteration
				});
				
				output.AddInitialConstraint({ constraintFaceID, consistentPerm });
			}
		}
	}

	//Now process the forbidden faces and see if it's been narrowed down to a single face.
	bool foundSingleFace = false;
	if (nonForbiddenFaces.Num() == 1)
	{
		auto onlyFaceID = nonForbiddenFaces.begin()->Key;
		auto facePermutations = nonForbiddenFaces.begin()->Value;
		auto firstListedPermutation = *facePermutations.GetFirstElement();

		//Is every allowed permutation symmetric to the first one?
		//If so, then this is a single forced face we can submit.
		auto differentPermutationsFromFirst = facePermutations;
		TOptional<WFC::Transformations> firstSymmetricPermutation;
		tileset->FacePrototypes[onlyFaceID]
				  .Unwrap(wfcLibraryData.WfcFacePrototypeFirstIDs[onlyFaceID])
				  .ForEachSymmetry(firstListedPermutation, [&](WFC::Transformations symmetricPerm)
		{
			if (!firstSymmetricPermutation)
				firstSymmetricPermutation = symmetricPerm;
			differentPermutationsFromFirst -= symmetricPerm;
		});
		
		if (differentPermutationsFromFirst.IsEmpty())
		{
			foundSingleFace = true;
			output.AddInitialConstraint({ onlyFaceID, static_cast<WFC_Transforms2D>(*firstSymmetricPermutation) });
		}
	}
	if (!foundSingleFace)
	{
		//If the forbidden faces didn't single out one possible face, it's still possible they contradict other constraints.
		//TODO: Implement.
		if (false)
		{
			
		}
		//Otherwise, there are some constraints we can't properly capture in the output.
		else
		{
			output.HasLighterConstraints = countNonForbiddenFaces() < initialNonForbiddenFacesCount;
		}
	}
	nonForbiddenFaces.Empty();

	return output;
}
TOptional<TTuple<int, WFC_Transforms2D>> UWfcGenerator::GetFacePossibility(FIntVector cellPos,
																		   WFC_Directions3D face) const
{
	auto output = GetFacePossibilities(cellPos, face);
	auto* facePtr = output.TryGetFace();
	if (facePtr)
		return MakeTuple(facePtr->FacePrototypeID, facePtr->FacePermutation);
	else
		return NullOpt;
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
