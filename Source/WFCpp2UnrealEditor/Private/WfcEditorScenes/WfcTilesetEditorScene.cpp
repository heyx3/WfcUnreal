#include "WfcEditorScenes/WfcTilesetEditorScene.h"

#include "GameFramework/WorldSettings.h"
#include "Kismet/BlueprintSetLibrary.h"

#include "WFCpp2UnrealEditor.h"
#include "WfcGenerator.h"
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
                                     UWfcGeneratorInitialState* overrideInitialState,
                                     FWfcTilesetEditorViewportClient* owner)
{
    check(owner);

	//Get the new tile data if it exists.
	const FWfcTile* newTile = nullptr;
	if (IsValid(tileset) && tile.IsSet())
	{
		const auto* found = tileset->Tiles.Find(*tile);
		if (found)
			newTile = found;
	}
	
	//Update our state based on the refreshed tileset/selected tile/changes to public fields.
	if (tileset && Mode == EWfcTilesetEditorMode::Generation && currentViewMode != Mode)
	{
		viewMode.Emplace<FEditorSceneObject_WfcGeneration>(
			*this, *owner,
			FEditorSceneObject_WfcGeneration_Display{
				static_cast<float>(SpacingBetweenTiles),
				FTransform{ },
				DisplayFaceConstraintsInGeneration,
				DisplayHotSpotsInGeneration,
				DisplayUnsolvablesInGeneration,
				DisplayBoringCellsInGeneration
			},
			tileset, GenerationSettings
		);
		currentViewMode = Mode;
		
		NGeneratorTicksToRun = 0;
		owner->RedrawRequested(owner->Viewport);
	}
	else if (Mode == EWfcTilesetEditorMode::OverrideGeneration &&
			 overrideInitialState && (overrideInitialState->Tileset) &&
			 (!viewMode.IsType<FWfcTilesetEditorOverrideGeneration>() ||
			   #define WFC_OG (viewMode.Get<FWfcTilesetEditorOverrideGeneration>())
			    WFC_OG.InitialState.IsValid() != IsValid(overrideInitialState) || 
			 	!UWfcGeneratorInitialState::CompareWfcInitialStates(WFC_OG.InitialState.Get(), overrideInitialState)))
	{
		viewMode.Emplace<FWfcTilesetEditorOverrideGeneration>(
			*this, *owner,
			FEditorSceneObject_WfcGeneration_Display{
				static_cast<float>(SpacingBetweenTiles - tileset->TileLength),
				FTransform{ },
				DisplayFaceConstraintsInGeneration,
				DisplayHotSpotsInGeneration,
				DisplayUnsolvablesInGeneration,
				DisplayBoringCellsInGeneration
			},
			overrideInitialState
		);
		GenerationSettings = viewMode.Get<FWfcTilesetEditorOverrideGeneration>()
								.AsGeneration().GetCurrentSettings();
		currentViewMode = EWfcTilesetEditorMode::OverrideGeneration;
		
		NGeneratorTicksToRun = 0;
		owner->RedrawRequested(owner->Viewport);
	}
	else if (newTile && (
			 currentViewMode != Mode ||
			 currentTileset.Get() != tileset ||
			 !currentTile || !currentTileID.IsSet() ||
			 *newTile != *currentTile || *tile != *currentTileID ||
			 !currentTileset->FacePrototypes.OrderIndependentCompareEqual(tileset->FacePrototypes) ||
			 (viewMode.IsType<FEditorSceneObject_WfcTileWithMatches>() &&
			 	!SetsAreEqual(currentFacesToMatch, FacesToMatchAgainst)) ||
			 currentPermutationToMatch != PermutationToMatchAgainst
		))
	{
		currentTileset = { tileset };
		currentTileID = *tile;
		currentTile = *newTile;
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
						FLinearColor{ 0, 0, 0, 1 },
						ShowFaceData
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
							FLinearColor{ 0, 0, 0, 1 },
							ShowFaceData
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
							FLinearColor{ 0, 0, 0, 1 },
							ShowFaceData
						},
						FLinearColor{ 0, 0, 0, 1 }
					}
				);
			break;
			case EWfcTilesetEditorMode::Generation:
				viewMode.Emplace<FEditorSceneObject_WfcGeneration>(
					*this, *owner,
					FEditorSceneObject_WfcGeneration_Display{
						static_cast<float>(SpacingBetweenTiles),
						FTransform{ },
						DisplayFaceConstraintsInGeneration,
						DisplayHotSpotsInGeneration,
						DisplayUnsolvablesInGeneration,
						DisplayBoringCellsInGeneration
					},
					tileset, GenerationSettings
				);
				NGeneratorTicksToRun = 0;
			break;
			case EWfcTilesetEditorMode::OverrideGeneration:
				if (IsValid(overrideInitialState))
				{
					viewMode.Emplace<FWfcTilesetEditorOverrideGeneration>(
						*this, *owner,
						FEditorSceneObject_WfcGeneration_Display{
							static_cast<float>(SpacingBetweenTiles),
							FTransform{ },
							DisplayFaceConstraintsInGeneration,
							DisplayHotSpotsInGeneration,
							DisplayUnsolvablesInGeneration,
							DisplayBoringCellsInGeneration
						},
						overrideInitialState
					);
					NGeneratorTicksToRun = 0;
				}
			break;
			
			default:
			    check(false);
				viewMode.Emplace<std::nullptr_t>(nullptr);
			break;
		}

		currentViewMode = Mode;
		owner->RedrawRequested(owner->Viewport);
	}
	else if (!newTile &&
		     !viewMode.IsType<FEditorSceneObject_WfcGeneration>() &&
		     !viewMode.IsType<FWfcTilesetEditorOverrideGeneration>())
	{
		currentTileset.Reset();
		currentTile.Reset();
		currentTileID.Reset();
		currentViewMode.Reset();
		viewMode.Set<std::nullptr_t>(nullptr);
		
		owner->RedrawRequested(owner->Viewport);	
	}
	else if (viewMode.IsType<FEditorSceneObject_WfcGeneration>() ||
			 viewMode.IsType<FWfcTilesetEditorOverrideGeneration>())
	{
		auto& generatorManager = *GetGeneratorManager();

		//NOTE: the button may be clicked rapidly by the user, adding excess rewind commands before the button gets disabled.
		for (int rewindI = 0; rewindI < NRewindsToRun && generatorManager.GetGenerator()->GetHistoryLength() > 0; ++rewindI)
			generatorManager.Rewind();
		NRewindsToRun = 0;
		
		generatorManager.Tick(NGeneratorTicksToRun);
		NGeneratorTicksToRun = 0;

		generatorManager.ChangeSpace(FEditorSceneObject_WfcGeneration_Display{
			static_cast<float>(SpacingBetweenTiles),
			FTransform{ },
			DisplayFaceConstraintsInGeneration,
			DisplayHotSpotsInGeneration,
			DisplayUnsolvablesInGeneration,
			DisplayBoringCellsInGeneration
		});
		generatorManager.RefreshSettings(GenerationSettings);
		
		owner->RedrawRequested(owner->Viewport);	
	}

	//TODO: Scale each face's alpha based on camera focus. This requires sending camera data to the editor-object.
}