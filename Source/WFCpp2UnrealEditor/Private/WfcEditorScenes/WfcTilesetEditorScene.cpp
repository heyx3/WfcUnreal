#include "WfcEditorScenes/WfcTilesetEditorScene.h"

#include "GameFramework/WorldSettings.h"

#include "WFCpp2UnrealEditor.h"


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

	auto refreshViz = [&]()
	{
		check(newTileData.IsSet());
		currentTileset = { tileset };
		currentTileID = *tile;
		currentTile = currentTileset->Tiles[*currentTileID];
		
		if (VisualizeWithPermutations)	
		{
			viewMode.Emplace<FEditorSceneObject_WfcTileWithPermutations>(
				*this, *owner,
				FTransform{ }, PermutationSpacing,
				FLinearColor{ 0, 0, 0, 1 }, FLinearColor{ 0, 0, 0, 1 },
				tileset, *tile
			);
		}
		else
		{
			viewMode.Emplace<FEditorSceneObject_WfcTile>(
				*this, *owner,
				FTransform{ }, FLinearColor{ 0, 0, 0, 1 },
				tileset, *tile
			);
		}
	};
	
	//If new and old tiles exist, check whether the referenced tile has changed.
	if (newTileData && currentTileset.IsValid() &&
		(VisualizeWithPermutations != viewMode.IsType<FEditorSceneObject_WfcTileWithPermutations>() ||
		 currentTileset.Get() != tileset ||
		 !currentTile.IsSet() || *newTileData != *currentTile ||
		 !currentTileID.IsSet() || *tile != *currentTileID ||
		 currentTileset->FacePrototypes.OrderIndependentCompareEqual(tileset->FacePrototypes)))
	{
		refreshViz();
	}
	//If new tile exists and the old doesn't, create a new visualizer.
	else if (newTileData && viewMode.IsType<std::nullptr_t>())
	{
		refreshViz();
	}
	//If old tile exists and new one doesn't, clear it out.
	else if (!newTileData && !viewMode.IsType<std::nullptr_t>())
	{
		viewMode.Set<std::nullptr_t>(nullptr);
	}

	//TODO: Scale each face's alpha based on camera focus. This requires sending camera data to the editor-object.
}
