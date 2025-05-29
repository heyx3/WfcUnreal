#include "WfcEditorScenes/WfcTilesetEditorScene.h"

#include "GameFramework/WorldSettings.h"

#include "WFCpp2UnrealEditor.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "WfcEditorScenes/WfcTilesetEditorViewportClient.h"


template<typename TSet>
static bool SetsAreEqual(const TSet& a, const TSet& b)
{
	if (a.Num() != b.Num())
		return false;
	for (const auto& x : a)
		if (!b.Contains(x))
			return false;
	return true;
}

FWfcTilesetEditorScene::FWfcTilesetEditorScene(ConstructionValues cvs)
    : FAdvancedPreviewScene(cvs)
{
    auto& world = *GetWorld();
    auto& worldSettings = *world.GetWorldSettings();

    //Configure the scene.
	world.bCreateRenderStateForHiddenComponentsWithCollsion = true;
    SetFloorVisibility(false);
	SetEnvironmentVisibility(true);
}

void FWfcTilesetEditorScene::Refresh(UWfcTileset* tileset, TOptional<WfcTileID> tile, const FVector& camPos,
                                     FWfcTilesetEditorViewportClient* owner)
{
    check(owner);

	//Get the new tile data if it exists.
	TOptional<FWfcTile> newTileData;
	if (IsValid(tileset) && tile.IsSet())
	{
		const auto* found = tileset->Tiles.Find(*tile);
		if (found)
			newTileData.Emplace(*found);
	}

	//If existing tile data needs to be replaced (or created), then do so.
	if (newTileData && (
			currentViewMode != Mode ||
			currentTileset.Get() != tileset ||
			!currentTile.IsSet() || !currentTileID.IsSet() ||
			*newTileData != *currentTile || *tile != *currentTileID ||
			!currentTileset->FacePrototypes.OrderIndependentCompareEqual(tileset->FacePrototypes) ||
			(viewMode.IsType<FEditorSceneObject_WfcTileWithMatches>() &&
				!SetsAreEqual(currentFacesToMatch, FacesToMatchAgainst)) ||
			currentPermutationToMatch != PermutationToMatchAgainst
		))
	{
		check(newTileData.IsSet());
		currentTileset = { tileset };
		currentTileID = *tile;
		currentTile = *newTileData;
		currentFacesToMatch = FacesToMatchAgainst;
		currentPermutationToMatch = PermutationToMatchAgainst;

		switch (Mode)
		{
			case EWfcTilesetEditorMode::Tile:
				viewMode.Emplace<FEditorSceneObject_WfcTile>(
					*this, *owner,
					FTransform{ },
					tileset, *tile, currentPermutationToMatch,
					FEditorSceneObject_WfcTile_Settings{
						{
							1.0f, true
						},
						true,
						FLinearColor{ 0, 0, 0, 1 }
					}
				);
			break;
			case EWfcTilesetEditorMode::Permutations:
				viewMode.Emplace<FEditorSceneObject_WfcTileWithPermutations>(
					*this, *owner,
					FTransform{ }, SpacingBetweenTiles,
					tileset, *tile,
					FEditorSceneObject_WfcPermutations_Settings{
						{
							{
								1.0f, true
							},
							true,
							FLinearColor{ 0, 0, 0, 1 }
						},
						FLinearColor{ 0, 0, 0, 1 }, FLinearColor{ 0.4, 0.4, 0.4, 1 }
					}
				);
			break;
			case EWfcTilesetEditorMode::Matches:
				viewMode.Emplace<FEditorSceneObject_WfcTileWithMatches>(
					*this, *owner,
					FTransform{ }, SpacingBetweenTiles,
					tileset, *tile,
					currentPermutationToMatch, currentFacesToMatch,
					FEditorSceneObject_WfcMatches_Settings{
						{
							{
								1.0f, true
							},
							true,
							FLinearColor{ 0, 0, 0, 1 }
						},
						FLinearColor{ 0, 0, 0, 1 }
					}
				);
			break;
			
			default:
			    check(false);
				viewMode.Emplace<std::nullptr_t>(nullptr);
			break;
		}

		currentViewMode = Mode;
		owner->RedrawRequested(owner->Viewport);
	}
	else if (!newTileData)
	{
		currentTileset.Reset();
		currentTile.Reset();
		currentTileID.Reset();
		currentViewMode.Reset();
		viewMode.Set<std::nullptr_t>(nullptr);
	}

	//TODO: Scale each face's alpha based on camera focus. This requires sending camera data to the editor-object.
}