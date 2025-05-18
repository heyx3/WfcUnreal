#pragma once

#include "AdvancedPreviewScene.h"

#include <array>
#include <cstddef>

#include "EditorSceneObjects.h"
#include "WfcTileset.h"

#include "WfcTilesetEditorScene.generated.h"


class UBoxComponent;
class USphereComponent;
class UTextRenderComponent;


UENUM()
enum class EWfcTilesetEditorMode : uint8
{
	Tile,
	Permutations,
	Matches,

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
	
    FWfcTilesetEditorScene(ConstructionValues cvs = ConstructionValues());

    //Call continuously so that this scene can respond to changes in tile data, camera, etc.
    void Refresh(UWfcTileset* tileset, TOptional<WfcTileID> tileID, const FVector& camPos,
                 class FWfcTilesetEditorViewportClient* owner);
    
private:
    
    int32 chosenTileIdx;
    TWeakObjectPtr<class UWfcTileGameData> chosenTileData;

	TVariant<std::nullptr_t,
			 FEditorSceneObject_WfcTile,
			 FEditorSceneObject_WfcTileWithPermutations,
			 FEditorSceneObject_WfcTileWithMatches
			> viewMode;
	
	TWeakObjectPtr<UWfcTileset> currentTileset;
	TOptional<FWfcTile> currentTile;
	TOptional<int> currentTileID;
	TOptional<EWfcTilesetEditorMode> currentViewMode;
	TSet<WFC_Directions3D> currentFacesToMatch;
	FWFC_Transform3D currentPermutationToMatch;
};