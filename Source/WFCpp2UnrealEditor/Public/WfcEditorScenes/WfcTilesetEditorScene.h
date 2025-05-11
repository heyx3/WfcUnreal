#pragma once

#include "AdvancedPreviewScene.h"

#include <array>
#include <cstddef>

#include "EditorSceneObjects.h"
#include "WfcTileset.h"


class UBoxComponent;
class USphereComponent;
class UTextRenderComponent;

//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//Stores the actors/components in the 3D tile visualization scene.
//The runtime logic for the scene is handled by the "viewport client", FWfcTilesetEditorViewportClient.
//Displays a main tile visualization component,
//    plus another tile oriented so that it can line up with a specific face on the main tile.
class FWfcTilesetEditorScene : public FAdvancedPreviewScene
{
public:

	bool VisualizeWithPermutations = false;
	double PermutationSpacing = 500.0;
	
    FWfcTilesetEditorScene(ConstructionValues cvs = ConstructionValues());

    //Call continuously so that this scene can respond to changes in tile data, camera, etc.
    void Refresh(UWfcTileset* tileset, TOptional<WfcTileID> tileID, const FVector& camPos,
                 class FWfcTilesetEditorViewportClient* owner);
    
private:
    
    int32 chosenTileIdx;
    TWeakObjectPtr<class UWfcTileGameData> chosenTileData;

	TVariant<std::nullptr_t,
			 FEditorSceneObject_WfcTile,
			 FEditorSceneObject_WfcTileWithPermutations
			> viewMode;
	
	TWeakObjectPtr<UWfcTileset> currentTileset;
	TOptional<FWfcTile> currentTile;
	TOptional<int> currentTileID;
};