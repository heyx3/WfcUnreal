#include "WfcEditorScenes/WfcTilesetEditorScene.h"

#include "GameFramework/WorldSettings.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcTileData.h"
#include "WfcTilesetEditorUtils.h"
#include "WfcEditorScenes/WfcTileVisualizer.h"


FWfcTilesetEditorScene::FWfcTilesetEditorScene(ConstructionValues cvs)
    : FAdvancedPreviewScene(cvs)
{
    auto& world = *GetWorld();
    auto& worldSettings = *world.GetWorldSettings();

    //Start up the world.
    worldSettings.NotifyBeginPlay();
    worldSettings.NotifyMatchStarted();
    worldSettings.SetActorHiddenInGame(false);
    world.bBegunPlay = true;

    //Configure the scene.
    SetFloorVisibility(false);
}

void FWfcTilesetEditorScene::Refresh(const UWfcTileset* tileset, TOptional<WfcTileID> tile, const FVector& camPos,
                                     FWfcTilesetEditorViewportClient* owner)
{
    check(owner);

	//Get the new tile data if it exists.
	const FWfcTile* newTileData;
	if (IsValid(tileset) && tile.IsSet())
		newTileData = tileset->Tiles.Find(*tile);
	else
		newTileData = nullptr;

	//If new and old tiles exist, check whether they're referencing different tiles.
	if (newTileData && currentTileVisualizer.IsValid() &&
		(currentTileVisualizer->GetInputs().Tileset != tileset ||
		 currentTileVisualizer->GetInputs().TileIdx != *tile))
	{
		currentTileVisualizer = WfcTileVisualizer::MakeVisualizer({
			*this, *owner,
			tileset, *tile, FTransform{ }
		});
	}
	//If new tile exists and the old doesn't, create a new visualizer.
	else if (newTileData && !currentTileVisualizer.IsValid())
	{
		currentTileVisualizer = WfcTileVisualizer::MakeVisualizer({
			*this, *owner,
			tileset, *tile, FTransform{ }
		});
	}
	//If old tile exists and new one doesn't, clear it out.
	else if (!newTileData && currentTileVisualizer.IsValid())
	{
		currentTileVisualizer.Reset();
	}

	//TODO: Scale each face's alpha based on camera focus. This requires sending camera data to the visualizer.
}
