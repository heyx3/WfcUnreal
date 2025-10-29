#pragma once

#include "AdvancedPreviewScene.h"

#include <array>
#include <cstddef>

#include "WfcTileset.h"
#include "EditorSceneObjects.h"


class UBoxComponent;
class USphereComponent;
class UTextRenderComponent;


UENUM()
enum class EWfcTilesetEditorMode : uint8
{
	Tile,
	Permutations,
	Matches,
	Generation,

	COUNT UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EWfcTilesetEditorMode, static_cast<int64_t>(EWfcTilesetEditorMode::COUNT));

//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//Stores the actors/components in the 3D tile visualization scene.
//The runtime logic for the scene is handled by the "viewport client", FWfcTilesetEditorViewportClient.
//Displays a main tile visualization component,
//    plus another tile oriented so that it can line up with a specific face on the main tile.
class FWfcTilesetEditorScene : public FAdvancedPreviewScene
{
public:

	EWfcTilesetEditorMode Mode = EWfcTilesetEditorMode::Tile;
	double SpacingBetweenTiles = 500.0;
	FWFC_Transform3D PermutationToMatchAgainst;
	TSet<WFC_Directions3D> FacesToMatchAgainst = { WFC_Directions3D::MaxX };
	
	FEditorSceneObject_WfcGeneration_Settings GenerationSettings;
	int NGeneratorTicksToRun = 0; //Consumed on every Refresh() call
	int NRewindsToRun = 0; //Consumed on every Refresh() call

	
    FWfcTilesetEditorScene(ConstructionValues cvs = ConstructionValues());

	
    //Call continuously so that this scene can respond to changes in tile data, camera, etc.
    void Refresh(UWfcTileset* tileset, TOptional<WfcTileID> tileID, const FVector& camPos,
                 class FWfcTilesetEditorViewportClient* owner);

	//Gets the inner generation object, or null if this scene isn't generating right now.
	const FEditorSceneObject_WfcGeneration* GetGeneratorManager() const
	{
		return viewMode.IsType<FEditorSceneObject_WfcGeneration>() ?
				   &viewMode.Get<FEditorSceneObject_WfcGeneration>() :
				   nullptr;
	}
	
    
private:
    
    int32 chosenTileIdx;
    TWeakObjectPtr<class UWfcTileGameData> chosenTileData;

	TVariant<std::nullptr_t,
			 FEditorSceneObject_WfcTile,
			 FEditorSceneObject_WfcTileWithPermutations,
			 FEditorSceneObject_WfcTileWithMatches,
		     FEditorSceneObject_WfcGeneration
			> viewMode;
	
	TWeakObjectPtr<UWfcTileset> currentTileset;
	TOptional<FWfcTile> currentTile;
	TOptional<int> currentTileID;
	TOptional<EWfcTilesetEditorMode> currentViewMode;
	TSet<WFC_Directions3D> currentFacesToMatch;
	FWFC_Transform3D currentPermutationToMatch;
};