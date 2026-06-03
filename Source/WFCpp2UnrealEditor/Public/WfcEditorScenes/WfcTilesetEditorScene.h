#pragma once

#include "AdvancedPreviewScene.h"

#include <array>
#include <cstddef>

#include "WfcGenerator.h"
#include "EditorSceneObjects.h"


class UBoxComponent;
class USphereComponent;
class UTextRenderComponent;


UENUM()
enum class EWfcTilesetEditorMode : uint8
{
	//Showing only the tile itself
	Tile,
	//Showing the tile's full set of permutations
	Permutations,
	//Showing all matches for some of the tile's faces
	Matches,
	//Running a generator (not visualizing the specific tile at all)
	Generation,

	//A special mode which runs a generator based on an Initial State asset,
	//    not even connected to the tileset being edited.
	OverrideGeneration,

	COUNT UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(EWfcTilesetEditorMode, static_cast<int64_t>(EWfcTilesetEditorMode::COUNT));

struct FWfcTilesetEditorOverrideGeneration : private FEditorSceneObject_WfcGeneration
{
public:
	TStrongObjectPtr<const UWfcGeneratorInitialState> InitialState;

	FWfcTilesetEditorOverrideGeneration(FWfcTilesetEditorScene& owner, FWfcTilesetEditorViewportClient& viewportClient,
									    const FEditorSceneObject_WfcGeneration_Display& display,
										const UWfcGeneratorInitialState* initialState,
										int immediatelyRunNIterations = 0)
		: FEditorSceneObject_WfcGeneration(owner, viewportClient, display, initialState, immediatelyRunNIterations),
		  InitialState(initialState)
	{
		
	}

	FEditorSceneObject_WfcGeneration& AsGeneration() { return *this; }
	const FEditorSceneObject_WfcGeneration& AsGeneration() const { return *this; }
};

//Based on this wonderful tutorial:
//  https://easycomplex-tech.com/blog/Unreal/AssetEditor/UEAssetEditorDev-AssetEditorPreview/

//Stores the actors/components in the 3D tile visualization scene.
//The runtime logic for the scene is handled by the "viewport client", FWfcTilesetEditorViewportClient.
//
//Display modes:
//  1. Tile, just showing the tile itself
//  2. Permutation, showing the tile's full set of permutations
//  3. Matches, showing all matches for some of the tile's faces
//  4. Generation, running a generator (not visualizing the specific tile at all)
//  5. OverrideGeneration, a special mode which runs a generator based on an Initial State asset.
class FWfcTilesetEditorScene : public FAdvancedPreviewScene
{
public:

	EWfcTilesetEditorMode Mode = EWfcTilesetEditorMode::Tile;
	double SpacingBetweenTiles = 500.0;
	bool ShowFaceData = true;
	FWFC_Transform3D PermutationToMatchAgainst;
	TSet<WFC_Directions3D> FacesToMatchAgainst = { WFC_Directions3D::MaxX };
	
	FEditorSceneObject_WfcGeneration_Settings GenerationSettings;
	int NGeneratorTicksToRun = 0; //Consumed on every Refresh() call
	int NRewindsToRun = 0; //Consumed on every Refresh() call
	bool DisplayFaceConstraintsInGeneration = false,
		 DisplayHotSpotsInGeneration = false,
		 DisplayUnsolvablesInGeneration = true,
		 DisplayBoringCellsInGeneration = true;

	
    FWfcTilesetEditorScene(ConstructionValues cvs = ConstructionValues());

	
    //Call continuously so that this scene can respond to changes in tile data, camera, etc.
    void Refresh(UWfcTileset* tileset, TOptional<WfcTileID> tileID, const FVector& camPos,
    			 UWfcGeneratorInitialState* overrideInitialState,
                 class FWfcTilesetEditorViewportClient* owner);

	//Gets the inner generation object (whether normal or override),
	//    returning null if this scene isn't generating right now.
	FEditorSceneObject_WfcGeneration* GetGeneratorManager() { return const_cast<FEditorSceneObject_WfcGeneration*>(const_cast<const FWfcTilesetEditorScene*>(this)->GetGeneratorManager()); }
	//Gets the inner generation object (whether normal or override),
	//    returning null if this scene isn't generating right now.
	const FEditorSceneObject_WfcGeneration* GetGeneratorManager() const
	{
		if (viewMode.IsType<FEditorSceneObject_WfcGeneration>())
			return &viewMode.Get<FEditorSceneObject_WfcGeneration>();
		else if (viewMode.IsType<FWfcTilesetEditorOverrideGeneration>())
			return &viewMode.Get<FWfcTilesetEditorOverrideGeneration>().AsGeneration();
		else
			return nullptr;
	}
	
    
private:
    
    int32 chosenTileIdx;

	TVariant<std::nullptr_t,
			 FEditorSceneObject_WfcTile,
			 FEditorSceneObject_WfcTileWithPermutations,
			 FEditorSceneObject_WfcTileWithMatches,
		     FEditorSceneObject_WfcGeneration,
			 FWfcTilesetEditorOverrideGeneration
			> viewMode;
	
	TWeakObjectPtr<UWfcTileset> currentTileset;
	TOptional<FWfcTile> currentTile;
	TOptional<int> currentTileID;
	TOptional<EWfcTilesetEditorMode> currentViewMode;
	TSet<WFC_Directions3D> currentFacesToMatch;
	FWFC_Transform3D currentPermutationToMatch;
};